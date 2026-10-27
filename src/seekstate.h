/**
 * Save and restore the playback state of a player instance.
 */
#ifndef SEEK_STATE_H
#define SEEK_STATE_H
#include "builders/residfp-builder/residfp-emu.h"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include "call.h"
#include "sidplayfp/sidplayfp.h"
namespace libsidplayfp
{
struct SeekStateAccess
{
    std::vector<uint8_t>& bytes;
    size_t offset = 0;
    bool reading;

    SeekStateAccess(std::vector<uint8_t>& data, size_t position, bool read)
        : bytes(data), offset(position), reading(read)
    {
    }

    static uint16_t trackerEntry(Player& p)
    {
        const auto* info = p.m_tune->getInfo();
        const auto* ram = p.m_c64.mmu.ramBank.ram;
        if (p.m_chips.size() != 1 || info->sidChipBase(0) != 0xd400 ||
            info->compatibility() > SidTuneInfo::COMPATIBILITY_PSID || ram[0x1000] != 0x4c ||
            ram[0x1003] != 0x4c || ram[0x1006] != 0x4c)
            return 0;
        bool banner = false;
        for (unsigned at = 0x1009; at <= 0x1011; ++at)
            if (!std::memcmp(ram + at, " SID-WIZARD 1.", 14))
            {
                banner = true;
                break;
            }
        if (!banner)
            return 0;
        const unsigned entry = info->playAddr();
        if (entry == 0x1003)
            return entry;
        // SID-Maker's multispeed wrapper increments its own LDA operand.
        if (entry < 0xf80 || entry + 24 > 0x1000)
            return 0;
        const auto word = [&](unsigned at) { return ram[at] + 256 * ram[at + 1]; };
        if (ram[entry] == 0xee && word(entry + 1) == entry + 4 && ram[entry + 3] == 0xa9 &&
            ram[entry + 5] == 0xcd && word(entry + 6) >= 0x1009 && word(entry + 6) <= 0x1010 &&
            ram[entry + 8] == 0xf0 && ram[entry + 9] == 3 && ram[entry + 10] == 0x4c &&
            word(entry + 11) == 0x1006 && ram[entry + 13] == 0xa9 && ram[entry + 14] == 0 &&
            ram[entry + 15] == 0x8d && word(entry + 16) == entry + 4 && ram[entry + 18] == 0x4c &&
            word(entry + 19) == 0x1003 && ram[word(entry + 6)] >= 2 && ram[word(entry + 6)] <= 8)
            return entry;
        return 0;
    }
    [[gnu::cold, gnu::noinline]] static bool enableTrackerSeek(Player& p, const char* identity)
    {
        if (!p.m_tune || !identity)
            return false;
        const bool future = !std::strcmp(identity, "MoN/FutureComposer") ||
                            !std::strcmp(identity, "(FutureComposer_V1.0)") ||
                            !std::strcmp(identity, "(FC_V4_Packed)");
        const bool soundmaker = !std::strcmp(identity, "SoundMaker") ||
                                !std::strcmp(identity, "(SoundMaker_V3)") ||
                                !std::strcmp(identity, "(SoundMaker_V4)");
        const bool master = !std::strcmp(identity, "TFMX/MasterComposer");
        const bool ubik = !std::strcmp(identity, "Ubik's_Musik");
        const bool more = soundmaker || master || ubik || !std::strcmp(identity, "DefMon") ||
                          !std::strcmp(identity, "Equalizer") ||
                          !std::strcmp(identity, "Cyberlogic_SoundStudio") ||
                          !std::strcmp(identity, "MusicComposer/FlashInc") ||
                          !std::strcmp(identity, "(MusicComposer_V3.0)") ||
                          !std::strcmp(identity, "(MusicComposer_V4.0)");
        const bool defmon = !std::strcmp(identity, "DefMon");
        const bool composer = !std::strcmp(identity, "MusicComposer/FlashInc") ||
                              !std::strcmp(identity, "(MusicComposer_V3.0)") ||
                              !std::strcmp(identity, "(MusicComposer_V4.0)");
        const auto word = [&](unsigned at)
        {
            return static_cast<unsigned>(p.m_c64.mmu.ramBank.ram[at]) +
                   256 * p.m_c64.mmu.ramBank.ram[at + 1];
        };
        const bool cyberExe = !std::strcmp(identity, "CyberTracker_exe");
        const bool mini1 = !std::strcmp(identity, "GoatTracker_V2/Mini");
        const bool mini2 = !std::strcmp(identity, "GoatTracker_V2/Mini2");
        const bool ninja1 = !std::strcmp(identity, "NinjaTracker_V1.x");
        const bool ninja = ninja1 || !std::strcmp(identity, "NinjaTracker_V2.x");
        const bool sdi = !std::strcmp(identity, "Geir_Tjelta/SIDDuzz'It");
        const bool factory1 = !std::strcmp(identity, "SidFactory/Laxity");
        const bool factory2 = !std::strcmp(identity, "SidFactory_II/Laxity");
        const bool sidtracker = !std::strcmp(identity, "SidTracker64");
        const bool jitt = !std::strcmp(identity, "JITT64_1.x");
        const bool known =
            cyberExe || more || jitt || future || !std::strcmp(identity, "OdinTracker") ||
            !std::strcmp(identity, "SadoTracker") || !std::strcmp(identity, "CyberTracker") ||
            mini1 || mini2 || ninja || sdi || factory1 || factory2 || sidtracker ||
            !std::strcmp(identity, "GoatTracker_V1.x") ||
            !std::strcmp(identity, "GoatTracker_V2.x") || !std::strcmp(identity, "JCH_NewPlayer") ||
            !std::strncmp(identity, "(JCH_NewPlayer_V", sizeof("(JCH_NewPlayer_V") - 1) ||
            !std::strcmp(identity, "CheeseCutter_2.x") ||
            !std::strncmp(identity, "(CheeseCutter_", sizeof("(CheeseCutter_") - 1) ||
            !std::strcmp(identity, "DMC") || !std::strcmp(identity, "(DMC_V4.x)") ||
            !std::strcmp(identity, "(DMC_V5.x)") || !std::strcmp(identity, "DMC_V6.x") ||
            !std::strcmp(identity, "Music_Assembler") || !std::strcmp(identity, "(VoiceTracker)") ||
            !std::strcmp(identity, "SidWinder");
        if (!known)
            return false;
        const auto* info = p.m_tune->getInfo();
        const auto* ram = p.m_c64.mmu.ramBank.ram;
        if (p.m_chips.size() != 1 || info->sidChipBase(0) != 0xd400 ||
            info->compatibility() > SidTuneInfo::COMPATIBILITY_PSID)
            return false;
        const unsigned load = info->loadAddr(), length = info->c64dataLen(),
                       play = info->playAddr(), end = load + length;
        if (load < 0x200 || end > 0xd000 || length < 7)
            return false;
        if (defmon && info->initAddr() == load && play == load + 17)
        {
            if (play + 22 > end || ram[load] != 0xa9 || ram[load + 2] != 0x8d ||
                word(load + 3) != 0xdc04 || ram[load + 5] != 0xa9 || ram[load + 7] != 0x8d ||
                word(load + 8) != 0xdc05 || ram[load + 10] != 0xa9 || ram[load + 11] != 0 ||
                ram[load + 12] != 0x85 || ram[load + 13] != 2 || ram[load + 14] != 0x4c)
                return false;
            const unsigned base = word(load + 15), table = word(play + 3), count = ram[play + 10];
            if (base < load || base + 9 > end || (base & 255) || ram[base] != 0x4c ||
                ram[base + 3] != 0x4c || ram[play] != 0xa6 || ram[play + 1] != 2 ||
                ram[play + 2] != 0xbd || ram[play + 5] != 0x8d || word(play + 6) != play + 18 ||
                ram[play + 8] != 0xe8 || ram[play + 9] != 0xe0 || count < 2 || count > 16 ||
                ram[play + 11] != 0xd0 || ram[play + 12] != 2 || ram[play + 13] != 0xa2 ||
                ram[play + 14] != 0 || ram[play + 15] != 0x86 || ram[play + 16] != 2 ||
                ram[play + 17] != 0x4c ||
                (word(play + 18) != base + 3 && word(play + 18) != base + 6) || table < load ||
                table + count > end)
                return false;
            for (unsigned n = 0; n < count; ++n)
                if (ram[table + n] != 3 && ram[table + n] != 6)
                    return false;
        }
        else if (composer && info->initAddr() == load && play == load + 16)
        {
            if (play + 12 > end || ram[load] != 0xa2 || ram[load + 2] != 0x8e ||
                word(load + 3) != 0xdc04 || ram[load + 5] != 0xa2 || ram[load + 7] != 0x8e ||
                word(load + 8) != 0xdc05 || ram[load + 10] != 0x4c)
                return false;
            const unsigned base = word(load + 11);
            if (base < load || base + 9 > end || (base & 255) || ram[base] != 0x4c ||
                ram[base + 3] != 0x4c || ram[base + 6] != 0x4c || ram[play] != 0x20 ||
                (word(play + 1) != base + 3 && word(play + 1) != base + 6) ||
                ram[play + 3] != 0xad || word(play + 4) != play + 1 || ram[play + 6] != 0x49 ||
                ram[play + 7] != 5 || ram[play + 8] != 0x8d || word(play + 9) != play + 1 ||
                ram[play + 11] != 0x60)
                return false;
        }
        else if (cyberExe)
        {
            const auto body = [&](unsigned at)
            {
                if (at >= load && at + 3 <= end && ram[at] == 0x4c)
                    return static_cast<unsigned>(ram[at + 1]) + 256 * ram[at + 2];
                return at;
            };
            const unsigned init = body(info->initAddr()), code = body(play);
            if (init < load || init + 9 > end || code < load || code + 15 > end ||
                ram[init] != 0x20 || ram[init + 3] != 0x20 || ram[init + 6] != 0x20 ||
                ram[code] != 0x20 || ram[code + 3] != 0x2c || ram[code + 6] != 0x20 ||
                ram[code + 9] != 0x20 || ram[code + 12] != 0x20)
                return false;
        }
        else if (master || ubik)
        {
            const unsigned init = info->initAddr();
            if (init < load || init + 3 > end || play < load || play + 3 > end ||
                ram[init] != 0x4c || ram[play] != 0x4c ||
                (master ? (play != load || init != load + 3) : play != init + 3))
                return false;
        }
        else if (jitt)
        {
            const unsigned init = info->initAddr();
            const unsigned initTarget = ram[load + 1] + 256 * ram[load + 2],
                           playTarget = ram[load + 4] + 256 * ram[load + 5];
            if (ram[load] != 0x4c || ram[load + 3] != 0x4c || initTarget < load ||
                initTarget >= end || playTarget < load || playTarget >= end ||
                (init != load && init != initTarget) || (play != load + 3 && play != playTarget))
                return false;
        }
        else if (mini1)
        {
            const unsigned init = info->initAddr();
            if (init < load || init + 3 > end || play != init + 3 || play + 3 > end ||
                ram[init] != 0x4c || ram[play] != 0x4c)
                return false;
            unsigned code = play;
            for (unsigned hops = 0; hops < 3; ++hops)
            {
                if (code < load || code >= end)
                    return false;
                for (unsigned n = 0; n < 4 && code < end && ram[code] == 0xea; ++n)
                    ++code;
                if (code + 3 > end)
                    return false;
                if (ram[code] != 0x4c)
                    break;
                code = static_cast<unsigned>(ram[code + 1]) + 256 * ram[code + 2];
            }
            if (code < load || code + 8 > end || ram[code] != 0xa2 || ram[code + 2] != 0x30 ||
                ram[code + 4] != 0xf0 || ram[code + 6] != 0xca || ram[code + 7] != 0x8a)
                return false;
        }
        else if (mini2)
        {
            const unsigned init = info->initAddr();
            if (init < load || init + 6 > end || play < load || play + 12 > end ||
                ram[init] != 0xaa || ram[init + 1] != 0xe8 || ram[init + 2] != 0x8e ||
                static_cast<unsigned>(ram[init + 3]) + 256 * ram[init + 4] != play + 1 ||
                ram[init + 5] != 0x60 || ram[play] != 0xa2 || ram[play + 2] != 0xd0 ||
                ram[play + 4] != 0xa0 || ram[play + 6] != 0xf0 || ram[play + 8] != 0x30 ||
                ram[play + 10] != 0xa9)
                return false;
        }
        else
        {
            const unsigned offset = (factory1 || factory2 || future || soundmaker) ? 6
                                    : sdi                                          ? 4
                                                                                   : 3;
            if (info->initAddr() != load || play != load + offset)
                return false;
            if (future)
            {
                if (play + 9 > end || ram[load] != 0x4c || ram[play] != 0xad ||
                    ram[play + 3] != 0xc9 || ram[play + 4] != 2 || ram[play + 5] != 0xf0 ||
                    ram[play + 7] != 0xc9 || ram[play + 8] != 1)
                    return false;
            }
            else if (factory1 || factory2)
            {
                if (ram[load] != 0x4c ||
                    !(ram[load + 3] == 0x4c ||
                      (ram[load + 3] == 0xea && ram[load + 4] == 0xea && ram[load + 5] == 0xea)))
                    return false;
                if (factory1)
                {
                    if (play + 7 > end || ram[play] != 0x2c || ram[play + 3] != 0x30 ||
                        ram[play + 5] != 0x70)
                        return false;
                }
                else if (play + 9 > end || ram[play] != 0xa9 || ram[play + 2] != 0x2c ||
                         ram[play + 5] != 0x30 || ram[play + 7] != 0x70)
                    return false;
            }
            else if (sidtracker)
            {
                if (play + 7 > end || ram[load] != 0x4c || ram[play] != 0xa2 ||
                    ram[play + 1] != 0 || ram[play + 2] != 0x20 || ram[play + 5] != 0xa2 ||
                    ram[play + 6] != 7)
                    return false;
            }
            else if (ninja)
            {
                if (ram[play] != 0xa2 || ram[play + 1] != 0 ||
                    ram[play + 2] != (ninja1 ? 0xa0 : 0xa9))
                    return false;
            }
            else if (ram[load + (sdi ? 1 : 0)] != 0x4c || ram[play] != 0x4c ||
                     (sdi && ram[load] != 0xaa))
                return false;
        }
        p.m_trackerEntry = play;
        return true;
    }
    static bool seekTracker(Player& p, unsigned targetMs)
    {
        if (!p.m_tune || !p.m_trackerEntry || !p.m_trackerSeekAllowed || targetMs <= p.timeMs() ||
            targetMs - p.timeMs() < 200)
            return false;
        auto& m = p.m_c64;
        auto& c = m.cpu;
        auto* ram = m.mmu.ramBank.ram;
        auto* chip = dynamic_cast<reSIDfpEmu*>(p.m_chips[0]);
        if (!chip || !chip->m_silent)
            return false;
        // Initialisation can read the SID and store the result in RAM.
        // A SID read after skipped emulation requires replay from the start.
        struct RestoreModes
        {
            MOS6510& cpu;
            bool loops, native;
            ~RestoreModes()
            {
                cpu.fastCpuSeek(loops);
                cpu.nativePcmSeek(native);
            }
        } restore{c, c.m_loopEnabled, c.m_pcmEnabled};
        p.fastCpuSeek(false);
        p.nativePcmSeek(false);
        const unsigned entry = p.m_trackerEntry;
        unsigned probes = 0;
        while (!(c.Register_ProgramCounter == entry + 1 && c.cycleCount == (ram[entry] << 3) &&
                 !c.m_pcmActive && !c.m_idleActive))
        {
            // Initialisation can take more than one bounded search.
            if (++probes > 100000)
                return false;
            if (p.play(1) < 0)
            {
                p.m_trackerSeekAllowed = false;
                return false;
            }
        }
        if (m.vic.regs[0x15])
            return false;
        if (!c.rdy || !c.flags.getI() || c.flags.getD() || c.nmiFlag || c.rstFlag ||
            m.cia2.interruptSource->icr || (m.cia1.interruptSource->icr & 0x1e))
            return false;
        const bool basicRam = !(m.mmu.loram && m.mmu.hiram);
        if (!m.mmu.charen || !(m.mmu.loram || m.mmu.hiram))
            return false;
        const auto* info = p.m_tune->getInfo();
        unsigned period;
        if (info->songSpeed() == SidTuneInfo::SPEED_VBI)
        {
            if (m.cia1.interruptSource->icr || m.vic.irqMask != 1)
                return false;
            period = m.vic.cyclesPerLine * m.vic.maxRasters;
        }
        else
        {
            if (m.cia1.interruptSource->icr != 1 || m.vic.irqMask || (m.cia1.regs[14] & 0x29) != 1)
                return false;
            period = static_cast<unsigned>(m.cia1.timerA.latch) + 1;
        }
        if (targetMs <= p.timeMs() + 40 || !period)
            return false;
        const double available = (targetMs - p.timeMs()) * m.cpuFrequency / (1000.0 * period);
        if (available < 2)
            return false;
        const unsigned count = static_cast<unsigned>(std::min(available - 1, 256.0));
        const auto reject = [&]() -> bool
        {
            p.m_trackerSeekAllowed = false;
            // Discarded hardware time cannot support a newly encountered dependency.
            if (p.m_trackerSkippedCalls)
                throw sidplayfp::SeekReplayRequired();
            return false;
        };
        unsigned code = entry;
        if (code <= 0xcffd && ram[code] == 0x4c)
            code = static_cast<unsigned>(ram[code + 1]) + 256 * ram[code + 2];
        const bool banked =
            code >= info->loadAddr() &&
            code + 15 <= static_cast<unsigned>(info->loadAddr()) + info->c64dataLen() &&
            ram[code] == 0x20 && ram[code + 3] == 0x2c && ram[code + 6] == 0x20 &&
            ram[code + 9] == 0x20 && ram[code + 12] == 0x20;
        PlayerCallBank bank{};
        if (banked)
        {
            const unsigned spEntry = c.Register_StackPointer;
            const unsigned resume = (ram[0x100 | ((spEntry + 1) & 255)] +
                                     256 * ram[0x100 | ((spEntry + 2) & 255)] + 1) &
                                    65535;
            const uint8_t tail[] = {0x68, 0x85, 1,    0xce, 0x19, 0xd0, 0xad, 0x0d,
                                    0xdc, 0x68, 0xa8, 0x68, 0xaa, 0x68, 0x40};
            const unsigned driverStart = p.m_info.m_driverAddr,
                           driverEnd = driverStart + p.m_info.m_driverLength;
            if (resume < driverStart + 13 || resume + sizeof(tail) > driverEnd ||
                std::memcmp(ram + resume, tail, sizeof(tail)) || ram[resume - 13] != 0xa5 ||
                ram[resume - 12] != 1 || ram[resume - 11] != 0x48 || ram[resume - 10] != 0xad ||
                ram[resume - 7] != 0x85 || ram[resume - 6] != 1 || ram[resume - 5] != 0xa9 ||
                ram[resume - 4] != 0 || ram[resume - 3] != 0x20)
                return reject();
            const unsigned stub = ram[resume - 2] + 256 * ram[resume - 1];
            const unsigned map = ram[resume - 9] + 256 * ram[resume - 8];
            if (stub < driverStart || stub + 3 > driverEnd || ram[stub] != 0x6c ||
                map < driverStart || map >= driverEnd || ram[map] != m.mmu.zeroRAMBank.data)
                return reject();
            const unsigned vector = ram[stub + 1] + 256 * ram[stub + 2];
            if (vector < driverStart || vector + 2 > driverEnd ||
                ram[vector] != static_cast<uint8_t>(entry) ||
                ram[vector + 1] != static_cast<uint8_t>(entry >> 8))
                return reject();
        }
        PlayerCall call{};
        std::memcpy(call.memory, ram, 65536);
        if (banked)
        {
            auto& port = m.mmu.zeroRAMBank;
            if ((port.dir & 0xc7) != 7 || port.data < 0x34 || port.data > 0x37 ||
                port.peek(1) != port.data)
                return reject();
            bank.dir = port.dir;
            bank.data = port.data;
            bank.pins = port.peek(1);
            bank.seed = m.mmu.seed;
            std::memcpy(bank.colours, m.vic.regs + 0x20, 15);
        }
        else
            std::memcpy(call.memory + 0xd020, m.vic.regs + 0x20, 15);
        uint8_t regs[32]{};
        p.m_chips[0]->getStatus(regs);
        if (banked)
            std::memcpy(bank.sid, regs, 25);
        else
            std::memcpy(call.memory + 0xd400, regs, 25);
        const auto a = c.Register_Accumulator, x = c.Register_X, y = c.Register_Y,
                   sp = c.Register_StackPointer, sr = c.flags.get();
        const unsigned driverLow = p.m_info.m_driverAddr,
                       driverHigh = driverLow + p.m_info.m_driverLength;
        for (unsigned n = 0; n < count; ++n)
        {
            call.pc = entry;
            call.a = a;
            call.x = x;
            call.y = y;
            call.sp = sp;
            call.p = sr;
            if (!(banked ? prepare_player_call_banked(&call, &bank)
                         : prepare_player_call_colours(&call, basicRam)))
            {
#ifdef SID_TRACKER_DIAGNOSTICS
                std::printf("Tracker fallback: call=%u CPU error=%u\n", n, call.error);
#endif
                return reject();
            }
            if (banked)
            {
                // The verified driver restores and reselects the port between updates.
                bank.seed = bank.seed * 1664525u + 1013904223u;
                bank.seed = bank.seed * 1664525u + 1013904223u;
                call.memory[1] = bank.seed & 255;
            }
            // The IRQ caller restores these bytes before the next update.
            if (std::memcmp(call.memory + 0x101 + sp, ram + 0x101 + sp, 0xff - sp) ||
                std::memcmp(call.memory + 0x314, ram + 0x314, 6))
                return reject();
            // Use ordinary emulation when a player call exceeds its interrupt period.
            if (call.cycles + 128 >= period)
            {
                return reject();
            }
            for (unsigned i = 0; i < call.count; ++i)
            {
                unsigned address = call.writes[i].address;
                // Reject writes to the driver's entry and scheduling code.
                if (((!banked || call.writes[i].kind == 1) && address >= 0xd419 &&
                     address < 0xd480) ||
                    (address >= driverLow && address < driverHigh))
                {
#ifdef SID_TRACKER_DIAGNOSTICS
                    std::printf("Tracker fallback: driver or extra SID write $%04x\n", address);
#endif
                    return reject();
                }
            }
        }
        // Resume at the call boundary with the caller's stack.
        // The private interpreter writes RAM, SID registers and inert VIC colours.
        if (banked)
        {
            std::memcpy(ram + 1, call.memory + 1, 65535);
            m.mmu.seed = bank.seed;
            std::memcpy(m.vic.regs + 0x20, bank.colours, 15);
        }
        else
        {
            std::memcpy(ram + 2, call.memory + 2, 0xa000 - 2);
            if (basicRam)
                std::memcpy(ram + 0xa000, call.memory + 0xa000, 0x2000);
            std::memcpy(ram + 0xc000, call.memory + 0xc000, 0x1000);
            std::memcpy(m.vic.regs + 0x20, call.memory + 0xd020, 15);
        }
        std::memset(c.m_loopCache, 0, sizeof(c.m_loopCache));
        std::memset(c.m_loopSeenPages, 0, sizeof(c.m_loopSeenPages));
        c.m_loopCacheUsed = false;
        c.m_pcmCached = false;
        for (unsigned i = 0; i < 25; ++i)
            chip->poke(i, banked ? bank.sid[i] : call.memory[0xd400 + i]);
        chip->m_sid.restoreFastSilentUsed(true);
        p.m_trackerSkippedCycles += static_cast<uint64_t>(count) * period;
        p.m_trackerSkippedCalls += count;
        return true;
    }

