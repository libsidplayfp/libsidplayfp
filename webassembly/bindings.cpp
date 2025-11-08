#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>

#include <residfp.h>

namespace
{
constexpr uint32_t kDefaultSampleRate = 44100;
constexpr bool kDefaultStereo = true;

emscripten::val makeEmptyInt16Array()
{
    return emscripten::val::global("Int16Array").new_(0);
}

size_t extractLength(const emscripten::val &value)
{
    if (value.isUndefined() || value.isNull())
    {
        return 0;
    }

    const emscripten::val lengthVal = value["length"];
    if (!lengthVal.isUndefined() && !lengthVal.isNull())
    {
        return lengthVal.as<size_t>();
    }

    const emscripten::val byteLengthVal = value["byteLength"];
    if (!byteLengthVal.isUndefined() && !byteLengthVal.isNull())
    {
        return byteLengthVal.as<size_t>();
    }

    return 0;
}
}

class SidPlayerContext
{
public:
    SidPlayerContext()
        : builder(std::make_unique<ReSIDfpBuilder>("WasmReSIDfp")),
          stereo(kDefaultStereo),
          channels(kDefaultStereo ? 2u : 1u),
          sampleRate(kDefaultSampleRate),
          configured(false)
    {
    }

    bool configure(uint32_t frequency, bool stereoPlayback)
    {
        if (!builder)
        {
            lastError = "SID builder not initialized";
            return false;
        }

        sampleRate = frequency;
        stereo = stereoPlayback;
        channels = stereo ? 2u : 1u;

        SidConfig cfg;
        cfg.frequency = sampleRate;
        cfg.playback = stereo ? SidConfig::STEREO : SidConfig::MONO;
        cfg.sidEmulation = builder.get();
        cfg.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
        cfg.digiBoost = true;

        if (!player.config(cfg))
        {
            lastError = player.error();
            configured = false;
            return false;
        }

        if (player.installedSIDs() > 0U)
        {
            player.initMixer(stereo);
        }
        configured = true;
        return true;
    }

    bool loadSidFile(const std::string &path)
    {
        tune = std::make_unique<SidTune>(path.c_str());
        if (!tune->getStatus())
        {
            lastError = tune->statusString();
            tune.reset();
            return false;
        }

        return finalizeTuneLoad();
    }

    bool loadSidBuffer(emscripten::val data)
    {
        const size_t length = extractLength(data);

        if (length == 0)
        {
            lastError = "Buffer length is zero";
            return false;
        }

        tuneBuffer.resize(length);
        emscripten::val view = emscripten::val(emscripten::typed_memory_view(length, tuneBuffer.data()));
        view.call<void>("set", data);

        tune = std::make_unique<SidTune>(tuneBuffer.data(), static_cast<uint32_t>(tuneBuffer.size()));
        if (!tune->getStatus())
        {
            lastError = tune->statusString();
            tune.reset();
            return false;
        }

        return finalizeTuneLoad();
    }

    unsigned int selectSong(unsigned int song)
    {
        if (!tune)
        {
            return 0U;
        }

        const unsigned int selected = tune->selectSong(song);
        if (!player.load(tune.get()))
        {
            lastError = player.error();
            return 0U;
        }

        if (!player.reset())
        {
            lastError = player.error();
        }

        return selected;
    }

    emscripten::val render(unsigned int cycles)
    {
        if (!tune || !configured)
        {
            return emscripten::val::null();
        }

        const int produced = player.play(cycles);
        if (produced < 0)
        {
            lastError = player.error();
            return emscripten::val::null();
        }

        if (produced == 0)
        {
            return makeEmptyInt16Array();
        }

        const size_t requiredSamples = static_cast<size_t>(produced) * channels;
        if (mixBuffer.size() < requiredSamples)
        {
            mixBuffer.resize(requiredSamples);
        }

        const unsigned int written = player.mix(mixBuffer.data(), static_cast<unsigned int>(produced));
        if (written == 0)
        {
            return makeEmptyInt16Array();
        }

        return emscripten::val(emscripten::typed_memory_view(static_cast<size_t>(written), mixBuffer.data()));
    }

    bool reset()
    {
        if (!player.reset())
        {
            lastError = player.error();
            return false;
        }
        return true;
    }

    bool hasTune() const
    {
        return static_cast<bool>(tune);
    }

    bool isStereo() const
    {
        return stereo;
    }

    unsigned int getChannels() const
    {
        return channels;
    }

    uint32_t getSampleRate() const
    {
        return sampleRate;
    }

    std::string getLastError() const
    {
        return lastError;
    }

    emscripten::val getTuneInfo() const
    {
        if (!tune)
        {
            return emscripten::val::null();
        }

        const SidTuneInfo *info = tune->getInfo();
        if (!info)
        {
            return emscripten::val::null();
        }

        emscripten::val obj = emscripten::val::object();
        obj.set("songs", info->songs());
        obj.set("startSong", info->startSong());
        obj.set("currentSong", info->currentSong());
        obj.set("loadAddress", info->loadAddr());
        obj.set("initAddress", info->initAddr());
        obj.set("playAddress", info->playAddr());
        obj.set("dataFileLen", info->dataFileLen());
        obj.set("c64dataLen", info->c64dataLen());
        obj.set("clockSpeed", static_cast<int>(info->clockSpeed()));
        obj.set("format", info->formatString() ? info->formatString() : "");

        emscripten::val infoStrings = emscripten::val::array();
        const unsigned int infoCount = info->numberOfInfoStrings();
        for (unsigned int i = 0; i < infoCount; ++i)
        {
            const char *str = info->infoString(i);
            infoStrings.set(i, str ? str : "");
        }
        obj.set("infoStrings", infoStrings);

        emscripten::val commentStrings = emscripten::val::array();
        const unsigned int commentCount = info->numberOfCommentStrings();
        for (unsigned int i = 0; i < commentCount; ++i)
        {
            const char *str = info->commentString(i);
            commentStrings.set(i, str ? str : "");
        }
        obj.set("commentStrings", commentStrings);

        return obj;
    }

