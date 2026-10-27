/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2026 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mos6510.h"
#include "call.h"
#include "sidplayfp/sidplayfp.h"

#include "Event.h"
#include "sidendian.h"

#include "opcodes.h"

#include <cstdio>
#include "mos6510debug.h"

#include "sidcxx11.h"

#include <iomanip>
#include <sstream>


namespace libsidplayfp
{

/**
 * Magic values for lxa and ane undocumented instructions.
 * Magic may be EE, EF, FE or FF, but most emulators seem to use EE.
 * The constants here defined are based on VICE testsuite which
 * refers to some real case usage of the opcodes.
 */
constexpr uint8_t lxa_magic = 0xee;
constexpr uint8_t ane_magic = 0xef;

constexpr int interruptDelay = 2;

//-------------------------------------------------------------------------//

template<void(MOS6510::*Func)()>
void StaticFuncWrapper(MOS6510& self)
{
    (self.*Func)();
}

/**
 * When AEC signal is high, no stealing is possible.
 */
void MOS6510::eventWithoutSteals()
{
    if (m_pcmActive)
    {
        if (m_playerCallActive)
            playerCallCycle();
        else
            nativePcmCycle();
        return;
    }
    do
    {
        const ProcessorCycle& instr = instrTable[cycleCount++];
        (instr.func)(*this);
        if (m_pcmActive)
        {
            nativePcmSchedule();
            return;
        }
        if (m_idleActive)
            return;
    } while (rdy && eventScheduler.clockCPU());
    eventScheduler.schedule(m_nosteal, 1);
}

/**
 * When AEC signal is low, steals permitted.
 */
void MOS6510::eventWithSteals()
{
    if (instrTable[cycleCount].nosteal)
    {
        const ProcessorCycle &instr = instrTable[cycleCount++];
        (instr.func)(*this);
        eventScheduler.schedule(m_steal, 1);
    }
    else
    {
        switch (cycleCount)
        {
        case (CLIn << 3):
            flags.setI(false);
            if (irqAssertedOnPin && (interruptCycle == MAX))
                interruptCycle = -MAX;
            break;
        case (SEIn << 3):
            flags.setI(true);
            if (!rstFlag && !nmiFlag && (cycleCount <= interruptCycle + interruptDelay))
                interruptCycle = MAX;
            break;
        case (SHAiy << 3) + 3:
        case (SHSay << 3) + 2:
        case (SHYax << 3) + 2:
        case (SHXay << 3) + 2:
        case (SHAay << 3) + 2:
            // Save rdy state for SH* instructions
            rdyOnThrowAwayRead = true;
            break;
        default:
            break;
        }

        // Even while stalled, the CPU can still process first clock of
        // interrupt delay, but only the first one.
        if (interruptCycle == cycleCount)
        {
            interruptCycle--;
        }
    }
}

void MOS6510::removeIRQ()
{
    if (!rstFlag && !nmiFlag && (interruptCycle != MAX))
        interruptCycle = MAX;
}

/**
 * Handle bus access signals. When RDY line is asserted, the CPU
 * will pause when executing the next read operation.
 *
 * @param newRDY new state for RDY signal
 */
void MOS6510::setRDY(bool newRDY)
{
    if (m_pcmActive)
    {
        nativePcmSync();
        rdy = newRDY;
        nativePcmSchedule();
        return;
    }
    if (m_idleActive)
    {
        idleSync();
        rdy = newRDY;
        return;
    }
    rdy = newRDY;

    if (rdy)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
    }
    else
    {
        eventScheduler.cancel(m_nosteal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}


/**
 * Push P on stack, decrement S.
 */
void MOS6510::PushSR()
{
    // Set the B flag, 0 for hardware interrupts
    // and 1 for BRK and PHP. Bit 5 is always 1
    // https://wiki.nesdev.org/w/index.php?title=Status_flags#The_B_flag
    Push(flags.get() | (d1x1 ? 0x20 : 0x30));
}

/**
 * increment S, Pop P off stack.
 */
void MOS6510::PopSR()
{
    flags.set(Pop());
    calculateInterruptTriggerCycle();
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Interrupt Routines                                                      //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

/**
 * This forces the CPU to abort whatever it is doing and immediately
 * enter the RST interrupt handling sequence. The implementation is
 * not compatible: instructions actually get aborted mid-execution.
 * However, there is no possible way to trigger this signal from
 * programs, so it's OK.
 */
void MOS6510::triggerRST()
{
    Initialise();
    cycleCount = BRKn << 3;
    rstFlag = true;
    calculateInterruptTriggerCycle();
}

/**
 * Trigger NMI interrupt on the CPU. Calling this method
 * flags that CPU must enter the NMI routine at earliest
 * opportunity. There is no way to cancel NMI request once
 * given.
 */
void MOS6510::triggerNMI()
{
    if (m_idleActive)
        idleWake();
    if (m_pcmActive)
    {
        if (m_playerCallActive)
            m_playerCallAllowed = false;
        throw sidplayfp::SeekReplayRequired();
    }
    nmiFlag = true;
    calculateInterruptTriggerCycle();

    /* maybe process 1 clock of interrupt delay. */
    if (!rdy)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/**
 * Pull IRQ line low on CPU.
 */
void MOS6510::triggerIRQ()
{
    if (m_idleActive)
        idleWake();
    irqAssertedOnPin = true;
    calculateInterruptTriggerCycle();

    /* maybe process 1 clock of interrupt delay. */
    if (!rdy && interruptCycle == cycleCount)
    {
        eventScheduler.cancel(m_steal);
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/**
 * Inform CPU that IRQ is no longer pulled low.
 */
void MOS6510::clearIRQ()
{
    irqAssertedOnPin = false;
    eventScheduler.schedule(clearInt, interruptDelay, EVENT_CLOCK_PHI1);
}

void MOS6510::interruptsAndNextOpcode()
{
    if (cycleCount > interruptCycle + interruptDelay)
    {
        if (cpu_debug) UNLIKELY
        {
            const event_clock_t cycles = eventScheduler.getTime(EVENT_CLOCK_PHI2);
            MOS6510Debug::DumpState(cycles, *this);
            fprintf(cpu_debug->m_fdbg, "****************************************************\n");
            fprintf(cpu_debug->m_fdbg, " interrupt (%d)\n", static_cast<int>(cycles));
            fprintf(cpu_debug->m_fdbg, "****************************************************\n");

            cpu_debug->instrStartPC = -1;
        }

        // Recognised PCM NMI entry: SEI; JMP ($0318), reached via $fffa.
        if (m_pcmEnabled && m_pcmCached && nmiFlag && !rstFlag)
        {
            const uint16_t entry = cpuRead(0xfffa) | (cpuRead(0xfffb) << 8);
            const uint16_t handler = cpuRead(0x318) | (cpuRead(0x319) << 8);
            if ((handler == m_pcmLow || handler == m_pcmHigh) && entry >= 0xe000 &&
                entry < 0xfffc && cpuRead(entry) == 0x78 && cpuRead(entry + 1) == 0x6c &&
                cpuRead(entry + 2) == 0x18 && cpuRead(entry + 3) == 0x03)
            {
                const auto sp = Register_StackPointer;
                const auto sr = flags.get();
                Register_StackPointer = static_cast<uint8_t>(sp - 3);
                flags.setI(true);
                if (beginNativePcm(handler))
                {
                    cpuWrite(0x100 | sp, Register_ProgramCounter >> 8);
                    cpuWrite(0x100 | static_cast<uint8_t>(sp - 1),
                             static_cast<uint8_t>(Register_ProgramCounter));
                    cpuWrite(0x100 | static_cast<uint8_t>(sp - 2), sr | 0x20);
                    for (unsigned i = 0; i < m_pcmActionCount; ++i)
                        m_pcmActions[i].cycle += 14;
                    for (int i = 95; i >= 0; --i)
                        m_pcmWriteCycle[i] = i >= 14 && m_pcmWriteCycle[i - 14];
                    m_pcmWriteCycle[2] = m_pcmWriteCycle[3] = m_pcmWriteCycle[4] = true;
                    nmiFlag = false;
                    d1x1 = false;
                    interruptCycle = MAX;
                    return;
                }
                Register_StackPointer = sp;
                flags.set(sr);
            }
        }
        cpuRead(Register_ProgramCounter);
        cycleCount = BRKn << 3;
        d1x1 = true;
        interruptCycle = MAX;
    }
    else
    {
        fetchNextOpcode();
    }
}

void MOS6510::fetchNextOpcode()
{
    if (cpu_debug) UNLIKELY
    {
        MOS6510Debug::DumpState(eventScheduler.getTime(EVENT_CLOCK_PHI2), *this);

        cpu_debug->instrStartPC = Register_ProgramCounter;
    }

    rdyOnThrowAwayRead = false;

    cycleCount = cpuRead(Register_ProgramCounter) << 3;
    Register_ProgramCounter++;

    // A self-jump in RAM/ROM has no side effects between hardware events.
    // Exclude the processor port, I/O and operands crossing either boundary.
    const uint_least16_t pc = Register_ProgramCounter - 1;
    if (cycleCount == (JMPw << 3) && rdy &&
        ((pc > 1 && pc < 0xcffe) || (pc >= 0xe000 && pc < 0xfffe)) &&
        cpuRead(pc + 1) == (pc & 0xff) && cpuRead(pc + 2) == (pc >> 8) && !checkInterrupts())
    {
        if (m_pcmEnabled || m_loopEnabled)
        {
            m_idleActive = true;
            m_idlePC = pc;
            m_idlePhase = 0;
            m_idleClock = eventScheduler.halfTime();
        }
        else
            eventScheduler.skipCPULoops(3);
    }

    if (m_loopEnabled && (cycleCount >> 3) == 0x4c && pc == 0x1003 && beginPlayerCall(pc))
        return;

    if (!checkInterrupts())
    {
        interruptCycle = MAX;
    }
    else if (interruptCycle != MAX)
    {
        interruptCycle = -MAX;
    }

    if (m_pcmEnabled && cycleCount == (PHAn << 3) && flags.getI())
        if (!beginDigiMixer(pc))
            beginNativePcm(pc);
}

/**
 * Evaluate when to execute an interrupt. Calling this method can also
 * result in the decision that no interrupt at all needs to be scheduled.
 */
void MOS6510::calculateInterruptTriggerCycle()
{
    /* Interrupt cycle not going to trigger? */
    if (interruptCycle == MAX)
    {
        if (checkInterrupts())
        {
            interruptCycle = cycleCount;
        }
    }
}

void MOS6510::IRQLoRequest()
{
    endian_16lo8(Register_ProgramCounter, cpuRead(Cycle_EffectiveAddress));
    d1x1 = false;
}

void MOS6510::IRQHiRequest()
{
    endian_16hi8(Register_ProgramCounter, cpuRead(Cycle_EffectiveAddress + 1));
    flags.setI(true);
}

/**
 * Read the next opcode byte from memory (and throw it away)
 */
void MOS6510::throwAwayFetch()
{
    cpuRead(Register_ProgramCounter);
}

/**
 * Issue throw-away read and fix address.
 * Some people use these to ACK CIA IRQs.
 */
void MOS6510::throwAwayRead()
{
    cpuRead(Cycle_EffectiveAddress);
    if (adl_carry)
        Cycle_EffectiveAddress += 0x100;
}

/**
 * Fetch value, increment PC.
 *
 * Addressing Modes:
 * - Immediate
 * - Relative
 */
void MOS6510::FetchDataByte()
{
    Cycle_Data = cpuRead(Register_ProgramCounter);
    if (!d1x1)
    {
        Register_ProgramCounter++;
    }

    if (cpu_debug) UNLIKELY
    {
        cpu_debug->instrOperand = Cycle_Data;
    }
}

/**
 * Fetch low address byte, increment PC.
 *
 * Addressing Modes:
 * - Stack Manipulation
 * - Absolute
 * - Zero Page
 * - Zero Page Indexed
 * - Absolute Indexed
 * - Absolute Indirect
 */
void MOS6510::FetchLowAddr()
{
    Cycle_EffectiveAddress = cpuRead(Register_ProgramCounter);
    Register_ProgramCounter++;

    if (cpu_debug) UNLIKELY
    {
        cpu_debug->instrOperand = Cycle_EffectiveAddress;
    }
}

/**
 * Read from address, add index register X to it.
 *
 * Addressing Modes:
 * - Zero Page Indexed
 */
void MOS6510::FetchLowAddrX()
{
    FetchLowAddr();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_X) & 0xff;
}

/**
 * Read from address, add index register Y to it.
 *
 * Addressing Modes:
 * - Zero Page Indexed
 */
void MOS6510::FetchLowAddrY()
{
    FetchLowAddr();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_Y) & 0xff;
}

/**
 * Fetch high address byte, increment PC (Absolute Addressing).
 * Low byte must have been obtained first!
 *
 * Addressing Modes:
 * - Absolute
 */
void MOS6510::FetchHighAddr()
{   // Get the high byte of an address from memory
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(Register_ProgramCounter));
    Register_ProgramCounter++;

    if (cpu_debug) UNLIKELY
    {
        endian_16hi8(cpu_debug->instrOperand, endian_16hi8(Cycle_EffectiveAddress));
    }
}

/**
 * Fetch high byte of address, add index register X to low address byte,
 * increment PC.
 *
 * Addressing Modes:
 * - Absolute Indexed
 */
void MOS6510::FetchHighAddrX()
{
    Cycle_EffectiveAddress += Register_X;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighAddr();
}

/**
 * Same as #FetchHighAddrX except doesn't worry about page crossing.
 */
void MOS6510::FetchHighAddrX2()
{
    FetchHighAddrX();
    if (!adl_carry)
        cycleCount++;
}

/**
 * Fetch high byte of address, add index register Y to low address byte,
 * increment PC.
 *
 * Addressing Modes:
 * - Absolute Indexed
 */
void MOS6510::FetchHighAddrY()
{
    Cycle_EffectiveAddress += Register_Y;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighAddr();
}

/**
 * Same as #FetchHighAddrY except doesn't worry about page crossing.
 */
void MOS6510::FetchHighAddrY2()
{
    FetchHighAddrY();
    if (!adl_carry)
        cycleCount++;
}

/**
 * Fetch pointer address low, increment PC.
 *
 * Addressing Modes:
 * - Absolute Indirect
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchLowPointer()
{
    Cycle_Pointer = cpuRead(Register_ProgramCounter);
    Register_ProgramCounter++;

    if (cpu_debug) UNLIKELY
    {
        cpu_debug->instrOperand = Cycle_Pointer;
    }
}

/**
 * Add X to it.
 *
 * Addressing Modes:
 * - Indexed Indirect (pre X)
 */
void MOS6510::FetchLowPointerX()
{
    endian_16lo8(Cycle_Pointer, (Cycle_Pointer + Register_X) & 0xff);
}

/**
 * Fetch pointer address high, increment PC.
 *
 * Addressing Modes:
 * - Absolute Indirect
 */
void MOS6510::FetchHighPointer()
{
    endian_16hi8(Cycle_Pointer, cpuRead(Register_ProgramCounter));
    Register_ProgramCounter++;

    if (cpu_debug) UNLIKELY
    {
        endian_16hi8(cpu_debug->instrOperand, endian_16hi8(Cycle_Pointer));
    }
}

/**
 * Fetch effective address low.
 *
 * Addressing Modes:
 * - Indirect
 * - Indexed Indirect (pre X)
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchLowEffAddr()
{
    Cycle_EffectiveAddress = cpuRead(Cycle_Pointer);
}

/**
 * Fetch effective address high.
 *
 * Addressing Modes:
 * - Indirect
 * - Indexed Indirect (pre X)
 */
void MOS6510::FetchHighEffAddr()
{
    endian_16lo8(Cycle_Pointer, (Cycle_Pointer + 1) & 0xff);
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(Cycle_Pointer));
}

/**
 * Fetch effective address high, add Y to low byte of effective address.
 *
 * Addressing Modes:
 * - Indirect indexed (post Y)
 */
void MOS6510::FetchHighEffAddrY()
{
    Cycle_EffectiveAddress += Register_Y;
    adl_carry = Cycle_EffectiveAddress > 0xff;
    FetchHighEffAddr();
}


/**
 * Same as #FetchHighEffAddrY except doesn't worry about page crossing.
 */
void MOS6510::FetchHighEffAddrY2()
{
    FetchHighEffAddrY();
    if (!adl_carry)
        cycleCount++;
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Data Accessing Routines                                          //
// Data Accessing operations as described in 64doc by John West and        //
// Marko Makela                                                            //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::FetchEffAddrDataByte()
{
    Cycle_Data = cpuRead(Cycle_EffectiveAddress);
}

/**
 * Write Cycle_Data to effective address.
 */
void MOS6510::PutEffAddrDataByte()
{
    cpuWrite(Cycle_EffectiveAddress, Cycle_Data);
}

/**
 * Push data on the stack
 */
void MOS6510::Push(uint8_t data)
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    cpuWrite(addr, data);
    Register_StackPointer--;
}

/**
 * Pop data from the stack
 */
uint8_t MOS6510::Pop()
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    return cpuRead(addr);
}

/**
 * Push Program Counter Low Byte on stack, decrement S.
 */
void MOS6510::PushLowPC()
{
    Push(endian_16lo8(Register_ProgramCounter));
}

/**
 * Push Program Counter High Byte on stack, decrement S.
 */
void MOS6510::PushHighPC()
{
    Push(endian_16hi8(Register_ProgramCounter));
}

/**
 * Increment stack and pull program counter low byte from stack.
 */
void MOS6510::PopLowPC()
{
    endian_16lo8(Cycle_EffectiveAddress, Pop());
}

/**
 * Increment stack and pull program counter high byte from stack.
 */
void MOS6510::PopHighPC()
{
    endian_16hi8(Cycle_EffectiveAddress, Pop());
}

void MOS6510::WasteCycle() {}

void MOS6510::brkPushLowPC()
{
    PushLowPC();

    if (rstFlag)
    {
        /* rst = %10x */
        Cycle_EffectiveAddress = 0xfffc;
    }
    else if (nmiFlag)
    {
        /* nmi = %01x */
        Cycle_EffectiveAddress = 0xfffa;
    }
    else
    {
        /* irq = %11x */
        Cycle_EffectiveAddress = 0xfffe;
    }

    rstFlag = false;
    nmiFlag = false;
    calculateInterruptTriggerCycle();
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Opcodes                                              //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::cld_instr()
{
    flags.setD(false);
    interruptsAndNextOpcode();
}

void MOS6510::cli_instr()
{
    flags.setI(false);
    calculateInterruptTriggerCycle();
    interruptsAndNextOpcode();
}

void MOS6510::jmp_instr()
{
    Register_ProgramCounter = Cycle_EffectiveAddress;

    interruptsAndNextOpcode();
}

void MOS6510::pha_instr()
{
    Push(Register_Accumulator);
}

/**
 * RTI does not delay the IRQ I flag change as it is set 3 cycles before
 * the end of the opcode, and thus the 6510 has enough time to call the
 * interrupt routine as soon as the opcode ends, if necessary.
 */
void MOS6510::rti_instr()
{
    Register_ProgramCounter = Cycle_EffectiveAddress;
    interruptsAndNextOpcode();

    if (cpu_debug) UNLIKELY
    {
        fprintf(cpu_debug->m_fdbg, "****************************************************\n\n");
    }
}

void MOS6510::rts_instr()
{
    cpuRead(Cycle_EffectiveAddress);
    Register_ProgramCounter = Cycle_EffectiveAddress;
    Register_ProgramCounter++;
}

void MOS6510::sed_instr()
{
    flags.setD(true);
    interruptsAndNextOpcode();
}

void MOS6510::sei_instr()
{
    flags.setI(true);
    interruptsAndNextOpcode();
    if (!rstFlag && !nmiFlag && interruptCycle != MAX)
        interruptCycle = MAX;
}

void MOS6510::sta_instr()
{
    Cycle_Data = Register_Accumulator;
    PutEffAddrDataByte();
}

void MOS6510::stx_instr()
{
    Cycle_Data = Register_X;
    PutEffAddrDataByte();
}

void MOS6510::sty_instr()
{
    Cycle_Data = Register_Y;
    PutEffAddrDataByte();
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Undocumented Opcodes                                 //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

#if 0
// Not required - Operation performed By another method
//
// Undocumented - HLT crashes the microprocessor.  When this opcode is executed, program
// execution ceases.  No hardware interrupts will execute either.  The author
// has characterized this instruction as a halt instruction since this is the
// most straightforward explanation for this opcode's behaviour.  Only a reset
// will restart execution.  This opcode leaves no trace of any operation
// performed!  No registers affected.
void MOS6510::hlt_instr() {}
#endif

/**
 * Perform the SH* instructions.
 */
void MOS6510::sh_instr()
{
    uint8_t tmp = endian_16hi8(Cycle_EffectiveAddress);

    /*
     * When the addressing/indexing causes a page boundary crossing
     * the highbyte of the target address is ANDed with the value stored.
     */
    if (adl_carry)
    {
        endian_16hi8(Cycle_EffectiveAddress, tmp & Cycle_Data);
    }
    else
        tmp++;

    /*
     * When a DMA is going on (the CPU is halted by the VIC-II)
     * while the instruction sha/shx/shy executes then the last
     * term of the ANDing (ADH+1) drops off.
     *
     * http://sourceforge.net/p/vice-emu/bugs/578/
     */
    if (!rdyOnThrowAwayRead)
    {
        Cycle_Data &= tmp;
    }

    PutEffAddrDataByte();
}

/**
 * Undocumented - This opcode stores the result of A AND X AND ADH+1 in memory.
 */
void MOS6510::axa_instr()
{
    Cycle_Data = Register_X & Register_Accumulator;
    sh_instr();
}

/**
 * Undocumented - This opcode ANDs the contents of the Y register with ADH+1 and stores the
 * result in memory.
 */
void MOS6510::say_instr()
{
    Cycle_Data = Register_Y;
    sh_instr();
}

/**
 * Undocumented - This opcode ANDs the contents of the X register with ADH+1 and stores the
 * result in memory.
 */
void MOS6510::xas_instr()
{
    Cycle_Data = Register_X;
    sh_instr();
}

/**
 * Undocumented - AXS ANDs the contents of the A and X registers (without changing the
 * contents of either register) and stores the result in memory.
 * AXS does not affect any flags in the processor status register.
 */
void MOS6510::axs_instr()
{
    Cycle_Data = Register_Accumulator & Register_X;
    PutEffAddrDataByte();
}


/**
 * BCD adding.
 */
void MOS6510::doADC()
{
    const unsigned int C      = flags.getC() ? 1 : 0;
    const unsigned int A      = Register_Accumulator;
    const unsigned int s      = Cycle_Data;
    const unsigned int regAC2 = A + s + C;

    if (flags.getD())
    {   // BCD mode
        unsigned int lo = (A & 0x0f) + (s & 0x0f) + C;
        unsigned int hi = (A & 0xf0) + (s & 0xf0);
        if (lo > 0x09)
            lo += 0x06;
        if (lo > 0x0f)
            hi += 0x10;

        flags.setZ(!(regAC2 & 0xff));
        flags.setN(hi & 0x80);
        flags.setV(((hi ^ A) & 0x80) && !((A ^ s) & 0x80));
        if (hi > 0x90)
            hi += 0x60;

        flags.setC(hi > 0xff);
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        flags.setC(regAC2 > 0xff);
        flags.setV(((regAC2 ^ A) & 0x80) && !((A ^ s) & 0x80));
        flags.setNZ(Register_Accumulator = regAC2 & 0xff);
    }
}

/**
 * BCD subtracting.
 */
void MOS6510::doSBC()
{
    const unsigned int C      = flags.getC() ? 0 : 1;
    const unsigned int A      = Register_Accumulator;
    const unsigned int s      = Cycle_Data;
    const unsigned int regAC2 = A - s - C;

    flags.setC(regAC2 < 0x100);
    flags.setV(((regAC2 ^ A) & 0x80) && ((A ^ s) & 0x80));
    flags.setNZ(regAC2);

    if (flags.getD())
    {   // BCD mode
        unsigned int lo = (A & 0x0f) - (s & 0x0f) - C;
        unsigned int hi = (A & 0xf0) - (s & 0xf0);
        if (lo & 0x10)
        {
             lo -= 0x06;
             hi -= 0x10;
        }
        if (hi & 0x100)
            hi -= 0x60;
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        Register_Accumulator = regAC2 & 0xff;
    }
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Addressing Routines                                 //
//-------------------------------------------------------------------------//


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Opcodes                                             //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::adc_instr()
{
    doADC();
    interruptsAndNextOpcode();
}

void MOS6510::and_instr()
{
    flags.setNZ(Register_Accumulator &= Cycle_Data);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - For a detailed explanation of this opcode look at:
 * http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
 */
void MOS6510::ane_instr()
{
    flags.setNZ(Register_Accumulator = (Register_Accumulator | ane_magic) & Register_X & Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::asl_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x80);
    flags.setNZ(Cycle_Data <<= 1);
}

void MOS6510::asla_instr()
{
    flags.setC(Register_Accumulator & 0x80);
    flags.setNZ(Register_Accumulator <<= 1);
    interruptsAndNextOpcode();
}

void MOS6510::fix_branch()
{
    // Throw away read
    cpuRead(Cycle_EffectiveAddress);

    // Fix address
    Register_ProgramCounter += Cycle_Data < 0x80 ? 0x0100 : 0xff00;
}

void MOS6510::branch_instr(bool condition)
{
    // 2 cycles spent before arriving here. spend 0 - 2 cycles here;
    // - condition false: Continue immediately to FetchNextInstr.
    //
    // Otherwise read the byte following the opcode (which is already scheduled to occur on this cycle).
    // This effort is wasted. Then calculate address of the branch target. If branch is on same page,
    // then continue at that insn on next cycle (this delays IRQs by 1 clock for some reason, allegedly).
    //
    // If the branch is on different memory page, issue a spurious read with wrong high byte before
    // continuing at the correct address.
    if (condition)
    {
        // issue the spurious read for next insn here.
        cpuRead(Register_ProgramCounter);

        Cycle_EffectiveAddress = endian_16lo8(Register_ProgramCounter);
        Cycle_EffectiveAddress += Cycle_Data;
        adl_carry = (Cycle_EffectiveAddress > 0xff) != (Cycle_Data > 0x7f);
        endian_16hi8(Cycle_EffectiveAddress, endian_16hi8(Register_ProgramCounter));

        Register_ProgramCounter = Cycle_EffectiveAddress;

        // Check for page boundary crossing
        if (!adl_carry)
        {
            // Skip next throw away read
            cycleCount++;

            // Hack: delay the interrupt past this instruction.
            if (interruptCycle >> 3 == cycleCount >> 3)
                interruptCycle += 2;
        }
    }
    else
    {
        // branch not taken: skip the following spurious read insn and go to FetchNextInstr immediately.
        interruptsAndNextOpcode();
    }
}

void MOS6510::bcc_instr()
{
    branch_instr(!flags.getC());
}

void MOS6510::bcs_instr()
{
    branch_instr(flags.getC());
}

void MOS6510::beq_instr()
{
    branch_instr(flags.getZ());
}

void MOS6510::bit_instr()
{
    flags.setZ((Register_Accumulator & Cycle_Data) == 0);
    flags.setN(Cycle_Data & 0x80);
    flags.setV(Cycle_Data & 0x40);
    interruptsAndNextOpcode();
}

void MOS6510::bmi_instr()
{
    branch_instr(flags.getN());
}

void MOS6510::bne_instr()
{
    branch_instr(!flags.getZ());
}

void MOS6510::bpl_instr()
{
    branch_instr(!flags.getN());
}

void MOS6510::bvc_instr()
{
    branch_instr(!flags.getV());
}

void MOS6510::bvs_instr()
{
    branch_instr(flags.getV());
}

void MOS6510::clc_instr()
{
    flags.setC(false);
    interruptsAndNextOpcode();
}

void MOS6510::clv_instr()
{
    flags.setV(false);
    interruptsAndNextOpcode();
}

void MOS6510::compare(uint8_t data)
{
    const uint_least16_t tmp = static_cast<uint_least16_t>(data) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x100);
    interruptsAndNextOpcode();
}

void MOS6510::cmp_instr()
{
    compare(Register_Accumulator);
}

void MOS6510::cpx_instr()
{
    compare(Register_X);
}

void MOS6510::cpy_instr()
{
    compare(Register_Y);
}

void MOS6510::dec_instr()
{
    PutEffAddrDataByte();
    flags.setNZ(--Cycle_Data);
}

void MOS6510::dex_instr()
{
    flags.setNZ(--Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::dey_instr()
{
    flags.setNZ(--Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::eor_instr()
{
    flags.setNZ(Register_Accumulator ^= Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::inc_instr()
{
    PutEffAddrDataByte();
    flags.setNZ(++Cycle_Data);
}

void MOS6510::inx_instr()
{
    flags.setNZ(++Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::iny_instr()
{
    flags.setNZ(++Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::lda_instr()
{
    flags.setNZ(Register_Accumulator = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::ldx_instr()
{
    flags.setNZ(Register_X = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::ldy_instr()
{
    flags.setNZ(Register_Y = Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::lsr_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x01);
    flags.setNZ(Cycle_Data >>= 1);
}

void MOS6510::lsra_instr()
{
    flags.setC(Register_Accumulator & 0x01);
    flags.setNZ(Register_Accumulator >>= 1);
    interruptsAndNextOpcode();
}

void MOS6510::ora_instr()
{
    flags.setNZ(Register_Accumulator |= Cycle_Data);
    interruptsAndNextOpcode();
}

void MOS6510::pla_instr()
{
    flags.setNZ(Register_Accumulator = Pop());
}

void MOS6510::rol_instr()
{
    const uint8_t newC = Cycle_Data & 0x80;
    PutEffAddrDataByte();
    Cycle_Data <<= 1;
    if (flags.getC())
        Cycle_Data |= 0x01;
    flags.setNZ(Cycle_Data);
    flags.setC(newC);
}

void MOS6510::rola_instr()
{
    const uint8_t newC = Register_Accumulator & 0x80;
    Register_Accumulator <<= 1;
    if (flags.getC())
        Register_Accumulator |= 0x01;
    flags.setNZ(Register_Accumulator);
    flags.setC(newC);
    interruptsAndNextOpcode();
}

void MOS6510::ror_instr()
{
    const uint8_t newC = Cycle_Data & 0x01;
    PutEffAddrDataByte();
    Cycle_Data >>= 1;
    if (flags.getC())
        Cycle_Data |= 0x80;
    flags.setNZ(Cycle_Data);
    flags.setC(newC);
}

void MOS6510::rora_instr()
{
    const uint8_t newC = Register_Accumulator & 0x01;
    Register_Accumulator >>= 1;
    if (flags.getC())
        Register_Accumulator |= 0x80;
    flags.setNZ(Register_Accumulator);
    flags.setC(newC);
    interruptsAndNextOpcode();
}

void MOS6510::sbx_instr()
{
    const unsigned int tmp = (Register_X & Register_Accumulator) - Cycle_Data;
    flags.setNZ(Register_X = tmp & 0xff);
    flags.setC(tmp < 0x100);
    interruptsAndNextOpcode();
}

void MOS6510::sbc_instr()
{
    doSBC();
    interruptsAndNextOpcode();
}

void MOS6510::sec_instr()
{
    flags.setC(true);
    interruptsAndNextOpcode();
}

void MOS6510::shs_instr()
{
    Register_StackPointer = Register_Accumulator & Register_X;
    Cycle_Data = Register_StackPointer;
    sh_instr();
}

void MOS6510::tax_instr()
{
    flags.setNZ(Register_X = Register_Accumulator);
    interruptsAndNextOpcode();
}

void MOS6510::tay_instr()
{
    flags.setNZ(Register_Y = Register_Accumulator);
    interruptsAndNextOpcode();
}

void MOS6510::tsx_instr()
{
    flags.setNZ(Register_X = Register_StackPointer);
    interruptsAndNextOpcode();
}

void MOS6510::txa_instr()
{
    flags.setNZ(Register_Accumulator = Register_X);
    interruptsAndNextOpcode();
}

void MOS6510::txs_instr()
{
    Register_StackPointer = Register_X;
    interruptsAndNextOpcode();
}

void MOS6510::tya_instr()
{
    flags.setNZ(Register_Accumulator = Register_Y);
    interruptsAndNextOpcode();
}

void MOS6510::invalidOpcode()
{
    std::ostringstream msg;
    msg << std::setfill('0')
        << "Illegal instruction $"
        << std::setw(2) << std::hex << (cycleCount >> 3)
        << " at address $"
        << std::setw(4) << (Register_ProgramCounter-1);
    throw haltInstruction(msg.str());
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Undocumented Opcodes                                //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

/**
 * Undocumented - This opcode ANDs the contents of the A register with an immediate value and
 * then LSRs the result.
 */
void MOS6510::alr_instr()
{
    Register_Accumulator &= Cycle_Data;
    flags.setC(Register_Accumulator & 0x01);
    flags.setNZ(Register_Accumulator >>= 1);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - ANC ANDs the contents of the A register with an immediate value and then
 * moves bit 7 of A into the Carry flag.  This opcode works basically
 * identically to AND #immed. except that the Carry flag is set to the same
 * state that the Negative flag is set to.
 */
void MOS6510::anc_instr()
{
    flags.setNZ(Register_Accumulator &= Cycle_Data);
    flags.setC(flags.getN());
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode ANDs the contents of the A register with an immediate value and
 * then RORs the result. (Implementation based on that of Frodo C64 Emulator)
 */
void MOS6510::arr_instr()
{
    const uint8_t data = Cycle_Data & Register_Accumulator;
    Register_Accumulator = data >> 1;
    if (flags.getC())
        Register_Accumulator |= 0x80;

    if (flags.getD())
    {
        flags.setN(flags.getC());
        flags.setZ(Register_Accumulator == 0);
        flags.setV((data ^ Register_Accumulator) & 0x40);

        if ((data & 0x0f) + (data & 0x01) > 5)
            Register_Accumulator  = (Register_Accumulator & 0xf0) | ((Register_Accumulator + 6) & 0x0f);
        flags.setC(((data + (data & 0x10)) & 0x1f0) > 0x50);
        if (flags.getC())
            Register_Accumulator += 0x60;
    }
    else
    {
        flags.setNZ(Register_Accumulator);
        flags.setC(Register_Accumulator & 0x40);
        flags.setV((Register_Accumulator & 0x40) ^ ((Register_Accumulator & 0x20) << 1));
    }
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode ASLs the contents of a memory location and then ORs the result
 * with the accumulator.
 */
void MOS6510::aso_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x80);
    Cycle_Data <<= 1;
    flags.setNZ(Register_Accumulator |= Cycle_Data);
}

/**
 * Undocumented - This opcode DECs the contents of a memory location and then CMPs the result
 * with the A register.
 */
void MOS6510::dcm_instr()
{
    PutEffAddrDataByte();
    Cycle_Data--;
    const uint_least16_t tmp = static_cast<uint_least16_t>(Register_Accumulator) - Cycle_Data;
    flags.setNZ(tmp);
    flags.setC(tmp < 0x100);
}

/**
 * Undocumented - This opcode INCs the contents of a memory location and then SBCs the result
 * from the A register.
 */
void MOS6510::ins_instr()
{
    PutEffAddrDataByte();
    Cycle_Data++;
    doSBC();
}

/**
 * Undocumented - This opcode ANDs the contents of a memory location with the contents of the
 * stack pointer register and stores the result in the accumulator, the X
 * register, and the stack pointer. Affected flags: N Z.
 */
void MOS6510::las_instr()
{
    flags.setNZ(Cycle_Data &= Register_StackPointer);
    Register_Accumulator  = Cycle_Data;
    Register_X            = Cycle_Data;
    Register_StackPointer = Cycle_Data;
    interruptsAndNextOpcode();
}

/**
 * Undocumented - This opcode loads both the accumulator and the X register with the contents
 * of a memory location.
 */
void MOS6510::lax_instr()
{
    flags.setNZ(Register_Accumulator = Register_X = Cycle_Data);
    interruptsAndNextOpcode();
}

/**
 * Undocumented - LSE LSRs the contents of a memory location and then EORs the result with
 * the accumulator.
 */
void MOS6510::lse_instr()
{
    PutEffAddrDataByte();
    flags.setC(Cycle_Data & 0x01);
    Cycle_Data >>= 1;
    flags.setNZ(Register_Accumulator ^= Cycle_Data);
}

/**
 * Undocumented - This opcode ORs the A register with #xx (the "magic" value),
 * ANDs the result with an immediate value, and then stores the result in both A and X.
 */
void MOS6510::oal_instr()
{
    flags.setNZ(Register_X = (Register_Accumulator = (Cycle_Data & (Register_Accumulator | lxa_magic))));
    interruptsAndNextOpcode();
}

/**
 * Undocumented - RLA ROLs the contents of a memory location and then ANDs the result with
 * the accumulator.
 */
void MOS6510::rla_instr()
{
    const uint8_t newC = Cycle_Data & 0x80;
    PutEffAddrDataByte();
    Cycle_Data = Cycle_Data << 1;
    if (flags.getC())
        Cycle_Data |= 0x01;
    flags.setC(newC);
    flags.setNZ(Register_Accumulator &= Cycle_Data);
}

/**
 * Undocumented - RRA RORs the contents of a memory location and then ADCs the result with
 * the accumulator.
 */
void MOS6510::rra_instr()
{
    const uint8_t newC = Cycle_Data & 0x01;
    PutEffAddrDataByte();
    Cycle_Data >>= 1;
    if (flags.getC())
        Cycle_Data |= 0x80;
    flags.setC(newC);
    doADC();
}

//-------------------------------------------------------------------------//

/**
 * Create new CPU emu.
 *
 * @param context
 *            The Event Context
 */
MOS6510::MOS6510(EventScheduler &scheduler, CPUDataBus &bus) :
    eventScheduler(scheduler),
    dataBus(bus),
    cpu_debug(nullptr),
    m_nosteal("CPU-nosteal", *this),
    m_steal("CPU-steal", *this),
    clearInt("Remove IRQ", *this)
{
    buildInstructionTable();

    // Intialise Processor Registers
    Register_Accumulator   = 0;
    Register_X             = 0;
    Register_Y             = 0;

    Cycle_EffectiveAddress = 0;
    Cycle_Data             = 0;

    Initialise();
}

/**
 * Build up the processor instruction table.
 */
void MOS6510::buildInstructionTable()
{
    for (unsigned int i = 0; i < 0x100; i++)
    {
#ifndef NDEBUG
        printf("Building Command %u[%02x]... ", i, i);
#endif

        /*
         * So: what cycles are marked as stealable? Rules are:
         *
         * - CPU performs either read or write at every cycle. Reads are
         *   always stealable. Writes are rare.
         *
         * - Every instruction begins with a sequence of reads. Writes,
         *   if any, are at the end for most instructions.
         */

        unsigned int buildCycle = i << 3;

        enum class AccessMode { WRITE, READ };
        AccessMode access = AccessMode::WRITE;
        bool legalMode  = true;
        bool legalInstr = true;

        switch (i)
        {
        // Accumulator or Implied addressing
        case ASLn: case CLCn: case CLDn: case CLIn: case CLVn:  case DEXn:
        case DEYn: case INXn: case INYn: case LSRn: case NOPn_: case PHAn:
        case PHPn: case PLAn: case PLPn: case ROLn: case RORn:
        case SECn: case SEDn: case SEIn: case TAXn:  case TAYn:
        case TSXn: case TXAn: case TXSn: case TYAn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayFetch>;
            break;

        // Immediate and Relative Addressing Mode Handler
        case ADCb: case ANDb:  case ANCb_: case ANEb: case ASRb: case ARRb:
        case BCCr: case BCSr:  case BEQr:  case BMIr: case BNEr: case BPLr:
        case BRKn: case BVCr:  case BVSr:  case CMPb: case CPXb: case CPYb:
        case EORb: case LDAb:  case LDXb:  case LDYb: case LXAb: case NOPb_:
        case ORAb: case SBCb_: case SBXb:  case RTIn: case RTSn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchDataByte>;
            break;

        // Zero Page Addressing Mode Handler - Read & RMW
        case ADCz:  case ANDz: case BITz: case CMPz: case CPXz: case CPYz:
        case EORz:  case LAXz: case LDAz: case LDXz: case LDYz: case ORAz:
        case NOPz_: case SBCz:
        case ASLz: case DCPz: case DECz: case INCz: case ISBz: case LSRz:
        case ROLz: case RORz: case SREz: case SLOz: case RLAz: case RRAz:
            access = AccessMode::READ;
            // fallthrough
        case SAXz: case STAz: case STXz: case STYz:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            break;

        // Zero Page with X Offset Addressing Mode Handler
        // these issue extra reads on the 0 page, but we don't care about it
        // because there are no detectable effects from them. These reads
        // occur during the "wasted" cycle.
        case ADCzx: case ANDzx:  case CMPzx: case EORzx: case LDAzx: case LDYzx:
        case NOPzx_: case ORAzx: case SBCzx:
        case ASLzx: case DCPzx: case DECzx: case INCzx: case ISBzx: case LSRzx:
        case RLAzx:    case ROLzx: case RORzx: case RRAzx: case SLOzx: case SREzx:
            access = AccessMode::READ;
            // fallthrough
        case STAzx: case STYzx:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddrX>;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            break;

        // Zero Page with Y Offset Addressing Mode Handler
        case LDXzy: case LAXzy:
            access = AccessMode::READ;
            // fallthrough
        case STXzy: case SAXzy:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddrY>;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            break;

        // Absolute Addressing Mode Handler
        case ADCa: case ANDa: case BITa: case CMPa: case CPXa: case CPYa:
        case EORa: case LAXa: case LDAa: case LDXa: case LDYa: case NOPa:
        case ORAa: case SBCa:
        case ASLa: case DCPa: case DECa: case INCa: case ISBa: case LSRa:
        case ROLa: case RORa: case SLOa: case SREa: case RLAa: case RRAa:
            access = AccessMode::READ;
            // fallthrough
        case JMPw: case SAXa: case STAa: case STXa: case STYa:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddr>;
            break;

        case JSRw:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            break;

        // Absolute With X Offset Addressing Mode Handler (Read)
        case ADCax: case ANDax:  case CMPax: case EORax: case LDAax:
        case LDYax: case NOPax_: case ORAax: case SBCax:
            access = AccessMode::READ;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddrX2>;
            // this cycle is skipped if the address is already correct.
            // otherwise, it will be read and ignored.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        // Absolute X (RMW; no page crossing handled, always reads before writing)
        case ASLax: case DCPax: case DECax: case INCax: case ISBax:
        case LSRax: case RLAax: case ROLax: case RORax: case RRAax:
        case SLOax: case SREax:
            access = AccessMode::READ;
            // fallthrough
        case SHYax: case STAax:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddrX>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        // Absolute With Y Offset Addresing Mode Handler (Read)
        case ADCay: case ANDay: case CMPay: case EORay: case LASay:
        case LAXay: case LDAay: case LDXay: case ORAay: case SBCay:
            access = AccessMode::READ;
            // fallthrough
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddrY2>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        // Absolute Y (No page crossing handled)
        case DCPay: case ISBay: case RLAay: case RRAay: case SLOay:
        case SREay:
            access = AccessMode::READ;
            // fallthrough
        case SHAay: case SHSay: case SHXay: case STAay:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddrY>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        // Absolute Indirect Addressing Mode Handler
        case JMPi:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowPointer>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighPointer>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowEffAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighEffAddr>;
            break;

        // Indexed with X Preinc Addressing Mode Handler
        case ADCix: case ANDix: case CMPix: case EORix: case LAXix: case LDAix:
        case ORAix: case SBCix:
        case DCPix: case ISBix: case SLOix: case SREix: case RLAix: case RRAix:
            access = AccessMode::READ;
            // fallthrough
        case SAXix: case STAix:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowPointer>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowPointerX>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowEffAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighEffAddr>;
            break;

        // Indexed with Y Postinc Addressing Mode Handler (Read)
        case ADCiy: case ANDiy: case CMPiy: case EORiy: case LAXiy:
        case LDAiy: case ORAiy: case SBCiy:
            access = AccessMode::READ;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowPointer>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowEffAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighEffAddrY2>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        // Indexed Y (No page crossing handled)
        case DCPiy: case ISBiy: case RLAiy: case RRAiy: case SLOiy:
        case SREiy:
            access = AccessMode::READ;
            // fallthrough
        case SHAiy: case STAiy:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowPointer>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchLowEffAddr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighEffAddrY>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::throwAwayRead>;
            break;

        default:
            legalMode = false;
            break;
        }

        if (access == AccessMode::READ)
        {
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchEffAddrDataByte>;
        }

        //---------------------------------------------------------------------------------------
        // Addressing Modes Finished, other cycles are instruction dependent
        //---------------------------------------------------------------------------------------
        switch(i)
        {
        case ADCz:  case ADCzx: case ADCa: case ADCax: case ADCay: case ADCix:
        case ADCiy: case ADCb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::adc_instr>;
            break;

        case ANCb_:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::anc_instr>;
            break;

        case ANDz:  case ANDzx: case ANDa: case ANDax: case ANDay: case ANDix:
        case ANDiy: case ANDb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::and_instr>;
            break;

        case ANEb: // Also known as XAA
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ane_instr>;
            break;

        case ARRb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::arr_instr>;
            break;

        case ASLn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::asla_instr>;
            break;

        case ASLz: case ASLzx: case ASLa: case ASLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::asl_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case ASRb: // Also known as ALR
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::alr_instr>;
            break;

        case BCCr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bcc_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BCSr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bcs_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BEQr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::beq_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BITz: case BITa:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bit_instr>;
            break;

        case BMIr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bmi_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BNEr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bne_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BPLr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bpl_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BRKn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PushHighPC>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::brkPushLowPC>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PushSR>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::IRQLoRequest>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::IRQHiRequest>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fetchNextOpcode>;
            break;

        case BVCr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bvc_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case BVSr:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::bvs_instr>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::fix_branch>;
            break;

        case CLCn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::clc_instr>;
            break;

        case CLDn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::cld_instr>;
            break;

        case CLIn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::cli_instr>;
            break;

        case CLVn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::clv_instr>;
            break;

        case CMPz:  case CMPzx: case CMPa: case CMPax: case CMPay: case CMPix:
        case CMPiy: case CMPb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::cmp_instr>;
            break;

        case CPXz: case CPXa: case CPXb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::cpx_instr>;
            break;

        case CPYz: case CPYa: case CPYb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::cpy_instr>;
            break;

        case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
        case DCPiy: // Also known as DCM
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::dcm_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case DECz: case DECzx: case DECa: case DECax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::dec_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case DEXn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::dex_instr>;
            break;

        case DEYn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::dey_instr>;
            break;

        case EORz:  case EORzx: case EORa: case EORax: case EORay: case EORix:
        case EORiy: case EORb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::eor_instr>;
            break;
#if 0
        // HLT, also known as JAM
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
            instrTable[buildCycle++].func = &StaticFuncWrapper<hlt_instr>;
            break;
#endif
        case INCz: case INCzx: case INCa: case INCax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::inc_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case INXn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::inx_instr>;
            break;

        case INYn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::iny_instr>;
            break;

        case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
        case ISBiy: // Also known as INS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ins_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case JSRw:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PushHighPC>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PushLowPC>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::FetchHighAddr>;
            // fallthrough
        case JMPw: case JMPi:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::jmp_instr>;
            break;

        case LASay:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::las_instr>;
            break;

        case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::lax_instr>;
            break;

        case LDAz:  case LDAzx: case LDAa: case LDAax: case LDAay: case LDAix:
        case LDAiy: case LDAb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::lda_instr>;
            break;

        case LDXz: case LDXzy: case LDXa: case LDXay: case LDXb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ldx_instr>;
            break;

        case LDYz: case LDYzx: case LDYa: case LDYax: case LDYb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ldy_instr>;
            break;

        case LSRn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::lsra_instr>;
            break;

        case LSRz: case LSRzx: case LSRa: case LSRax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::lsr_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case NOPn_: case NOPb_:
        case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        // NOPb NOPz NOPzx - Also known as SKBn
        // NOPa NOPax      - Also known as SKWn
            break;

        case LXAb: // Also known as OAL
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::oal_instr>;
            break;

        case ORAz:  case ORAzx: case ORAa: case ORAax: case ORAay: case ORAix:
        case ORAiy: case ORAb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ora_instr>;
            break;

        case PHAn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::pha_instr>;
            break;

        case PHPn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PushSR>;
            break;

        case PLAn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::pla_instr>;
            break;

        case PLPn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopSR>;
            break;

        case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
        case RLAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rla_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case ROLn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rola_instr>;
            break;

        case ROLz: case ROLzx: case ROLa: case ROLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rol_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case RORn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rora_instr>;
            break;

        case RORz: case RORzx: case RORa: case RORax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::ror_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
        case RRAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rra_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case RTIn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopSR>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopLowPC>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopHighPC>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rti_instr>;
            break;

        case RTSn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::WasteCycle>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopLowPC>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PopHighPC>;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::rts_instr>;
            break;

        case SAXz: case SAXzy: case SAXa: case SAXix: // Also known as AXS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::axs_instr>;
            break;

        case SBCz:  case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
        case SBCiy: case SBCb_:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sbc_instr>;
            break;

        case SBXb:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sbx_instr>;
            break;

        case SECn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sec_instr>;
            break;

        case SEDn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sed_instr>;
            break;

        case SEIn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sei_instr>;
            break;

        case SHAay: case SHAiy: // Also known as AXA
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::axa_instr>;
            break;

        case SHSay: // Also known as TAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::shs_instr>;
            break;

        case SHXay: // Also known as XAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::xas_instr>;
            break;

        case SHYax: // Also known as SAY
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::say_instr>;
            break;

        case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
        case SLOiy: // Also known as ASO
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::aso_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
        case SREiy: // Also known as LSE
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::lse_instr>;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::PutEffAddrDataByte>;
            break;

        case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
        case STAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sta_instr>;
            break;

        case STXz: case STXzy: case STXa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::stx_instr>;
            break;

        case STYz: case STYzx: case STYa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::sty_instr>;
            break;

        case TAXn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::tax_instr>;
            break;

        case TAYn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::tay_instr>;
            break;

        case TSXn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::tsx_instr>;
            break;

        case TXAn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::txa_instr>;
            break;

        case TXSn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::txs_instr>;
            break;

        case TYAn:
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::tya_instr>;
            break;

        default:
            legalInstr = false;
            break;
        }

        // Missing an addressing mode or implementation makes opcode invalid.
        // These are normally called HLT instructions. In the hardware, the
        // CPU state machine locks up and will never recover.
        if (!(legalMode && legalInstr))
        {
            instrTable[buildCycle++].func = &StaticFuncWrapper<&MOS6510::invalidOpcode>;
        }

        // check for IRQ triggers or fetch next opcode...
        instrTable[buildCycle].func = &StaticFuncWrapper<&MOS6510::interruptsAndNextOpcode>;

#ifndef NDEBUG
        printf("Done [%u Cycles]\n", buildCycle - (i << 3));
#endif
    }
}

/**
 * Initialise CPU Emulation (Registers).
 */
void MOS6510::Initialise()
{
    m_pcmActive = false;
    m_playerCallActive = false;
    m_idleActive = false;
    m_pcmCached = false;
    m_loopEnabled = false;
    m_loopCacheUsed = false;
    m_loopSkippedCycles = 0;
    std::memset(m_loopCache, 0, sizeof(m_loopCache));
    std::memset(m_loopSeenPages, 0, sizeof(m_loopSeenPages));
    m_pcmEnabled = false;
    m_pcmCalls = 0;
    // Reset stack
    Register_StackPointer = 0xff;

    // Reset Cycle Count
    cycleCount = (BRKn << 3) + 6; // fetchNextOpcode


    if (cpu_debug) UNLIKELY
    {
        cpu_debug->instrStartPC = -1;
    }

    // Reset Status Register
    flags.reset();

    // Set PC to some value
    Register_ProgramCounter = 0;

    // IRQs pending check
    irqAssertedOnPin = false;
    nmiFlag = false;
    rstFlag = false;
    interruptCycle = MAX;

    // Signals
    rdy = true;
    d1x1 = false;

    eventScheduler.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
}

/**
 * Reset CPU Emulation.
 */
void MOS6510::reset()
{
    // Internal Stuff
    Initialise();

    // Set processor port to the default values
    cpuWrite(0, 0x2f);
    cpuWrite(1, 0x37);

    // Requires External Bits
    // Read from reset vector for program entry point
    endian_16lo8(Cycle_EffectiveAddress, cpuRead(0xfffc));
    endian_16hi8(Cycle_EffectiveAddress, cpuRead(0xfffd));
    Register_ProgramCounter = Cycle_EffectiveAddress;
}

/**
 * Module Credits.
 */
const char *MOS6510::credits()
{
    return
        "MOS6510 Cycle Exact Emulation\n"
        "\t(C) 2000 Simon A. White\n"
        "\t(C) 2008-2010 Antti S. Lankila\n"
        "\t(C) 2011-2026 Leandro Nini\n";
}

void MOS6510::debug(bool enable, FILE *out)
{
    if (enable)
    {
        cpu_debug = MAKE_UNIQUE(CPUDebug);
        cpu_debug->m_fdbg = out ? out : stdout;
    }
    else
    {
        cpu_debug.reset();
    }
}

void MOS6510::pcmCodeWrite(uint16_t address)
{
    if (m_pcmActive && m_pcmActions[0].kind == PcmActionKind::MixerFinish &&
        (address < 2 || (address >= m_pcmLow && address < m_pcmHigh)))
        throw sidplayfp::SeekReplayRequired();
    if (m_idleActive && address >= m_idlePC && address <= m_idlePC + 2)
        idleWake();
    if (m_pcmCached && (address < 2 || (address >= m_pcmLow && address < m_pcmLow + 50) ||
                        (address >= m_pcmHigh && address < m_pcmHigh + 43)))
    {
        if (m_pcmActive)
        {
            if (m_playerCallActive)
                m_playerCallAllowed = false;
            throw sidplayfp::SeekReplayRequired();
        }
        m_pcmCached = false;
    }
}

bool MOS6510::cachePcmRoutine(uint16_t pc)
{
    if (pc < 0x0200 || pc > 0xcfce)
        return false;
    uint16_t low = pc, high = 0;
    if (cpuRead(pc + 1) == 0x98)
        high = cpuRead(pc + 27) | (cpuRead(pc + 32) << 8);
    else if (cpuRead(pc + 1) == 0xa5 && cpuRead(pc + 3) == 0xcd)
    {
        low = cpuRead(pc + 29) | (cpuRead(pc + 34) << 8);
        high = pc;
    }
    else
        return false;
    if (low < 0x0200 || low > 0xcfce || high < 0x0200 || high > 0xcfd5)
        return false;
    static constexpr int lowPattern[50] = {
        0x48, 0x98, 0x48, 0xa0, 0,    0xa5, -1,   0xcd, -1,   -1,   0xf0, 0x1f, 0xb1,
        -1,   0x8d, -1,   -1,   0x29, 0x0f, 0x4a, 0x18, 0x69, -1,   0x8d, -1,   -1,
        0xa9, -1,   0x8d, 0x18, 3,    0xa9, -1,   0x8d, 0x19, 3,    0x68, 0xa8, 0xad,
        0x0d, 0xdd, 0x68, 0x40, 0xa9, -1,   0x8d, -1,   -1,   0xd0, 0xe8};
    static constexpr int highPattern[43] = {
        0x48, 0xa5, -1,   0xcd, -1,   -1,   0xf0, 0x14, 0xad, -1,   -1,   0x4a, 0x4a, 0x4a, 0x4a,
        0x4a, 0x18, 0x69, -1,   0x8d, -1,   -1,   0xe6, -1,   0xd0, 2,    0xe6, -1,   0xa9, -1,
        0x8d, 0x18, 3,    0xa9, -1,   0x8d, 0x19, 3,    0xad, 0x0d, 0xdd, 0x68, 0x40};
    uint8_t l[50], h[43];
    for (unsigned i = 0; i < 50; ++i)
    {
        l[i] = cpuRead(low + i);
        if (lowPattern[i] >= 0 && l[i] != lowPattern[i])
            return false;
    }
    for (unsigned i = 0; i < 43; ++i)
    {
        h[i] = cpuRead(high + i);
        if (highPattern[i] >= 0 && h[i] != highPattern[i])
            return false;
    }
    const auto word = [](uint8_t lo, uint8_t hi) { return static_cast<uint16_t>(lo | (hi << 8)); };
    const auto ordinary = [](uint16_t a) { return a >= 0x0200 && a < 0xd000; };
    const auto overlaps = [&](uint16_t a)
    { return (a >= low && a < low + 50) || (a >= high && a < high + 43); };
    const uint16_t scratch = word(l[15], l[16]), end = word(l[8], l[9]), dac = word(l[24], l[25]);
    if (l[13] < 2 || l[13] == 255 || l[6] != l[13] + 1 || h[2] != l[6] || h[23] != l[13] ||
        h[27] != l[6] || word(h[4], h[5]) != end || word(h[9], h[10]) != scratch ||
        h[18] != l[22] || word(h[20], h[21]) != dac || word(l[46], l[47]) != dac ||
        word(l[27], l[32]) != high || word(h[29], h[34]) != low || !ordinary(scratch) ||
        !ordinary(end) || overlaps(scratch) || overlaps(0x0318) || overlaps(0x0319) ||
        scratch == 0x0318 || scratch == 0x0319 ||
        !(((dac >= 0xd400 && dac < 0xd800) || dac >= 0xde00) && (dac & 31) == 0x18))
        return false;
    m_pcmLow = low;
    m_pcmHigh = high;
    m_pcmScratch = scratch;
    m_pcmEnd = end;
    m_pcmDac = dac;
    m_pcmPointer = l[13];
    m_pcmAdd = l[22];
    m_pcmEndValue = l[44];
    m_pcmCached = true;
    return true;
}

void MOS6510::addPcmAction(unsigned cycle, PcmActionKind kind, uint16_t address, uint8_t value)
{
    m_pcmActions[m_pcmActionCount++] = {cycle, kind, address, value};
}

bool MOS6510::beginNativePcm(uint16_t pc)
{
    if (!m_pcmCached && !cachePcmRoutine(pc))
        return false;
    if (pc != m_pcmLow && pc != m_pcmHigh)
        return false;
    const uint8_t lo = cpuRead(m_pcmPointer), hi = cpuRead(m_pcmPointer + 1);
    const bool end = hi == cpuRead(m_pcmEnd), high = pc == m_pcmHigh;
    const uint16_t source = static_cast<uint16_t>(lo | (hi << 8));
    if (!high && !end &&
        (source < 2 || (source >= 0x100 && source < 0x200) ||
         (source >= 0xd000 && source < 0xe000)))
        return false;
    m_pcmSavedA = Register_Accumulator;
    m_pcmSavedY = Register_Y;
    m_pcmSavedSP = Register_StackPointer;
    m_pcmActionCount = m_pcmActionIndex = 0;
    m_pcmCycle = 0;
    addPcmAction(2, PcmActionKind::Write, 0x100 | m_pcmSavedSP, m_pcmSavedA);
    const auto page = [](unsigned from, unsigned to)
    { return ((from ^ to) & 0xff00) != 0 ? 1u : 0u; };
    unsigned vectorCycle;
    if (!high)
    {
        addPcmAction(7, PcmActionKind::Write, 0x100 | static_cast<uint8_t>(m_pcmSavedSP - 1),
                     m_pcmSavedY);
        if (end)
        {
            const unsigned branch = page(m_pcmLow + 12, m_pcmLow + 43);
            addPcmAction(25 + branch, PcmActionKind::Write, m_pcmDac, m_pcmEndValue);
            vectorCycle = 29 + branch + page(m_pcmLow + 50, m_pcmLow + 26);
        }
        else
        {
            addPcmAction(23, PcmActionKind::ReadLow, source);
            addPcmAction(27, PcmActionKind::Scratch, m_pcmScratch);
            addPcmAction(39, PcmActionKind::Dac, m_pcmDac);
            vectorCycle = 40;
        }
        addPcmAction(vectorCycle + 5, PcmActionKind::Write, 0x318, static_cast<uint8_t>(m_pcmHigh));
        addPcmAction(vectorCycle + 11, PcmActionKind::Write, 0x319,
                     static_cast<uint8_t>(m_pcmHigh >> 8));
        addPcmAction(vectorCycle + 21, PcmActionKind::Ack, 0xdd0d);
        addPcmAction(vectorCycle + 22, PcmActionKind::Finish, m_pcmLow + 41);
    }
    else
    {
        if (end)
            vectorCycle = 13 + page(m_pcmHigh + 8, m_pcmHigh + 28);
        else
        {
            addPcmAction(15, PcmActionKind::ReadHigh, m_pcmScratch);
            addPcmAction(33, PcmActionKind::Dac, m_pcmDac);
            addPcmAction(37, PcmActionKind::Write, m_pcmPointer, lo);
            addPcmAction(38, PcmActionKind::Write, m_pcmPointer, static_cast<uint8_t>(lo + 1));
            if (lo == 255)
            {
                addPcmAction(44, PcmActionKind::Write, m_pcmPointer + 1, hi);
                addPcmAction(45, PcmActionKind::Write, m_pcmPointer + 1,
                             static_cast<uint8_t>(hi + 1));
                vectorCycle = 46;
            }
            else
                vectorCycle = 42 + page(m_pcmHigh + 26, m_pcmHigh + 28);
        }
        addPcmAction(vectorCycle + 5, PcmActionKind::Write, 0x318, static_cast<uint8_t>(m_pcmLow));
        addPcmAction(vectorCycle + 11, PcmActionKind::Write, 0x319,
                     static_cast<uint8_t>(m_pcmLow >> 8));
        addPcmAction(vectorCycle + 15, PcmActionKind::Ack, 0xdd0d);
        addPcmAction(vectorCycle + 16, PcmActionKind::Finish, m_pcmHigh + 41);
    }
    if (end)
        flags.setC(true);
    // Combine DAC writes and RAM work at the final handler boundary.
    // Account for elapsed CPU cycles and RDY stalls on writes.
    const unsigned count = m_pcmActionCount;
    m_pcmActionCount = m_pcmMemoryCount = 0;
    for (auto& write : m_pcmWriteCycle)
        write = false;
    for (unsigned i = 0; i < count; ++i)
    {
        auto action = m_pcmActions[i];
        switch (action.kind)
        {
        case PcmActionKind::ReadLow:
        case PcmActionKind::ReadHigh:
            m_pcmSample = cpuRead(action.address);
            Register_Accumulator = action.kind == PcmActionKind::ReadLow ? ((m_pcmSample & 15) >> 1)
                                                                         : (m_pcmSample >> 5);
            Cycle_Data = m_pcmAdd;
            flags.setC(false);
            doADC();
            break;
        case PcmActionKind::Scratch:
            action.kind = PcmActionKind::Write;
            action.value = m_pcmSample;
            m_pcmWriteCycle[action.cycle] = true;
            m_pcmMemoryWrites[m_pcmMemoryCount++] = action;
            break;
        case PcmActionKind::Dac:
            action.kind = PcmActionKind::Write;
            action.value = Register_Accumulator;
            m_pcmWriteCycle[action.cycle] = true;
            m_pcmMemoryWrites[m_pcmMemoryCount++] = action;
            break;
        case PcmActionKind::Write:
            m_pcmWriteCycle[action.cycle] = true;
            m_pcmMemoryWrites[m_pcmMemoryCount++] = action;
            break;
        case PcmActionKind::Ack:
            break;
        default:
            m_pcmActions[m_pcmActionCount++] = action;
            break;
        }
    }
    m_pcmClock = eventScheduler.halfTime();
    m_pcmActive = true;
    ++m_pcmCalls;
    instrTable[nativeSlot].func = &StaticFuncWrapper<&MOS6510::nativePcmCycle>;
    cycleCount = nativeSlot;
    updatePcmCycleKind();
    return true;
}

void MOS6510::updatePcmCycleKind()
{
    const auto& action =
        m_playerCallActive ? m_playerActions[m_pcmActionIndex] : m_pcmActions[m_pcmActionIndex];
    instrTable[nativeSlot].nosteal =
        action.cycle == m_pcmCycle &&
        (action.kind == PcmActionKind::Write || action.kind == PcmActionKind::Scratch ||
         action.kind == PcmActionKind::Dac);
}

void MOS6510::nativePcmSeek(bool enable)
{
    m_pcmEnabled = enable;
    if (!enable && !m_loopEnabled && m_idleActive)
        idleWake();
}

void MOS6510::idleSync()
{
    const auto now = eventScheduler.halfTime();
    if (now <= m_idleClock)
        return;
    if (rdy)
    {
        const auto clocks = (now - m_idleClock - 1) / 2;
        m_idlePhase = (m_idlePhase + clocks % 3) % 3;
    }
    m_idleClock = now - ((now & 1) ? 2 : 1);
    cycleCount = (JMPw << 3) + m_idlePhase;
    Register_ProgramCounter = m_idlePC + 1 + m_idlePhase;
    Cycle_EffectiveAddress = m_idlePC;
}

void MOS6510::idleWake()
{
    idleSync();
    m_idleActive = false;
    if (rdy)
        eventScheduler.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
    else
        eventScheduler.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
}

void MOS6510::nativePcmSync()
{
    if (m_playerCallActive)
    {
        playerCallSync();
        return;
    }
    const auto now = eventScheduler.halfTime();
    if (now <= m_pcmClock)
        return;
    unsigned clocks = static_cast<unsigned>((now - m_pcmClock - 1) / 2);
    const auto next = m_pcmActions[m_pcmActionIndex].cycle;
    if (clocks >= next - m_pcmCycle)
        clocks = next - m_pcmCycle - 1;
    if (!rdy)
    {
        unsigned writes = 0;
        while (m_pcmCycle + writes + 1 < 96 && m_pcmWriteCycle[m_pcmCycle + writes + 1])
            ++writes;
        if (clocks > writes)
            clocks = writes;
    }
    m_pcmCycle += clocks;
    m_pcmClock = now - ((now & 1) ? 2 : 1);
}

void MOS6510::nativePcmSchedule()
{
    if (m_playerCallActive)
    {
        playerCallSchedule();
        return;
    }
    eventScheduler.cancel(m_nosteal);
    eventScheduler.cancel(m_steal);
    const auto action = m_pcmActions[m_pcmActionIndex];
    const auto gap = action.cycle - m_pcmCycle;
    if (!rdy)
    {
        unsigned writes = 0;
        while (m_pcmCycle + writes + 1 < 96 && m_pcmWriteCycle[m_pcmCycle + writes + 1])
            ++writes;
        if (gap > writes)
            return;
    }
    const auto next = eventScheduler.halfTime() | 1;
    const auto target = m_pcmClock + static_cast<event_clock_t>(gap) * 2;
    if (target < next)
        throw sidplayfp::SeekReplayRequired();
    eventScheduler.schedule(m_nosteal, static_cast<unsigned>((target - next) / 2),
                            EVENT_CLOCK_PHI2);
}

void MOS6510::nativePcmCycle()
{
    const auto action = m_pcmActions[m_pcmActionIndex++];
    m_pcmCycle = action.cycle;
    m_pcmClock = eventScheduler.halfTime();
    switch (action.kind)
    {
    case PcmActionKind::Write:
        cpuWrite(action.address, action.value);
        break;
    case PcmActionKind::Ack:
        Register_Accumulator = cpuRead(action.address);
        flags.setNZ(Register_Accumulator);
        break;
    case PcmActionKind::MixerFinish:
        for (unsigned i = 0; i < m_pcmMemoryCount; ++i)
            cpuWrite(m_pcmMemoryWrites[i].address, m_pcmMemoryWrites[i].value);
        Register_StackPointer = static_cast<uint8_t>(m_pcmSavedSP - 2);
        m_pcmActive = false;
        Register_ProgramCounter = action.address;
        fetchNextOpcode();
        if (rdy)
            eventScheduler.schedule(m_nosteal, 1);
        else
            eventScheduler.schedule(m_steal, 1);
        return;
    case PcmActionKind::Finish:
        Register_Accumulator = cpuRead(0xdd0d);
        for (unsigned i = 0; i < m_pcmMemoryCount; ++i)
            cpuWrite(m_pcmMemoryWrites[i].address, m_pcmMemoryWrites[i].value);
        Register_Y = m_pcmSavedY;
        Register_StackPointer = static_cast<uint8_t>(m_pcmSavedSP - 1);
        flags.setNZ(Register_Accumulator);
        m_pcmActive = false;
        Register_ProgramCounter = action.address;
        fetchNextOpcode();
        if (rdy)
            eventScheduler.schedule(m_nosteal, 1);
        else
            eventScheduler.schedule(m_steal, 1);
        return;
    default:
        break;
    }
    nativePcmSchedule();
}

bool MOS6510::beginDigiMixer(uint16_t pc)
{
    if (pc < 0x9d00 || pc > 0x9f50 || ((pc & 255) != 0 && (pc & 255) != 0x50))
        return false;
    static constexpr uint8_t code0[] = {72,  152, 72,  160, 0,   177, 81, 41,  15, 74, 24,
                                        105, 8,   141, 24,  212, 169, 80, 141, 24, 3};
    static constexpr uint8_t code1[] = {72,  152, 72,  160, 0,   177, 81,  74,  74,
                                        74,  74,  74,  24,  105, 8,   141, 24,  212,
                                        169, 0,   141, 24,  3,   230, 81,  208, 15};
    static constexpr uint8_t code2[] = {72,  152, 72,  160, 0,   177, 76,  74,  74,
                                        74,  74,  74,  24,  105, 8,   141, 24,  212,
                                        169, 80,  141, 24,  3,   230, 76,  208, 15};
    static constexpr uint8_t code3[] = {72,  152, 72,  160, 0,   177, 76, 41,  15, 74, 24,
                                        105, 8,   141, 24,  212, 169, 0,  141, 24, 3};
    static constexpr uint8_t code4[] = {72,  152, 72,  160, 0,  177, 76,  74, 74,  74, 74,  141, 64,
                                        3,   177, 81,  41,  15, 24,  109, 64, 3,   74, 24,  73,  15,
                                        141, 24,  212, 169, 80, 141, 24,  3,  230, 76, 208, 17};
    static constexpr uint8_t code5[] = {72,  152, 72,  160, 0,  177, 81,  74, 74,  74, 74,  141, 64,
                                        3,   177, 76,  41,  15, 24,  109, 64, 3,   74, 24,  73,  15,
                                        141, 24,  212, 169, 0,  141, 24,  3,  230, 81, 208, 17};
    static constexpr const uint8_t* codes[] = {code0, code1, code2, code3, code4, code5};
    const unsigned mode = ((pc >> 8) - 0x9d) * 2 + ((pc & 255) ? 1 : 0);
    const bool mix = mode >= 4, advance = mix || mode == 1 || mode == 2;
    const unsigned length = mix ? 38 : advance ? 27 : 21;
    if (m_loopCache[pc] != 3)
    {
        for (unsigned i = 0; i < length; ++i)
            if (cpuRead(pc + i) != codes[mode][i])
                return false;
        m_loopCache[pc] = 3;
        m_loopCacheUsed = true;
        m_loopSeenPages[pc >> 8] = true;
    }
    const auto* code = codes[mode];
    const unsigned finish = mix ? 55 : advance ? 42 : 21;
    // Keep sample-end and page-crossing control flow interpreted.
    const uint8_t pointer = code[6], other = mix ? code[15] : 0;
    const uint8_t lo = cpuRead(pointer), hi = cpuRead(static_cast<uint8_t>(pointer + 1));
    if (advance && lo == 255)
        return false;
    const unsigned branch = mix ? 36 : 25;
    if (advance &&
        (cpuRead(pc + branch) != 0xd0 ||
         static_cast<uint16_t>(pc + branch + 2 + static_cast<int8_t>(cpuRead(pc + branch + 1))) !=
             pc + finish))
        return false;
    if (cpuRead(pc + finish) != 0x68 || cpuRead(pc + finish + 1) != 0xa8 ||
        cpuRead(pc + finish + 2) != 0x68)
        return false;
    const uint16_t address = lo | (hi << 8),
                   address2 =
                       mix ? (cpuRead(other) | (cpuRead(static_cast<uint8_t>(other + 1)) << 8)) : 0;
    const auto safe = [](uint16_t a) { return a >= 0x400 && (a < 0xd000 || a >= 0xe000); };
    if (!safe(address) || (mix && !safe(address2)))
        return false;
    m_pcmCached = false;
    m_pcmLow = pc;
    m_pcmHigh = pc + length;
    m_pcmSavedSP = Register_StackPointer;
    m_pcmSavedA = Register_Accumulator;
    m_pcmSavedY = Register_Y;
    m_pcmActionCount = m_pcmActionIndex = m_pcmMemoryCount = m_pcmCycle = 0;
    for (auto& w : m_pcmWriteCycle)
        w = false;
    const auto write = [&](unsigned cycle, uint16_t a, uint8_t v)
    {
        m_pcmWriteCycle[cycle] = true;
        m_pcmMemoryWrites[m_pcmMemoryCount++] = {cycle, PcmActionKind::Write, a, v};
    };
    write(2, 0x100 | m_pcmSavedSP, m_pcmSavedA);
    write(7, 0x100 | static_cast<uint8_t>(m_pcmSavedSP - 1), m_pcmSavedY);
    Register_Y = 0;
    unsigned clocks;
    if (mix)
    {
        const uint8_t a = cpuRead(address) >> 4, b = cpuRead(address2) & 15;
        const uint16_t scratch = code[12] | (code[13] << 8);
        if (scratch < 0x200 || scratch >= 0xd000)
            throw sidplayfp::SeekReplayRequired();
        write(26, scratch, a);
        Register_Accumulator = b;
        Cycle_Data = a;
        flags.setC(false);
        doADC();
        Register_Accumulator = (Register_Accumulator >> 1) ^ 15;
        flags.setC(false);
        flags.setNZ(Register_Accumulator);
        write(49, 0xd418, Register_Accumulator);
        Register_Accumulator = code[30];
        write(55, 0x318, Register_Accumulator);
        clocks = 64;
        write(59, pointer, lo);
        write(60, pointer, static_cast<uint8_t>(lo + 1));
        flags.setNZ(static_cast<uint8_t>(lo + 1));
    }
    else
    {
        const uint8_t sample = cpuRead(address);
        Register_Accumulator = advance ? sample >> 5 : (sample & 15) >> 1;
        Cycle_Data = code[advance ? 14 : 12];
        flags.setC(false);
        doADC();
        write(advance ? 32 : 26, 0xd418, Register_Accumulator);
        const uint8_t next = code[advance ? 19 : 17];
        Register_Accumulator = next;
        write(advance ? 38 : 32, 0x318, next);
        flags.setNZ(next);
        clocks = advance ? 47 : 33;
        if (advance)
        {
            write(42, pointer, lo);
            write(43, pointer, static_cast<uint8_t>(lo + 1));
            flags.setNZ(static_cast<uint8_t>(lo + 1));
        }
    }
    if (advance && ((pc + branch + 2) ^ (pc + finish)) & 0xff00)
        ++clocks;
    addPcmAction(clocks, PcmActionKind::MixerFinish, pc + finish);
    m_pcmClock = eventScheduler.halfTime();
    m_pcmActive = true;
    ++m_pcmCalls;
    instrTable[nativeSlot].func = &StaticFuncWrapper<&MOS6510::nativePcmCycle>;
    cycleCount = nativeSlot;
    updatePcmCycleKind();
    return true;
}

void MOS6510::fastCpuSeek(bool enable)
{
    m_loopEnabled = enable;
    if (!enable && !m_pcmEnabled && m_idleActive)
        idleWake();
}
void MOS6510::playerCallSync()
{
    const auto now = eventScheduler.halfTime();
    if (now <= m_pcmClock)
        return;
    unsigned clocks = static_cast<unsigned>((now - m_pcmClock - 1) / 2);
    const auto next = m_playerActions[m_pcmActionIndex].cycle;
    if (clocks >= next - m_pcmCycle)
        clocks = next - m_pcmCycle - 1;
    if (!rdy)
    {
        unsigned writes = 0;
        while (m_pcmCycle + writes + 1 < 65536 && m_playerWriteCycle[m_pcmCycle + writes + 1])
            ++writes;
        if (clocks > writes)
            clocks = writes;
    }
    m_pcmCycle += clocks;
    m_pcmClock = now - ((now & 1) ? 2 : 1);
}
void MOS6510::playerCallSchedule()
{
    eventScheduler.cancel(m_nosteal);
    eventScheduler.cancel(m_steal);
    while (m_playerActions[m_pcmActionIndex].kind == PcmActionKind::Write &&
           m_playerActions[m_pcmActionIndex].address < 0xd000)
        ++m_pcmActionIndex;
    const auto action = m_playerActions[m_pcmActionIndex];
    const auto gap = action.cycle - m_pcmCycle;
    if (!rdy)
    {
        unsigned writes = 0;
        while (m_pcmCycle + writes + 1 < 65536 && m_playerWriteCycle[m_pcmCycle + writes + 1])
            ++writes;
        if (gap > writes)
            return;
    }
    const auto next = eventScheduler.halfTime() | 1;
    const auto target = m_pcmClock + static_cast<event_clock_t>(gap) * 2;
    if (target < next)
    {
        m_playerCallAllowed = false;
        throw sidplayfp::SeekReplayRequired();
    }
    eventScheduler.schedule(m_nosteal, static_cast<unsigned>((target - next) / 2),
                            EVENT_CLOCK_PHI2);
}
bool MOS6510::beginPlayerCall(uint16_t pc)
{
    if (!m_playerCallAllowed || !rdy || cpu_debug || m_pcmActive || m_idleActive || !flags.getI() ||
        flags.getD() || nmiFlag || rstFlag || interruptCycle != MAX || !dataBus.directReadRAM)
        return false;
    const auto* ram = dataBus.directReadRAM;
    if (std::memcmp(ram + 0x1011, " SID-WIZARD 1.9", 14) &&
        std::memcmp(ram + 0x1011, " SID-WIZARD 1.8", 14))
        return false;
    const auto port = cpuRead(1);
    if (!(port & 4) || !(port & 3))
        return false;
    PlayerCall call;
    std::memcpy(call.memory, ram, 65536);
    call.pc = pc;
    call.a = Register_Accumulator;
    call.x = Register_X;
    call.y = Register_Y;
    call.sp = Register_StackPointer;
    call.p = flags.get();
    if (!prepare_player_call(&call))
        return false;
    m_pcmActionCount = m_pcmActionIndex = m_pcmCycle = 0;
    m_playerWriteCycle.reset();
    for (unsigned i = 0; i < call.count; ++i)
    {
        const auto& w = call.writes[i];
        if (w.cycle == 0)
            return false;
        m_playerActions[m_pcmActionCount++] = {w.cycle, PcmActionKind::Write, w.address, w.value};
        m_playerWriteCycle[w.cycle] = true;
    }
    m_playerActions[m_pcmActionCount++] = {call.cycles, PcmActionKind::PlayerFinish, call.pc, 0};
    m_pcmSavedA = call.a;
    m_pcmPointer = call.x;
    m_pcmSavedY = call.y;
    m_pcmSavedSP = call.sp;
    m_pcmEndValue = call.p;
    m_pcmClock = eventScheduler.halfTime();
    m_pcmActive = true;
    m_playerCallActive = true;
    m_pcmCached = false;
    m_pcmActions[0].kind = PcmActionKind::Write;
    ++m_pcmCalls;
    cycleCount = nativeSlot;
    updatePcmCycleKind();
    return true;
}
void MOS6510::playerCallCycle()
{
    const auto action = m_playerActions[m_pcmActionIndex++];
    m_pcmCycle = action.cycle;
    m_pcmClock = eventScheduler.halfTime();
    if (action.kind == PcmActionKind::PlayerFinish)
    {
        for (unsigned i = 0; i + 1 < m_pcmActionCount; ++i)
        {
            const auto& w = m_playerActions[i];
            if (w.address < 0xd000)
                cpuWrite(w.address, w.value);
        }
        Register_Accumulator = m_pcmSavedA;
        Register_X = m_pcmPointer;
        Register_Y = m_pcmSavedY;
        Register_StackPointer = m_pcmSavedSP;
        flags.set(m_pcmEndValue);
        m_pcmActive = false;
        m_playerCallActive = false;
        Register_ProgramCounter = action.address;
        interruptCycle = MAX;
        fetchNextOpcode();
        if (rdy)
            eventScheduler.schedule(m_nosteal, 1);
        else
            eventScheduler.schedule(m_steal, 1);
        return;
    }
    cpuWrite(action.address, action.value);
    playerCallSchedule();
}

}