    void block(void* data, size_t size)
    {
        if (reading)
        {
            if (offset > bytes.size() || size > bytes.size() - offset)
                throw std::runtime_error("Invalid SID checkpoint");
            std::memcpy(data, bytes.data() + offset, size);
        }
        else
        {
            const auto* first = static_cast<const uint8_t*>(data);
            bytes.insert(bytes.end(), first, first + size);
        }
        offset += size;
    }
    template <class T> void field(T& value)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "Checkpoint fields must not own resources");
        block(&value, sizeof(value));
    }
    template <class... T> void fields(T&... values)
    {
        int expanded[] = {0, (field(values), 0)...};
        (void)expanded;
    }
    void event(Event& e)
    {
        fields(e.next, e.triggerTime);
    }
    void timer(Timer& t)
    {
        event(t);
        event(t.m_cycleSkippingEvent);
        fields(t.ciaEventPauseTime, t.pbToggle, t.timer, t.latch, t.lastControlValue, t.m_state);
    }
    void interrupt(InterruptSource& i)
    {
        fields(i.last_clear, i.last_set, i.icr, i.idr, i.idrTemp, i.scheduled, i.asserted);
        event(i.interruptEvent);
        event(i.updateIdrEvent);
        event(i.setIrqEvent);
        event(i.clearIrqEvent);
    }
    void tod(Tod& t)
    {
        event(t);
        fields(t.cycles, t.period, t.todtickcounter, t.isLatched, t.isStopped, t.m_clock, t.m_latch,
               t.m_alarm);
    }
    void serial(SerialPort& s)
    {
        event(s);
        event(s.flipCntEvent);
        event(s.flipFakeEvent);
        event(s.startSdrEvent);
        fields(s.lastSync, s.count, s.cnt, s.cntHistory, s.loaded, s.pending, s.forceFinish,
               s.model4485);
    }
    void cia(MOS652X& c)
    {
        field(c.regs);
        timer(c.timerA);
        timer(c.timerB);
        interrupt(*c.interruptSource);
        tod(c.tod);
        serial(c.serialPort);
        event(c.bTickEvent);
    }
    void vic(MOS656X& v)
    {
        event(v);
        fields(v.clock, v.rasterClk, v.cyclesPerLine, v.maxRasters, v.lineCycle, v.rasterY,
               v.yscroll, v.areBadLinesEnabled, v.isBadLine, v.rasterYIRQCondition, v.vblanking,
               v.lpAsserted, v.irqFlags, v.irqMask, v.regs, v.lp);
        fields(v.sprites.exp_flop, v.sprites.dma, v.sprites.mc_base, v.sprites.mc);
        event(v.badLineStateChangeEvent);
        event(v.rasterYIRQEdgeDetectorEvent);
        event(v.lightpenTriggerEvent);
    }
    void cpu(MOS6510& c)
    {
        fields(c.cycleCount, c.interruptCycle, c.irqAssertedOnPin, c.nmiFlag, c.rstFlag, c.rdy,
               c.adl_carry, c.d1x1, c.rdyOnThrowAwayRead, c.flags, c.Register_ProgramCounter,
               c.Cycle_EffectiveAddress, c.Cycle_Pointer, c.Cycle_Data, c.Register_StackPointer,
               c.Register_Accumulator, c.Register_X, c.Register_Y);
        fields(c.instrTable[MOS6510::nativeSlot], c.m_pcmActions, c.m_pcmActionCount,
               c.m_pcmActionIndex, c.m_pcmCycle, c.m_pcmEnabled, c.m_pcmActive, c.m_pcmCached,
               c.m_pcmLow, c.m_pcmHigh, c.m_pcmScratch, c.m_pcmEnd, c.m_pcmDac, c.m_pcmPointer,
               c.m_pcmAdd, c.m_pcmEndValue, c.m_pcmSample, c.m_pcmSavedA, c.m_pcmSavedY,
               c.m_pcmSavedSP, c.m_pcmCalls, c.m_pcmMemoryWrites, c.m_pcmMemoryCount,
               c.m_pcmWriteCycle, c.m_pcmClock, c.m_idleActive, c.m_idlePC, c.m_idlePhase,
               c.m_idleClock);
        fields(c.m_loopEnabled, c.m_loopSkippedCycles, c.m_playerCallActive, c.m_playerActions,
               c.m_playerWriteCycle);
        if (reading)
        {
            std::memset(c.m_loopCache, 0, sizeof(c.m_loopCache));
            std::memset(c.m_loopSeenPages, 0, sizeof(c.m_loopSeenPages));
            c.m_loopCacheUsed = false;
        }
        event(c.m_nosteal);
        event(c.m_steal);
        event(c.clearInt);
    }
    void memory(MMU& m)
    {
        fields(m.loram, m.hiram, m.charen, m.seed, m.cpuReadMap, m.cpuWriteMap, m.ramBank.ram);
        auto& z = m.zeroRAMBank;
        fields(z.dataBit6, z.dataBit7, z.dir, z.data, z.dataRead, z.procPortPins);
    }
    void xsidChannel(XsidChannel& c)
    {
        event(c.sampleEvent);
        event(c.galwayEvent);
        fields(c.m_phase, c.reg, c.mode, c.active, c.address, c.cycleCount, c.volShift,
               c.sampleLimit, c.sample, c.samRepeat, c.samScale, c.samOrder, c.samNibble,
               c.samEndAddr, c.samRepeatAddr, c.samPeriod, c.galTones, c.galInitLength, c.galLength,
               c.galVolume, c.galLoopWait, c.galNullWait, c.cycles, c.outputs);
    }
    void xsid(XSID& x)
    {
        event(x);
        xsidChannel(x.ch4);
        xsidChannel(x.ch5);
        fields(x.muted, x.suppressed, x.sidData0x18, x._sidSamples, x.sampleOffset, x.wasRunning);
    }
    void machine(c64& c)
    {
        fields(c.cpuFrequency, c.irqCount, c.oldBAState, c.eventScheduler.firstEvent,
               c.eventScheduler.currentTime, c.eventScheduler.batchLimit,
               c.eventScheduler.batchCount);
        cia(c.cia1);
        field(c.cia1.last_ta);
        cia(c.cia2);
        vic(c.vic);
        field(c.colorRAMBank.ram);
        memory(c.mmu);
        cpu(c.cpu);
        xsid(c.playSid);
    }
    void chip(reSIDfpEmu& s)
    {
        fields(s.lastpoke, s.m_accessClk, s.isMuted, s.isFilterDisabled, s.m_silent, s.m_fastPcm, s.m_pcmDetected,
               s.m_readbackUsed, s.m_volumeWrites, s.m_lastVolume, s.m_volumeProbeStart,
               s.m_probeWindow);
        bool used = s.m_sid.fastSilentUsed();
        field(used);
        const auto size = static_cast<size_t>(s.m_sid.stateSize());
        std::vector<char> state(size);
        if (!reading &&
            s.m_sid.saveState(state.data(), static_cast<int>(size)) != static_cast<int>(size))
            throw std::runtime_error("Cannot save SID chip state");
        block(state.data(), size);
        if (reading)
        {
            s.m_sid.restoreState(state.data(), static_cast<int>(size));
            s.m_sid.fastSilent(s.m_fastPcm && s.m_pcmDetected && !s.m_readbackUsed);
            s.m_sid.restoreFastSilentUsed(used);
            s.m_bufferpos = 0;
        }
    }
    void player(Player& p)
    {
        machine(p.m_c64);
        field(p.m_startTime);
        fields(p.m_trackerSkippedCycles, p.m_trackerSkippedCalls, p.m_trackerEntry,
               p.m_trackerSeekAllowed);
        for (auto* s : p.m_chips)
            chip(*static_cast<reSIDfpEmu*>(s));
    }
};
void Player::initTrackerSeek()
{
    m_trackerSkippedCycles = m_trackerSkippedCalls = 0;
    m_trackerSeekAllowed = true;
    m_trackerEntry = SeekStateAccess::trackerEntry(*this);
}
bool Player::enableTrackerSeek(const char* identity)
{
    return SeekStateAccess::enableTrackerSeek(*this, identity);
}
bool Player::seekTracker(unsigned targetMs)
{
    return SeekStateAccess::seekTracker(*this, targetMs);
}
std::vector<uint8_t> Player::saveState()
{
    if (!m_tune || m_chips.empty())
        return {};
    for (auto* chip : m_chips)
        if (!dynamic_cast<reSIDfpEmu*>(chip))
            return {};
    std::vector<uint8_t> state;
    SeekStateAccess a{state, 0, false};
    const Player* owner = this;
    auto generation = m_stateGeneration;
    a.fields(owner, generation);
    a.player(*this);
    return state;
}
bool Player::restoreState(const std::vector<uint8_t>& state)
{
    // Validate size and lifetime before touching the machine or event queue.
    if (state.size() != saveState().size() ||
        state.size() < sizeof(this) + sizeof(m_stateGeneration))
        return false;
    const Player* owner = nullptr;
    uint64_t generation = 0;
    std::memcpy(&owner, state.data(), sizeof(owner));
    std::memcpy(&generation, state.data() + sizeof(owner), sizeof(generation));
    if (owner != this || generation != m_stateGeneration)
        return false;
    auto copy = state;
    SeekStateAccess a{copy, sizeof(owner) + sizeof(generation), true};
    a.player(*this);
    return a.offset == state.size();
}
} // namespace libsidplayfp
#endif