    emscripten::val getEngineInfo() const
    {
        const SidInfo &info = player.info();
        emscripten::val obj = emscripten::val::object();
        obj.set("name", info.name() ? info.name() : "");
        obj.set("version", info.version() ? info.version() : "");
        obj.set("channels", info.channels());
        obj.set("driverAddress", info.driverAddr());
        obj.set("driverLength", info.driverLength());
        obj.set("powerOnDelay", info.powerOnDelay());
        obj.set("speed", info.speedString() ? info.speedString() : "");

        emscripten::val creditsArray = emscripten::val::array();
        const unsigned int creditsCount = info.numberOfCredits();
        for (unsigned int i = 0; i < creditsCount; ++i)
        {
            const char *credit = info.credits(i);
            creditsArray.set(i, credit ? credit : "");
        }
        obj.set("credits", creditsArray);

        obj.set("kernal", info.kernalDesc() ? info.kernalDesc() : "");
        obj.set("basic", info.basicDesc() ? info.basicDesc() : "");
        obj.set("chargen", info.chargenDesc() ? info.chargenDesc() : "");

        obj.set("sidChips", info.numberOfSIDs());

        return obj;
    }

    bool setSystemROMs(emscripten::val kernal, emscripten::val basic, emscripten::val chargen)
    {
        const auto copyRom = [&](emscripten::val src, std::vector<uint8_t> &target, size_t expectedSize, const char *name) -> bool {
            if (src.isUndefined() || src.isNull())
            {
                target.clear();
                return true;
            }

            const size_t length = extractLength(src);
            if (length == 0)
            {
                lastError = std::string(name) + " buffer length is zero";
                return false;
            }

            if ((expectedSize != 0) && (length != expectedSize))
            {
                lastError = std::string(name) + " buffer expected " + std::to_string(expectedSize) + " bytes";
                return false;
            }

            target.resize(length);
            emscripten::val view = emscripten::val(emscripten::typed_memory_view(length, target.data()));
            view.call<void>("set", src);
            return true;
        };

        if (!copyRom(kernal, kernalRom, 8192, "KERNAL ROM"))
        {
            return false;
        }
        if (!copyRom(basic, basicRom, 8192, "BASIC ROM"))
        {
            return false;
        }
        if (!copyRom(chargen, chargenRom, 4096, "CHARGEN ROM"))
        {
            return false;
        }

        const uint8_t *kernalPtr = kernalRom.empty() ? nullptr : kernalRom.data();
        const uint8_t *basicPtr = basicRom.empty() ? nullptr : basicRom.data();
        const uint8_t *chargenPtr = chargenRom.empty() ? nullptr : chargenRom.data();

        player.setRoms(kernalPtr, basicPtr, chargenPtr);

        if (!player.reset())
        {
            lastError = player.error();
            return false;
        }

        player.initMixer(stereo);

        return true;
    }

private:
    bool finalizeTuneLoad()
    {
        if (!configured && !configure(sampleRate, stereo))
        {
            return false;
        }

        tune->selectSong(0);

        if (!player.load(tune.get()))
        {
            lastError = player.error();
            tune.reset();
            return false;
        }

        if (!player.reset())
        {
            lastError = player.error();
            return false;
        }

        if (player.installedSIDs() > 0U)
        {
            player.initMixer(stereo);
        }

        return true;
    }

    sidplayfp player;
    std::unique_ptr<ReSIDfpBuilder> builder;
    std::unique_ptr<SidTune> tune;
    std::vector<uint8_t> tuneBuffer;
    std::vector<int16_t> mixBuffer;
    std::vector<uint8_t> kernalRom;
    std::vector<uint8_t> basicRom;
    std::vector<uint8_t> chargenRom;
    bool stereo;
    unsigned int channels;
    uint32_t sampleRate;
    bool configured;
    std::string lastError;
};

EMSCRIPTEN_BINDINGS(libsidplayfp_wasm)
{
    emscripten::class_<SidPlayerContext>("SidPlayerContext")
        .constructor<>()
        .function("configure", &SidPlayerContext::configure)
        .function("loadSidFile", &SidPlayerContext::loadSidFile)
        .function("loadSidBuffer", &SidPlayerContext::loadSidBuffer)
        .function("selectSong", &SidPlayerContext::selectSong)
        .function("render", &SidPlayerContext::render)
        .function("reset", &SidPlayerContext::reset)
        .function("hasTune", &SidPlayerContext::hasTune)
        .function("isStereo", &SidPlayerContext::isStereo)
        .function("getChannels", &SidPlayerContext::getChannels)
        .function("getSampleRate", &SidPlayerContext::getSampleRate)
        .function("getLastError", &SidPlayerContext::getLastError)
        .function("getTuneInfo", &SidPlayerContext::getTuneInfo)
        .function("getEngineInfo", &SidPlayerContext::getEngineInfo)
        .function("setSystemROMs", &SidPlayerContext::setSystemROMs);

    emscripten::enum_<SidConfig::playback_t>("PlaybackMode")
        .value("MONO", SidConfig::MONO)
        .value("STEREO", SidConfig::STEREO);
}
