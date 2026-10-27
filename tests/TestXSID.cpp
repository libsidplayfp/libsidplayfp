// Legacy PlaySID register sequencing, independent of SID analogue filtering.
#include "xsid/xsid.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace libsidplayfp;
static void require(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
struct Harness final : XSID
{
    std::array<uint8_t, 65536> ram{};
    std::vector<uint8_t> writes;
    explicit Harness(EventScheduler& scheduler) : XSID(&scheduler) {}
    uint8_t readMemByte(uint_least16_t address) override
    {
        return ram[address];
    }
    void writeMemByte(uint8_t value) override
    {
        writes.push_back(value);
    }
};
struct Tick final : Event
{
    EventScheduler& scheduler;
    explicit Tick(EventScheduler& s) : Event("test clock"), scheduler(s)
    {
        s.schedule(*this, 1);
    }
    void event() override
    {
        scheduler.schedule(*this, 1);
    }
};
static void sample(Harness& h, unsigned channel, unsigned scale = 0, unsigned order = 0)
{
    auto write = [&](unsigned reg, uint8_t value) { h.write(channel + reg, value); };
    write(0x1e, 0);
    write(0x1f, 0x20);
    write(0x3d, 8);
    write(0x3e, 0x20);
    write(0x3f, 0);
    write(0x5d, 10);
    write(0x5e, 0);
    write(0x5f, scale);
    write(0x7d, order);
    write(0x7e, 0);
    write(0x7f, 0x20);
    write(0x1d, 0xff);
}
int main()
{
    try
    {
        EventScheduler scheduler;
        Harness h(scheduler);
        Tick tick(scheduler);
        for (unsigned i = 0; i < 8; ++i)
            h.ram[0x2000 + i] = static_cast<uint8_t>(0x10 + i * 0x22);
        h.storeSidData0x18(8);
        h.writes.clear();
        sample(h, 0);
        for (unsigned i = 0; i < 500; ++i)
            scheduler.clock();
        require(h.writes.size() >= 16, "Huelsbeck samples advance");
        for (unsigned i = 0; i < 16; ++i)
            require(h.writes[i] == i, "low/high nibble order and RAM reads");
        h.reset();
        h.storeSidData0x18(8);
        h.writes.clear();
        sample(h, 0, 0, 1);
        for (unsigned i = 0; i < 500; ++i)
            scheduler.clock();
        require(h.writes.size() >= 16 && h.writes[0] == 1 && h.writes[1] == 0,
                "reverse nibble order");
        h.reset();
        h.storeSidData0x18(8);
        h.writes.clear();
        sample(h, 0x100);
        for (unsigned i = 0; i < 500; ++i)
            scheduler.clock();
        require(h.writes.size() >= 16, "second sample channel");
        h.reset();
        h.writes.clear();
        sample(h, 0, 255);
        for (unsigned i = 0; i < 500; ++i)
            scheduler.clock();
        require(h.writes.empty(), "invalid scale rejected");
        h.reset();
        h.storeSidData0x18(8);
        h.writes.clear();
        h.write(0x1e, 0);
        h.write(0x1f, 0x20);
        h.write(0x3d, 3);
        h.write(0x3e, 1);
        h.write(0x3f, 1);
        h.write(0x5d, 1);
        h.write(0x1d, 2);
        for (unsigned i = 0; i < 1000; ++i)
            scheduler.clock();
        require(h.writes.size() > 3 && *std::min_element(h.writes.begin(), h.writes.end()) !=
                                           *std::max_element(h.writes.begin(), h.writes.end()),
                "Galway tone/noise sequence");
        h.reset();
        h.mute(true);
        h.writes.clear();
        sample(h, 0);
        for (unsigned i = 0; i < 500; ++i)
            scheduler.clock();
        require(h.writes.empty(), "muted samples continue without output");
        puts("PASS: XSID nibble orders, RAM reads, both channels, invalid scale, Galway and mute");
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
