/**
 * Save and restore the playback state of a player instance.
 */
#ifndef SEEK_STATE_H
#define SEEK_STATE_H
#include "builders/residfp-builder/residfp-emu.h"
#include <cstring>
#include <stdexcept>
#include <type_traits>
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
        for (auto* s : p.m_chips)
            chip(*static_cast<reSIDfpEmu*>(s));
    }
};
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
