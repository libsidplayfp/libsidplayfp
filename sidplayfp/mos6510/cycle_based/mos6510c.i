/***************************************************************************
                          mos6510.i  -  Cycle Accurate 6510 emulation
                             -------------------
    begin                : Thu May 11 06:22:40 BST 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: mos6510c.i,v $
 *  Revision 1.51  2004/11/12 20:18:43  s_a_white
 *  Valgrind memory access fixes
 *
 *  Revision 1.50  2004/09/18 11:15:01  s_a_white
 *  Mixer's Iisibiisi.sid showed that we were not delaying long enough
 *  between receiving an NMI and starting its processing.
 *
 *  Revision 1.49  2004/06/26 11:11:21  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *
 *  Revision 1.48  2004/05/03 22:37:17  s_a_white
 *  Remove debug code.
 *
 *  Revision 1.47  2004/04/23 01:34:59  s_a_white
 *  CPU timing corrected.
 *
 *  Revision 1.46  2004/04/23 01:04:26  s_a_white
 *  Overlap FetchOpcode with last cycle of previous instruction rather than
 *  next cycle of this instruction.  Although results are almost indentical there
 *  are differences when cycle stealing is invloved.
 *
 *  Revision 1.45  2004/03/07 22:37:42  s_a_white
 *  Cycle stealing only takes effect after 3 consecutive writes or any read.
 *  However stealing does not effect internal only operations and since all writes
 *  are a maximum of 3 cycles followed by a read make just the reads stealable.
 *
 *  Revision 1.44  2004/03/06 21:05:42  s_a_white
 *  Remove debug statements.
 *
 *  Revision 1.43  2004/02/29 14:33:59  s_a_white
 *  If an interrupt occurs during a branch instruction but after the decision
 *  has occured then it should not be delayed.
 *
 *  Revision 1.42  2003/10/29 22:18:04  s_a_white
 *  IRQs are now only taken in on phase 1 as previously they could be clocked
 *  in on both phases of the cycle resulting in them sometimes not being
 *  delayed long enough.
 *
 *  Revision 1.41  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.40  2003/10/16 07:48:32  s_a_white
 *  Allow redirection of debug information of file.
 *
 *  Revision 1.39  2003/07/10 06:52:26  s_a_white
 *  Fixed two memory reads that should have been data and not code accesses
 *
 *  Revision 1.38  2003/05/16 18:28:59  s_a_white
 *  Code modifications to allowing compiling on the QNX6 platform.
 *
 *  Revision 1.37  2003/02/20 21:12:19  s_a_white
 *  Switching to NMI during BRK now occurs correctly.
 *
 *  Revision 1.36  2003/02/20 19:00:20  s_a_white
 *  Code tidy
 *
 *  Revision 1.35  2003/01/23 17:42:33  s_a_white
 *  Fix use of new when exceptions are not supported.
 *
 *  Revision 1.34  2003/01/21 19:26:16  s_a_white
 *  Pending interrupt check is one cycle to late.  Adjust the interrupt delay tests
 *  to compensate.
 *
 *  Revision 1.33  2003/01/20 23:10:48  s_a_white
 *  Fixed RMW instructions to perform memory re-write on the correct cycle.
 *
 *  Revision 1.32  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.31  2003/01/17 08:42:09  s_a_white
 *  Event scheduler phase support.  Better handling the operation of IRQs
 *  during stolen cycles.
 *
 *  Revision 1.30  2002/12/16 08:42:58  s_a_white
 *  Fixed use of nothrow to be namespaced with std::.
 *
 *  Revision 1.29  2002/11/28 20:35:06  s_a_white
 *  Reduced number of thrown exceptions when dma occurs.
 *
 *  Revision 1.28  2002/11/25 20:10:55  s_a_white
 *  A bus access failure should stop the CPU dead like the cycle never started.
 *  This is currently simulated using throw (execption handling) for now.
 *
 *  Revision 1.27  2002/11/21 19:52:48  s_a_white
 *  CPU upgraded to be like other components.  Theres nolonger a clock call,
 *  instead events are registered to occur at a specific time.
 *
 *  Revision 1.26  2002/11/19 22:57:33  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.25  2002/11/01 19:22:36  s_a_white
 *  Removed debug printf.
 *
 *  Revision 1.24  2002/11/01 17:35:27  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.23  2002/03/12 18:48:03  s_a_white
 *  Tidied illegal instruction debug print out.
 *
 *  Revision 1.22  2001/12/11 19:24:15  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.21  2001/11/16 19:21:03  s_a_white
 *  Sign fixes.
 *
 *  Revision 1.20  2001/10/28 21:31:26  s_a_white
 *  Removed kernel debuging code.
 *
 *  Revision 1.19  2001/09/03 22:21:52  s_a_white
 *  When initialising the status register and therefore unmasking the irqs,
 *  check the irq line to see if any are pending.
 *
 *  Revision 1.18  2001/08/10 20:05:50  s_a_white
 *  Fixed RMW instructions which broke due to the optimisation.
 *
 *  Revision 1.17  2001/08/05 15:46:02  s_a_white
 *  No longer need to check on which cycle an instruction ends or when to print
 *  debug information.
 *
 *  Revision 1.16  2001/07/14 13:15:30  s_a_white
 *  Accumulator is now unsigned, which improves code readability.  Emulation
 *  tested with testsuite 2.15.  Various instructions required modification.
 *
 *  Revision 1.15  2001/04/20 22:23:11  s_a_white
 *  Handling of page boundary crossing now correct for branch instructions.
 *
 *  Revision 1.14  2001/03/28 22:59:59  s_a_white
 *  Converted some bad cpuRead's to
 *  cpuRead
 *
 *  Revision 1.13  2001/03/28 21:17:34  s_a_white
 *  Added support for proper RMW instructions.
 *
 *  Revision 1.12  2001/03/24 18:09:17  s_a_white
 *  On entry to interrupt routine the first instruction in the handler is now always
 *  executed before pending interrupts are re-checked.
 *
 *  Revision 1.11  2001/03/22 22:40:43  s_a_white
 *  Added new header for definition of nothrow.
 *
 *  Revision 1.10  2001/03/21 22:27:18  s_a_white
 *  Change to IRQ error message.
 *
 *  Revision 1.9  2001/03/19 23:46:35  s_a_white
 *  NMI no longer sets I flag.  RTI and store instructions are no longer
 *  overlapped.
 *
 *  Revision 1.8  2001/03/09 22:28:51  s_a_white
 *  Speed optimisation update and fix for interrupt flag in PushSR call.
 *
 *  Revision 1.7  2001/02/22 08:28:57  s_a_white
 *  Interrupt masking fixed.
 *
 *  Revision 1.6  2001/02/13 23:01:44  s_a_white
 *  cpuRead now used for some memory accesses.
 *
 *  Revision 1.5  2000/12/24 00:45:38  s_a_white
 *  HAVE_EXCEPTIONS update
 *
 *  Revision 1.4  2000/12/14 23:55:07  s_a_white
 *  PushSR optimisation and PopSR code cleanup.
 *
 ***************************************************************************/

#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include <string.h>

#ifdef PC64_TESTSUITE
static const char _sidtune_CHRtab[256] =  // CHR$ conversion table (0x01 = no output)
{
   0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0xd, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x21, 0x1,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
   0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};
static char filetmp[0x100];
static int  filepos = 0;
#endif // PC64_TESTSUITE

//-------------------------------------------------------------------------//

/** When AEC signal is high, no stealing is possible */
void MOS6510::eventWithoutSteals  (void)
{
    const ProcessorCycle &instr = instrTable[cycleCount++];
    (this->*(instr.func)) ();
    eventContext.schedule(m_nosteal, 1);
}

/** When AEC signal is low, steals permitted */
void MOS6510::eventWithSteals  (void)
{
    if (instrTable[cycleCount].nosteal) {
        const ProcessorCycle &instr = instrTable[cycleCount++];
        (this->*(instr.func)) ();
        eventContext.schedule(m_steal, 1);
    } else {
        /* Even while stalled, the CPU can still process first clock of
        * interrupt delay, but only the first one. However, IRQ may be
        * modified by CLI and SEI, and are specially handled below. */
        if (nmiCycle == cycleCount)
            nmiCycle --;

        /* If stalled on first cycle of CLIn, consume IRQ delay
        * normally and thus allow IRQ to trigger on the next
        * instruction. */
        if (cycleCount == CLIn << 3) {
            cycleCount = NOPn << 3;
            flagI = false;
            irqCycle = irqAsserted ? -MAX : MAX;
            return;
        }

        /* If stalled on first cycle of SEIn, don't consume IRQ delay.
        * This has the effect of causing the interrupt to be skipped
        * if it arrives during the CPU being stalled on this cycle. */
        if (cycleCount == SEIn << 3)
            return;

        /* no special conditions matched */
        if (irqCycle == cycleCount)
            irqCycle --;
    }
}

/**
* Set N and Z flag values.
*
* @param value to set flags from
*/
void MOS6510::setFlagsNZ(const uint8_t value)
{
    flagZ = value == 0;
    flagN = value & 0x80;
}

uint8_t MOS6510::getStatusRegister(void)
{
    uint8_t sr = 0x20;
    if (flagN) {
        sr |= 0x80;
    }
    if (flagV) {
        sr |= 0x40;
    }
    if (flagB) {
        sr |= 0x10;
    }
    if (flagD) {
        sr |= 0x08;
    }
    if (flagI) {
        sr |= 0x04;
    }
    if (flagZ) {
        sr |= 0x02;
    }
    if (flagC) {
        sr |= 0x01;
    }
    return sr;
}

void MOS6510::setStatusRegister(const uint8_t sr)
{
    flagC = sr & 0x01;
    flagZ = sr & 0x02;
    flagI = sr & 0x04;
    flagD = sr & 0x08;
    flagB = sr & 0x10;
    flagV = sr & 0x40;
    flagN = sr & 0x80;
}


/**
* Handle bus access signals. When RDY line is asserted, the CPU
* will pause when executing the next read operation.
*
* @param rdy new state for RDY signal
*/
void MOS6510::setRDY (const bool newAec)
{
    if (rdy == newAec)
        return;

    rdy = newAec;
    if (rdy) {
        eventContext.cancel(m_steal);
        eventContext.schedule(m_nosteal, 0, EVENT_CLOCK_PHI2);
    } else {
        eventContext.cancel(m_nosteal);
        eventContext.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}


/**
* Push P on stack, decrement S
*
* @param newFlagB
*            new value for B flag to set on CPU and write to RAM
*/
void MOS6510::PushSR (const bool b_flag)
{
    flagB = b_flag;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    env->cpuWrite (addr, getStatusRegister());
    Register_StackPointer--;
}

void MOS6510::PushSR (void)
{
    PushSR (true);
}

/**
* increment S, Pop P off stack
*/
void MOS6510::PopSR (void)
{
    // Get status register off stack
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    setStatusRegister(env->cpuRead (addr));
    flagB = true;
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
void MOS6510::triggerRST (void)
{
    Initialise();
    cycleCount = oRST << 3;
}

/**
* Trigger NMI interrupt on the CPU. Calling this method
* flags that CPU must enter the NMI routine at earliest
* opportunity. There is no way to cancel NMI request once
* given.
*/
void MOS6510::triggerNMI (void)
{
    if (nmiCycle != MAX)
        return;

    nmiCycle = cycleCount;
    /* maybe process 1 clock of interrupt delay. */
    if (! rdy) {
        eventContext.cancel(m_steal);
        eventContext.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/** Pull IRQ line low on CPU. */
void MOS6510::triggerIRQ (void)
{
    /* mark interrupt arrival time */
    if (irqAsserted)
        return;

    irqAsserted = true;
    irqCycle = cycleCount;
    /* maybe process 1 clock of interrupt delay. */
    if (! rdy) {
        eventContext.cancel(m_steal);
        eventContext.schedule(m_steal, 0, EVENT_CLOCK_PHI2);
    }
}

/** Inform CPU that IRQ is no longer pulled low. */
void MOS6510::clearIRQ (void)
{
    irqAsserted = false;
}

void MOS6510::interruptsAndNextOpcode (void)
{
    int offset;
    if (cycleCount > nmiCycle + 2) {
        nmiCycle = MAX;
        offset = oNMI;
    } else if (!flagI && cycleCount > irqCycle + 2) {
        offset = oIRQ;
    } else {
#ifdef MOS6510_DEBUG
        if (dodump)
            MOS6510Debug::DumpState(eventContext.getTime(EVENT_CLOCK_PHI2), *this);
#endif
        FetchOpcode();
        return;
    }

#ifdef MOS6510_DEBUG
    if (dodump)
    {
        const event_clock_t cycles = eventContext.getTime (EVENT_CLOCK_PHI2);
        fprintf (m_fdbg, "****************************************************\n");
        switch (offset)
        {
        case oIRQ:
            fprintf (m_fdbg, " IRQ Routine (%u)\n", cycles);
            break;
        case oNMI:
            fprintf (m_fdbg, " NMI Routine (%u)\n", cycles);
            break;
        case oRST:
            fprintf (m_fdbg, " RST Routine (%u)\n", cycles);
            break;
        }
        fprintf (m_fdbg, "****************************************************\n");
        MOS6510Debug::DumpState(cycles, *this);
    }
#endif
    /* These offset >= 0x100 code paths terminate in FetchOpcode before
    * coming back. This is important because irqCount is only valid during
    * 1 instruction's execution. */
    cycleCount = (offset << 3) + 1;
    const ProcessorCycle &instr = instrTable[cycleCount++];
    (this->*(instr.func)) ();
}

void MOS6510::RSTLoRequest (void)
{
    /* reset the values in MMU */
    env->cpuWrite(0, 0x2f);
    env->cpuWrite(1, 0x37);
    endian_32lo8 (Register_ProgramCounter, env->cpuRead (0xFFFC));
}

void MOS6510::RSTHiRequest (void)
{
    endian_32hi8 (Register_ProgramCounter, env->cpuRead (0xFFFD));
}

void MOS6510::NMILoRequest (void)
{
    endian_32lo8 (Register_ProgramCounter, env->cpuRead (0xFFFA));
}

void MOS6510::NMIHiRequest (void)
{
    endian_32hi8 (Register_ProgramCounter, env->cpuRead (0xFFFB));
}

void MOS6510::IRQRequest (void)
{
    PushSR (false);
    flagI = true;
}

void MOS6510::IRQLoRequest (void)
{
    endian_32lo8 (Register_ProgramCounter, env->cpuRead (0xFFFE));
}

void MOS6510::IRQHiRequest (void)
{
    endian_32hi8 (Register_ProgramCounter, env->cpuRead (0xFFFF));
}

/**
* Fetch opcode, increment PC<BR>
*
* Addressing Modes: All
*/
void MOS6510::FetchOpcode (void)
{
    // Next line used for Debug
    instrStartPC = endian_32lo16 (Register_ProgramCounter);

    cycleCount = (env->cpuRead(endian_32lo16 (Register_ProgramCounter)) & 0xff) << 3;
    Register_ProgramCounter++;

    irqCycle = irqAsserted ? -MAX : MAX;
    if (nmiCycle != MAX)
        nmiCycle = -MAX;
}

/**
* Read the next opcode byte from memory (and throw it away)
*/
void MOS6510::throwAwayFetch (void)
{
    env->cpuRead (endian_32lo16 (Register_ProgramCounter));
}

/**
* Issue throw-away read. Some people use these to ACK CIA IRQs.
*/
void MOS6510::throwAwayRead (void)
{
    env->cpuRead (Cycle_HighByteWrongEffectiveAddress);
}

/**
* Fetch value, increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Immediate
* <LI>Relative
* </UL>
*/
void MOS6510::FetchDataByte (void)
{
    Cycle_Data = env->cpuRead (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;
}

/**
* Fetch low address byte, increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Stack Manipulation
* <LI>Absolute
* <LI>Zero Page
* <LI>Zero Page Indexed
* <LI>Absolute Indexed
* <LI>Absolute Indirect
* </UL>
*/
void MOS6510::FetchLowAddr (void)
{
    Cycle_EffectiveAddress = env->cpuRead (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;
}

/**
* Read from address, add index register X to it<BR>
*
* Addressing Modes:
* <UL>
* <LI>Zero Page Indexed
* </UL>
*/
void MOS6510::FetchLowAddrX (void)
{
    FetchLowAddr ();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_X) & 0xFF;
}

/**
* Read from address, add index register Y to it<BR>
*
* Addressing Modes:
* <UL>
* <LI>Zero Page Indexed
* </UL>
*/
void MOS6510::FetchLowAddrY (void)
{
    FetchLowAddr ();
    Cycle_EffectiveAddress = (Cycle_EffectiveAddress + Register_Y) & 0xFF;
}

/**
* Fetch high address byte, increment PC (Absolute Addressing)<BR>
*
* Low byte must have been obtained first!<BR>
*
* Addressing Modes:
* <UL>
* <LI>Absolute
* </UL>
*/
void MOS6510::FetchHighAddr (void)
{   // Get the high byte of an address from memory
    endian_16hi8 (Cycle_EffectiveAddress, env->cpuRead (endian_32lo16 (Register_ProgramCounter)));
    Register_ProgramCounter++;

    // Next line used for Debug
    endian_16hi8 (instrOperand, endian_16hi8 (Cycle_EffectiveAddress));
}

/**
* Fetch high byte of address, add index register X to low address byte,<BR>
*
* increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Absolute Indexed
* </UL>
*/
void MOS6510::FetchHighAddrX (void)
{
    FetchHighAddr ();
    Cycle_HighByteWrongEffectiveAddress = (Cycle_EffectiveAddress & 0xff00) | ((Cycle_EffectiveAddress + Register_X) & 0xff);
    Cycle_EffectiveAddress += Register_X;
}

 // Handle page boundary crossing
void MOS6510::FetchHighAddrX2 (void)
{
    FetchHighAddrX();
    if (Cycle_EffectiveAddress == Cycle_HighByteWrongEffectiveAddress)
        cycleCount++;
}

/**
* Fetch high byte of address, add index register Y to low address byte,<BR>
*
* increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Absolute Indexed
* </UL>
*/
void MOS6510::FetchHighAddrY (void)
{
    FetchHighAddr ();
    Cycle_HighByteWrongEffectiveAddress = Cycle_EffectiveAddress & 0xff00 | ((Cycle_EffectiveAddress + Register_Y) & 0xff);
    Cycle_EffectiveAddress += Register_Y;
}

// Same as above except dosen't worry about page crossing
void MOS6510::FetchHighAddrY2 (void)
{
    FetchHighAddrY ();
    if (Cycle_EffectiveAddress == Cycle_HighByteWrongEffectiveAddress)
        cycleCount++;
}

/**
* Fetch pointer address low, increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Absolute Indirect
* <LI>Indirect indexed (post Y)
* </UL>
*/
void MOS6510::FetchLowPointer (void)
{
    Cycle_Pointer = env->cpuRead (endian_32lo16 (Register_ProgramCounter));
    Register_ProgramCounter++;

    // Nextline used for Debug
    endian_16hi8 (instrOperand, endian_16hi8 (Cycle_Pointer));
}

/**
* Add X to it<BR>
*
* Addressing Modes:
* <UL>
* <LI>Indexed Indirect (pre X)
* </UL>
*/
void MOS6510::FetchLowPointerX (void)
{
    endian_16lo8 (Cycle_Pointer, (Cycle_Pointer + Register_X) & 0xFF);
}

/**
* Fetch pointer address high, increment PC<BR>
*
* Addressing Modes:
* <UL>
* <LI>Absolute Indirect
* </UL>
*/
void MOS6510::FetchHighPointer (void)
{
    endian_16hi8 (Cycle_Pointer, env->cpuRead (endian_32lo16 (Register_ProgramCounter)));
    Register_ProgramCounter++;

    // Nextline used for Debug
    endian_16hi8 (instrOperand, endian_16hi8 (Cycle_Pointer));
}

/**
* Fetch effective address low<BR>
*
* Addressing Modes:
* <UL>
* <LI>Indirect
* <LI>Indexed Indirect (pre X)
* <LI>Indirect indexed (post Y)
* </UL>
*/
void MOS6510::FetchLowEffAddr (void)
{
    Cycle_EffectiveAddress = env->cpuRead (Cycle_Pointer);
}

/**
* Fetch effective address high<BR>
*
* Addressing Modes:
* <UL>
* <LI>Indirect
* <LI>Indexed Indirect (pre X)
* </UL>
*/
void MOS6510::FetchHighEffAddr (void)
{   // Rev 1.03 (Mike) - Extra +1 removed
    endian_16lo8 (Cycle_Pointer, (Cycle_Pointer + 1) & 0xff);
    endian_16hi8 (Cycle_EffectiveAddress, env->cpuRead (Cycle_Pointer));
}

/**
* Fetch effective address high, add Y to low byte of effective address<BR>
*
* Addressing Modes:
* <UL>
* <LI>Indirect indexed (post Y)
* <UL>
*/
void MOS6510::FetchHighEffAddrY (void)
{
    FetchHighEffAddr ();
    Cycle_HighByteWrongEffectiveAddress = (Cycle_EffectiveAddress & 0xff00) | ((Cycle_EffectiveAddress + Register_Y) & 0xff);
    Cycle_EffectiveAddress += Register_Y;
}

// Handle page boundary crossing
void MOS6510::FetchHighEffAddrY2 (void)
{
    FetchHighEffAddrY();
    if (Cycle_EffectiveAddress == Cycle_HighByteWrongEffectiveAddress)
        cycleCount++;
}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Data Accessing Routines                                          //
// Data Accessing operations as described in 64doc by John West and        //
// Marko Makela                                                            //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::FetchEffAddrDataByte (void)
{
    Cycle_Data = env->cpuRead (Cycle_EffectiveAddress);
}

/**
* Write Cycle_Data to effective address.
*/
void MOS6510::PutEffAddrDataByte (void)
{
    env->cpuWrite (Cycle_EffectiveAddress, Cycle_Data);
}

/**
* Push Program Counter Low Byte on stack, decrement S
*/
void MOS6510::PushLowPC (void)
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    env->cpuWrite (addr, endian_32lo8 (Register_ProgramCounter));
    Register_StackPointer--;
}

/**
* Push Program Counter High Byte on stack, decrement S
*/
void MOS6510::PushHighPC (void)
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    env->cpuWrite (addr, endian_32hi8 (Register_ProgramCounter));
    Register_StackPointer--;
}

/**
* Increment stack and pull program counter low byte from stack.
*/
void MOS6510::PopLowPC (void)
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    endian_16lo8 (Cycle_EffectiveAddress, env->cpuRead (addr));
}

/**
* Increment stack and pull program counter high byte from stack.
*/
void MOS6510::PopHighPC (void)
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    endian_16hi8 (Cycle_EffectiveAddress, env->cpuRead (addr));
}

void MOS6510::WasteCycle (void)
{}

//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Opcodes                                              //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::brk_instr (void)
{
    PushSR   ();
    flagI = true;

    /* check if we need to transform BRK into NMI.
    * c64doc says "If the interrupt arrives before the
    * flag-setting cycle" */
    if (nmiCycle < (BRKn << 3) + 3) {
        cycleCount = (oNMI << 3) + 5;
        nmiCycle = MAX;
    }
}

void MOS6510::cld_instr (void)
{
    flagD = false;
    interruptsAndNextOpcode ();
}

void MOS6510::cli_instr (void)
{
    if (flagI) {
        irqCycle = cycleCount;
    }

    flagI = false;
    interruptsAndNextOpcode();
}

void MOS6510::jmp_instr (void)
{
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);

#ifdef PC64_TESTSUITE
    // Hack - Output character to screen
    const unsigned short pc = endian_32lo16 (Register_ProgramCounter);

    if (pc == 0xffd2)
    {
        const char ch = _sidtune_CHRtab[Register_Accumulator];
        switch (ch)
        {
        case 0:
            break;
        case 1:
            fprintf (stderr, " ");
        case 0xd:
            fprintf (stderr, "\n");
            filepos = 0;
            break;
        default:
            filetmp[filepos++] = ch - 'A' + 'a';
            fprintf (stderr, "%c", ch);
        }
    }

    if (pc == 0xe16f)
    {
        filetmp[filepos] = '\0';
        env->envLoadFile (filetmp);
    }
#endif // PC64_TESTSUITE

    interruptsAndNextOpcode ();
}

void MOS6510::pha_instr (void)
{
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    env->cpuWrite (addr, Register_Accumulator);
    Register_StackPointer--;
}

/* RTI does not delay the IRQ I flag change as it is set 3 cycles before
 * the end of the opcode, and thus the 6510 has enough time to call the
 * interrupt routine as soon as the opcode ends, if necessary. */
void MOS6510::rti_instr (void)
{
#ifdef MOS6510_DEBUG
    if (dodump)
        fprintf (m_fdbg, "****************************************************\n\n");
#endif
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
    interruptsAndNextOpcode ();
}

void MOS6510::rts_instr (void)
{
    env->cpuRead(Cycle_EffectiveAddress);
    endian_32lo16 (Register_ProgramCounter, Cycle_EffectiveAddress);
    Register_ProgramCounter++;
}

void MOS6510::sed_instr (void)
{
    flagD = true;
    interruptsAndNextOpcode ();
}

void MOS6510::sei_instr (void)
{
    interruptsAndNextOpcode();
    flagI = true;
}

void MOS6510::sta_instr (void)
{
    Cycle_Data = Register_Accumulator;
    PutEffAddrDataByte ();
}

void MOS6510::stx_instr (void)
{
    Cycle_Data = Register_X;
    PutEffAddrDataByte ();
}

void MOS6510::sty_instr (void)
{
    Cycle_Data = Register_Y;
    PutEffAddrDataByte ();
}



//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Common Instruction Undocumented Opcodes                                 //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

// Undocumented - This opcode stores the result of A AND X AND the high
// byte of the target address of the operand +1 in memory.
void MOS6510::axa_instr (void)
{
    Cycle_Data = Register_X & Register_Accumulator & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    if (Cycle_HighByteWrongEffectiveAddress != Cycle_EffectiveAddress)
        Cycle_EffectiveAddress = endian_16(Cycle_Data, (uint8_t)Cycle_EffectiveAddress);
    PutEffAddrDataByte ();
}

// Undocumented - AXS ANDs the contents of the A and X registers (without changing the
// contents of either register) and stores the result in memory.
// AXS does not affect any flags in the processor status register.
void MOS6510::axs_instr (void)
{
    Cycle_Data = Register_Accumulator & Register_X;
    PutEffAddrDataByte ();
}

/* Not required - Operation performed By another method
// Undocumented - HLT crashes the microprocessor.  When this opcode is executed, program
// execution ceases.  No hardware interrupts will execute either.  The author
// has characterized this instruction as a halt instruction since this is the
// most straightforward explanation for this opcode's behaviour.  Only a reset
// will restart execution.  This opcode leaves no trace of any operation
// performed!  No registers affected.
void MOS6510::hlt_instr (void)
{
}
*/

/* Not required - Operation performed By another method
void MOS6510::nop_instr (void)
{
}
*/

/* Not required - Operation performed By another method
void MOS6510::php_instr (void)
{
}
*/

// Undocumented - This opcode ANDs the contents of the Y register with <ab+1> and stores the
// result in memory.
void MOS6510::say_instr (void)
{
    Cycle_Data = Register_Y & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    if (Cycle_HighByteWrongEffectiveAddress != Cycle_EffectiveAddress)
        Cycle_EffectiveAddress = endian_16(Cycle_Data, (uint8_t)Cycle_EffectiveAddress);
    PutEffAddrDataByte ();
}

/* Not required - Operation performed By another method
// Undocumented - skip next byte.
void MOS6510::skb_instr (void)
{
    Register_ProgramCounter++;
}
*/

/* Not required - Operation performed By another method
// Undocumented - skip next word.
void MOS6510::skw_instr (void)
{
    Register_ProgramCounter += 2;
}
*/

// Undocumented - This opcode ANDs the contents of the X register with <ab+1> and stores the
// result in memory.
void MOS6510::xas_instr (void)
{
    Cycle_Data = Register_X & (endian_16hi8 (Cycle_EffectiveAddress) + 1);
    if (Cycle_HighByteWrongEffectiveAddress != Cycle_EffectiveAddress)
        Cycle_EffectiveAddress = endian_16(Cycle_Data, (uint8_t)Cycle_EffectiveAddress);
    PutEffAddrDataByte ();
}


/** BCD adding */
void MOS6510::doADC (void)
{
    const uint C      = flagC ? 1 : 0;
    const uint A      = Register_Accumulator;
    const uint s      = Cycle_Data;
    const uint regAC2 = A + s + C;

    if (flagD)
    {   // BCD mode
        uint lo = (A & 0x0f) + (s & 0x0f) + C;
        uint hi = (A & 0xf0) + (s & 0xf0);
        if (lo > 0x09)
            lo += 0x06;
        if (lo > 0x0f)
            hi += 0x10;

        flagZ = !(regAC2 & 0xff);
        flagN = hi & 0x80;
        flagV = ((hi ^ A) & 0x80) && !((A ^ s) & 0x80);
        if (hi > 0x90)
            hi += 0x60;

        flagC = hi > 0xff;
        Register_Accumulator = (hi | (lo & 0x0f));
    }
    else
    {   // Binary mode
        flagC = regAC2 > 0xff;
        flagV = ((regAC2 ^ A) & 0x80) && !((A ^ s) & 0x80);
        setFlagsNZ (Register_Accumulator = regAC2 & 0xff);
    }
}

/** BCD subtracting */
void MOS6510::doSBC (void)
{
    const uint C      = flagC? 0 : 1;
    const uint A      = Register_Accumulator;
    const uint s      = Cycle_Data;
    const uint regAC2 = A - s - C;

    flagC = regAC2 < 0x100;
    flagV =((regAC2 ^ A) & 0x80) && ((A ^ s) & 0x80);
    setFlagsNZ (regAC2);

    if (flagD)
    {   // BCD mode
        uint lo = (A & 0x0f) - (s & 0x0f) - C;
        uint hi = (A & 0xf0) - (s & 0xf0);
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
//-------------------------------------------------------------------------/


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Opcodes                                             //
// See and 6510 Assembly Book for more information on these instructions   //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

void MOS6510::adc_instr (void)
{
    doADC ();
    interruptsAndNextOpcode ();
}

void MOS6510::and_instr (void)
{
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::ane_instr (void)
{
    // For an explanation of this opcode look at:
    // http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_(XAA,_ANE)
    // Based on the table on the same page the most common value of magic
    // for the MOS 6510 is FF
    const uint8_t magic = 0xff;
    setFlagsNZ (Register_Accumulator = (Register_Accumulator | magic) & Register_X & Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::asl_instr (void)
{
    PutEffAddrDataByte ();
    flagC = Cycle_Data & 0x80;
    setFlagsNZ (Cycle_Data <<= 1);
}

void MOS6510::asla_instr (void)
{
    flagC = Register_Accumulator & 0x80;
    setFlagsNZ (Register_Accumulator <<= 1);
    interruptsAndNextOpcode ();
}

void MOS6510::branch_instr (const bool condition)
{
    /*
    * 2 cycles spent before arriving here. spend 0 - 2 cycles here;
    * - condition false: Continue immediately to FetchNextInstr.
    *
    * Otherwise read the byte following the opcode (which is already scheduled to occur on this cycle).
    * This effort is wasted. Then calculate address of the branch target. If branch is on same page,
    * then continue at that insn on next cycle (this delays IRQs by 1 clock for some reason, allegedly).
    *
    * If the branch is on different memory page, issue a spurious read with wrong high byte before
    * continuing at the correct address.
    */
    if (condition)
    {
        /* issue the spurious read for next insn here. */
        env->cpuRead(endian_32lo16 (Register_ProgramCounter));
        Cycle_HighByteWrongEffectiveAddress = Register_ProgramCounter & 0xff00 | Register_ProgramCounter + (int8_t) Cycle_Data & 0xff;
        Cycle_EffectiveAddress = Register_ProgramCounter + (int8_t) Cycle_Data;

        // Check for page boundary crossing
        if (Cycle_EffectiveAddress == Cycle_HighByteWrongEffectiveAddress)
        {
            cycleCount ++;
            if (irqCycle != -MAX && irqCycle != MAX)
                irqCycle += 2;
            if (nmiCycle != -MAX && nmiCycle != MAX)
                nmiCycle +=2;
        }
        Register_ProgramCounter = Cycle_EffectiveAddress;
    }
    else
    {
        /* branch not taken: skip the following spurious read insn and go to FetchNextInstr immediately. */
        interruptsAndNextOpcode ();
    }
}

void MOS6510::bcc_instr (void)
{
    branch_instr (!flagC);
}

void MOS6510::bcs_instr (void)
{
    branch_instr (flagC);
}

void MOS6510::beq_instr (void)
{
    branch_instr (flagZ);
}

void MOS6510::bit_instr (void)
{
    flagZ = (Register_Accumulator & Cycle_Data) == 0;
    flagN = Cycle_Data & 0x80;
    flagV = Cycle_Data & 0x40;
    interruptsAndNextOpcode ();
}

void MOS6510::bmi_instr (void)
{
    branch_instr (flagN);
}

void MOS6510::bne_instr (void)
{
    branch_instr (!flagZ);
}

void MOS6510::bpl_instr(void)
{
    branch_instr (!flagN);
}

void MOS6510::bvc_instr (void)
{
    branch_instr (!flagV);
}

void MOS6510::bvs_instr (void)
{
    branch_instr (flagV);
}

void MOS6510::clc_instr (void)
{
    flagC = false;
    interruptsAndNextOpcode ();
}

void MOS6510::clv_instr (void)
{
    flagV = false;
    interruptsAndNextOpcode ();
}

void MOS6510::cmp_instr (void)
{
    const uint_least16_t tmp = (uint_least16_t) Register_Accumulator - Cycle_Data;
    setFlagsNZ (tmp);
    flagC = tmp < 0x100;
    interruptsAndNextOpcode ();
}

void MOS6510::cpx_instr (void)
{
    const uint_least16_t tmp = (uint_least16_t) Register_X - Cycle_Data;
    setFlagsNZ (tmp);
    flagC = tmp < 0x100;
    interruptsAndNextOpcode ();
}

void MOS6510::cpy_instr (void)
{
    const uint_least16_t tmp = (uint_least16_t) Register_Y - Cycle_Data;
    setFlagsNZ (tmp);
    flagC = tmp < 0x100;
    interruptsAndNextOpcode ();
}

void MOS6510::dec_instr (void)
{
    PutEffAddrDataByte ();
    setFlagsNZ (--Cycle_Data);
}

void MOS6510::dex_instr (void)
{
    setFlagsNZ (--Register_X);
    interruptsAndNextOpcode ();
}

void MOS6510::dey_instr (void)
{
    setFlagsNZ (--Register_Y);
    interruptsAndNextOpcode ();
}

void MOS6510::eor_instr (void)
{
    setFlagsNZ (Register_Accumulator ^= Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::inc_instr (void)
{
    PutEffAddrDataByte ();
    setFlagsNZ (++Cycle_Data);
}

void MOS6510::inx_instr (void)
{
    setFlagsNZ (++Register_X);
    interruptsAndNextOpcode ();
}

void MOS6510::iny_instr (void)
{
    setFlagsNZ (++Register_Y);
    interruptsAndNextOpcode ();
}

void MOS6510::lda_instr (void)
{
    setFlagsNZ (Register_Accumulator = Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::ldx_instr (void)
{
    setFlagsNZ (Register_X = Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::ldy_instr (void)
{
    setFlagsNZ (Register_Y = Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::lsr_instr (void)
{
    PutEffAddrDataByte ();
    flagC = Cycle_Data & 0x01;
    setFlagsNZ (Cycle_Data >>= 1);
}

void MOS6510::lsra_instr (void)
{
    flagC = Register_Accumulator & 0x01;
    setFlagsNZ (Register_Accumulator >>= 1);
    interruptsAndNextOpcode ();
}

void MOS6510::ora_instr (void)
{
    setFlagsNZ (Register_Accumulator |= Cycle_Data);
    interruptsAndNextOpcode ();
}

void MOS6510::pla_instr (void)
{
    Register_StackPointer++;
    const uint_least16_t addr = endian_16(SP_PAGE, Register_StackPointer);
    setFlagsNZ (Register_Accumulator = env->cpuRead (addr));
}

void MOS6510::plp_instr (void)
{
    interruptsAndNextOpcode();
}

void MOS6510::rol_instr (void)
{
    const uint8_t tmp = Cycle_Data & 0x80;
    PutEffAddrDataByte ();
    Cycle_Data   <<= 1;
    if (flagC)
        Cycle_Data |= 0x01;
    setFlagsNZ (Cycle_Data);
    flagC = tmp;
}

void MOS6510::rola_instr (void)
{
    const uint8_t tmp = Register_Accumulator & 0x80;
    Register_Accumulator <<= 1;
    if (flagC)
        Register_Accumulator |= 0x01;
    setFlagsNZ (Register_Accumulator);
    flagC = tmp;
    interruptsAndNextOpcode ();
}

void MOS6510::ror_instr (void)
{
    const uint8_t tmp  = Cycle_Data & 0x01;
    PutEffAddrDataByte ();
    Cycle_Data >>= 1;
    if (flagC)
        Cycle_Data |= 0x80;
    setFlagsNZ (Cycle_Data);
    flagC = tmp;
}

void MOS6510::rora_instr (void)
{
    const uint8_t tmp = Register_Accumulator & 0x01;
    Register_Accumulator >>= 1;
    if (flagC)
        Register_Accumulator |= 0x80;
    setFlagsNZ (Register_Accumulator);
    flagC = tmp;
    interruptsAndNextOpcode ();
}

void MOS6510::sbx_instr (void)
{
    const uint tmp = (Register_X & Register_Accumulator) - Cycle_Data;
    setFlagsNZ (Register_X = tmp & 0xff);
    flagC = tmp < 0x100;
    interruptsAndNextOpcode ();
}

void MOS6510::sbc_instr (void)
{
    doSBC ();
    interruptsAndNextOpcode ();
}

void MOS6510::sec_instr (void)
{
    flagC = true;
    interruptsAndNextOpcode ();
}

void MOS6510::shs_instr (void)
{
    Register_StackPointer = Register_Accumulator & Register_X;
    Cycle_Data = (endian_16hi8 (Cycle_EffectiveAddress) + 1) & Register_StackPointer;
    if (Cycle_HighByteWrongEffectiveAddress != Cycle_EffectiveAddress)
        Cycle_EffectiveAddress = endian_16(Cycle_Data, (uint8_t)Cycle_EffectiveAddress);
    PutEffAddrDataByte ();
}

void MOS6510::tax_instr (void)
{
    setFlagsNZ (Register_X = Register_Accumulator);
    interruptsAndNextOpcode ();
}

void MOS6510::tay_instr (void)
{
    setFlagsNZ (Register_Y = Register_Accumulator);
    interruptsAndNextOpcode ();
}

void MOS6510::tsx_instr (void)
{   // Rev 1.03 (saw) - Got these tsx and txs reversed
    setFlagsNZ (Register_X = Register_StackPointer);
    interruptsAndNextOpcode ();
}

void MOS6510::txa_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_X);
    interruptsAndNextOpcode ();
}

void MOS6510::txs_instr (void)
{   // Rev 1.03 (saw) - Got these tsx and txs reversed
    Register_StackPointer = Register_X;
    interruptsAndNextOpcode ();
}

void MOS6510::tya_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_Y);
    interruptsAndNextOpcode ();
}

void MOS6510::illegal_instr (void)
{
    cycleCount --;
}


//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
// Generic Instruction Undocuemented Opcodes                               //
// See documented 6502-nmo.opc by Adam Vardy for more details              //
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//

// Undocumented - This opcode ANDs the contents of the A register with an immediate value and
// then LSRs the result.
void MOS6510::alr_instr (void)
{
    Register_Accumulator &= Cycle_Data;
    flagC = Register_Accumulator & 0x01;
    setFlagsNZ (Register_Accumulator >>= 1);
    interruptsAndNextOpcode ();
}

// Undocumented - ANC ANDs the contents of the A register with an immediate value and then
// moves bit 7 of A into the Carry flag.  This opcode works basically
// identically to AND #immed. except that the Carry flag is set to the same
// state that the Negative flag is set to.
void MOS6510::anc_instr (void)
{
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
    flagC   = flagN;
    interruptsAndNextOpcode ();
}

// Undocumented - This opcode ANDs the contents of the A register with an immediate value and
// then RORs the result (Implementation based on that of Frodo C64 Emulator)
void MOS6510::arr_instr (void)
{
    const uint8_t data = Cycle_Data & Register_Accumulator;
    Register_Accumulator = data >> 1;
    if (flagC)
        Register_Accumulator |= 0x80;

    if (flagD)
    {
        flagN = flagC;
        flagZ = Register_Accumulator == 0;
        flagV = (data ^ Register_Accumulator) & 0x40;

        if ((data & 0x0f) + (data & 0x01) > 5)
            Register_Accumulator  = (Register_Accumulator & 0xf0) | ((Register_Accumulator + 6) & 0x0f);
        flagC = ((data + (data & 0x10)) & 0x1f0) > 0x50;
        if (flagC)
            Register_Accumulator += 0x60;
    }
    else
    {
        setFlagsNZ (Register_Accumulator);
        flagC = Register_Accumulator & 0x40;
        flagV = (Register_Accumulator & 0x40) ^ ((Register_Accumulator & 0x20) << 1);
    }
    interruptsAndNextOpcode ();
}

// Undocumented - This opcode ASLs the contents of a memory location and then ORs the result
// with the accumulator.
void MOS6510::aso_instr (void)
{
    PutEffAddrDataByte ();
    flagC = Cycle_Data & 0x80;
    Cycle_Data <<= 1;
    setFlagsNZ (Register_Accumulator |= Cycle_Data);
}

// Undocumented - This opcode DECs the contents of a memory location and then CMPs the result
// with the A register.
void MOS6510::dcm_instr (void)
{
    PutEffAddrDataByte ();
    Cycle_Data--;
    const uint_least16_t tmp = (uint_least16_t) Register_Accumulator - Cycle_Data;
    setFlagsNZ (tmp);
    flagC = tmp < 0x100;
}

// Undocumented - This opcode INCs the contents of a memory location and then SBCs the result
// from the A register.
void MOS6510::ins_instr (void)
{
    PutEffAddrDataByte ();
    Cycle_Data++;
    doSBC ();
}

// Undocumented - This opcode ANDs the contents of a memory location with the contents of the
// stack pointer register and stores the result in the accumulator, the X
// register, and the stack pointer.  Affected flags: N Z.
void MOS6510::las_instr (void)
{
    setFlagsNZ (Cycle_Data &= endian_16lo8 (Register_StackPointer));
    Register_Accumulator  = Cycle_Data;
    Register_X            = Cycle_Data;
    Register_StackPointer = Cycle_Data;
    interruptsAndNextOpcode ();
}

// Undocumented - This opcode loads both the accumulator and the X register with the contents
// of a memory location.
void MOS6510::lax_instr (void)
{
    setFlagsNZ (Register_Accumulator = Register_X = Cycle_Data);
    interruptsAndNextOpcode ();
}

// Undocumented - LSE LSRs the contents of a memory location and then EORs the result with
// the accumulator.
void MOS6510::lse_instr (void)
{
    PutEffAddrDataByte ();
    flagC = Cycle_Data & 0x01;
    Cycle_Data >>= 1;
    setFlagsNZ (Register_Accumulator ^= Cycle_Data);
}

// Undocumented - This opcode ORs the A register with #xx, ANDs the result with an immediate
// value, and then stores the result in both A and X.
// xx may be EE,EF,FE, OR FF, but most emulators seem to use EE
void MOS6510::oal_instr (void)
{
    setFlagsNZ (Register_X = (Register_Accumulator = (Cycle_Data & (Register_Accumulator | 0xee))));
    interruptsAndNextOpcode ();
}

// Undocumented - RLA ROLs the contents of a memory location and then ANDs the result with
// the accumulator.
void MOS6510::rla_instr (void)
{
    const uint8_t tmp = Cycle_Data & 0x80;
    PutEffAddrDataByte ();
    Cycle_Data  = Cycle_Data << 1;
    if (flagC) Cycle_Data |= 0x01;
    flagC = tmp;
    setFlagsNZ (Register_Accumulator &= Cycle_Data);
}

// Undocumented - RRA RORs the contents of a memory location and then ADCs the result with
// the accumulator.
void MOS6510::rra_instr (void)
{
    const uint8_t tmp  = Cycle_Data & 0x01;
    PutEffAddrDataByte ();
    Cycle_Data >>= 1;
    if (flagC) Cycle_Data |= 0x80;
    flagC = tmp;
    doADC ();
}

//-------------------------------------------------------------------------//

/**
* Create new CPU emu
*
* @param context
*            The Event Context
*/
MOS6510::MOS6510 (EventContext *context)
:eventContext(*context),
 m_fdbg(stdout),
 m_nosteal("CPU-nosteal", *this, &MOS6510::eventWithoutSteals),
 m_steal("CPU-steal", *this, &MOS6510::eventWithSteals)
{
    //----------------------------------------------------------------------
    // Build up the processor instruction table
    for (int i = 0; i < 0x100; i++)
    {
#if MOS6510_DEBUG > 1
        printf ("Building Command %d[%02x]..", i, i);
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

        int buildCycle = i << 3;

        typedef enum { WRITE, READ } AccessMode;
        AccessMode access = WRITE;
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
            instrTable[buildCycle++].func = &MOS6510::throwAwayFetch;
        break;

        // Immediate and Relative Addressing Mode Handler
        case ADCb: case ANDb:  case ANCb_: case ANEb: case ASRb: case ARRb:
        case BCCr: case BCSr:  case BEQr:  case BMIr: case BNEr: case BPLr:
        case BRKn: case BVCr:  case BVSr:  case CMPb: case CPXb: case CPYb:
        case EORb: case LDAb:  case LDXb:  case LDYb: case LXAb: case NOPb_:
        case ORAb: case SBCb_: case SBXb:  case RTIn: case RTSn:
            instrTable[buildCycle++].func = &MOS6510::FetchDataByte;
        break;

        // Zero Page Addressing Mode Handler - Read & RMW
        case ADCz:  case ANDz: case BITz: case CMPz: case CPXz: case CPYz:
        case EORz:  case LAXz: case LDAz: case LDXz: case LDYz: case ORAz:
        case NOPz_: case SBCz:
        case ASLz: case DCPz: case DECz: case INCz: case ISBz: case LSRz:
        case ROLz: case RORz: case SREz: case SLOz: case RLAz: case RRAz:
            access = READ;
        case SAXz: case STAz: case STXz: case STYz:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
        break;

        // Zero Page with X Offset Addressing Mode Handler
        // these issue extra reads on the 0 page, but we don't care about it
        // because there are no detectable effects from them. These reads
        // occur during the "wasted" cycle.
        case ADCzx: case ANDzx:  case CMPzx: case EORzx: case LDAzx: case LDYzx:
        case NOPzx_: case ORAzx: case SBCzx:
        case ASLzx: case DCPzx: case DECzx: case INCzx: case ISBzx: case LSRzx:
        case RLAzx:    case ROLzx: case RORzx: case RRAzx: case SLOzx: case SREzx:
            access = READ;
        case STAzx: case STYzx:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddrX;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
        break;

        // Zero Page with Y Offset Addressing Mode Handler
        case LDXzy: case LAXzy:
            access = READ;
        case STXzy: case SAXzy:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddrY;
            // operates on 0 page in read mode. Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
        break;

        // Absolute Addressing Mode Handler
        case ADCa: case ANDa: case BITa: case CMPa: case CPXa: case CPYa:
        case EORa: case LAXa: case LDAa: case LDXa: case LDYa: case NOPa:
        case ORAa: case SBCa:
        case ASLa: case DCPa: case DECa: case INCa: case ISBa: case LSRa:
        case ROLa: case RORa: case SLOa: case SREa: case RLAa: case RRAa:
            access = READ;
        case JMPw: case SAXa: case STAa: case STXa: case STYa:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddr;
        break;

        case JSRw:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
        break;

        // Absolute With X Offset Addressing Mode Handler (Read)
        case ADCax: case ANDax:  case CMPax: case EORax: case LDAax:
        case LDYax: case NOPax_: case ORAax: case SBCax:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrX2;
            // this cycle is skipped if the address is already correct.
            // otherwise, it will be read and ignored.
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        // Absolute X (RMW; no page crossing handled, always reads before writing)
        case ASLax: case DCPax: case DECax: case INCax: case ISBax:
        case LSRax: case RLAax: case ROLax: case RORax: case RRAax:
        case SLOax: case SREax:
            access = READ;
        case SHYax: case STAax:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrX;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        // Absolute With Y Offset Addresing Mode Handler (Read)
        case ADCay: case ANDay: case CMPay: case EORay: case LASay:
        case LAXay: case LDAay: case LDXay: case ORAay: case SBCay:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrY2;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        // Absolute Y (No page crossing handled)
        case DCPay: case ISBay: case RLAay: case RRAay: case SLOay:
        case SREay:
            access = READ;
        case SHAay: case SHSay: case SHXay: case STAay:
            instrTable[buildCycle++].func = &MOS6510::FetchLowAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddrY;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        // Absolute Indirect Addressing Mode Handler
        case JMPi:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchHighPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddr;
        break;

        // Indexed with X Preinc Addressing Mode Handler
        case ADCix: case ANDix: case CMPix: case EORix: case LAXix: case LDAix:
        case ORAix: case SBCix:
        case DCPix: case ISBix: case SLOix: case SREix: case RLAix: case RRAix:
            access = READ;
        case SAXix: case STAix:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointerX;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddr;

        break;

        // Indexed with Y Postinc Addressing Mode Handler (Read)
        case ADCiy: case ANDiy: case CMPiy: case EORiy: case LAXiy:
        case LDAiy: case ORAiy: case SBCiy:
            access = READ;
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddrY2;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        // Indexed Y (No page crossing handled)
        case DCPiy: case ISBiy: case RLAiy: case RRAiy: case SLOiy:
        case SREiy:
            access = READ;
        case SHAiy: case STAiy:
            instrTable[buildCycle++].func = &MOS6510::FetchLowPointer;
            instrTable[buildCycle++].func = &MOS6510::FetchLowEffAddr;
            instrTable[buildCycle++].func = &MOS6510::FetchHighEffAddrY;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;

        break;

        default:
            legalMode = false;
        break;
        }

        if (access == READ) {
            instrTable[buildCycle++].func = &MOS6510::FetchEffAddrDataByte;
        }

        //---------------------------------------------------------------------------------------
        // Addressing Modes Finished, other cycles are instruction dependent
        switch(i)
        {
        case ADCz:  case ADCzx: case ADCa: case ADCax: case ADCay: case ADCix:
        case ADCiy: case ADCb:
            instrTable[buildCycle++].func = &MOS6510::adc_instr;
        break;

        case ANCb_:
            instrTable[buildCycle++].func = &MOS6510::anc_instr;
        break;

        case ANDz:  case ANDzx: case ANDa: case ANDax: case ANDay: case ANDix:
        case ANDiy: case ANDb:
            instrTable[buildCycle++].func = &MOS6510::and_instr;
        break;

        case ANEb: // Also known as XAA
            instrTable[buildCycle++].func = &MOS6510::ane_instr;
        break;

        case ARRb:
            instrTable[buildCycle++].func = &MOS6510::arr_instr;
        break;

        case ASLn:
            instrTable[buildCycle++].func = &MOS6510::asla_instr;
        break;

        case ASLz: case ASLzx: case ASLa: case ASLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::asl_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case ASRb: // Also known as ALR
            instrTable[buildCycle++].func = &MOS6510::alr_instr;
        break;

        case BCCr:
            instrTable[buildCycle++].func = &MOS6510::bcc_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BCSr:
            instrTable[buildCycle++].func = &MOS6510::bcs_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BEQr:
            instrTable[buildCycle++].func = &MOS6510::beq_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BITz: case BITa:
            instrTable[buildCycle++].func = &MOS6510::bit_instr;
        break;

        case BMIr:
            instrTable[buildCycle++].func = &MOS6510::bmi_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BNEr:
            instrTable[buildCycle++].func = &MOS6510::bne_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BPLr:
            instrTable[buildCycle++].func = &MOS6510::bpl_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BRKn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushHighPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushLowPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::brk_instr;
            instrTable[buildCycle++].func = &MOS6510::IRQLoRequest;
            instrTable[buildCycle++].func = &MOS6510::IRQHiRequest;
            instrTable[buildCycle++].func = &MOS6510::FetchOpcode;
        break;

        case BVCr:
            instrTable[buildCycle++].func = &MOS6510::bvc_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case BVSr:
            instrTable[buildCycle++].func = &MOS6510::bvs_instr;
            instrTable[buildCycle++].func = &MOS6510::throwAwayRead;
        break;

        case CLCn:
            instrTable[buildCycle++].func = &MOS6510::clc_instr;
        break;

        case CLDn:
            instrTable[buildCycle++].func = &MOS6510::cld_instr;
        break;

        case CLIn:
            instrTable[buildCycle++].func = &MOS6510::cli_instr;
        break;

        case CLVn:
            instrTable[buildCycle++].func = &MOS6510::clv_instr;
        break;

        case CMPz:  case CMPzx: case CMPa: case CMPax: case CMPay: case CMPix:
        case CMPiy: case CMPb:
            instrTable[buildCycle++].func = &MOS6510::cmp_instr;
        break;

        case CPXz: case CPXa: case CPXb:
            instrTable[buildCycle++].func = &MOS6510::cpx_instr;
        break;

        case CPYz: case CPYa: case CPYb:
            instrTable[buildCycle++].func = &MOS6510::cpy_instr;
        break;

        case DCPz: case DCPzx: case DCPa: case DCPax: case DCPay: case DCPix:
        case DCPiy: // Also known as DCM
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::dcm_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case DECz: case DECzx: case DECa: case DECax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::dec_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case DEXn:
            instrTable[buildCycle++].func = &MOS6510::dex_instr;
        break;

        case DEYn:
            instrTable[buildCycle++].func = &MOS6510::dey_instr;
        break;

        case EORz:  case EORzx: case EORa: case EORax: case EORay: case EORix:
        case EORiy: case EORb:
            instrTable[buildCycle++].func = &MOS6510::eor_instr;
        break;

/* HLT // Also known as JAM
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
        case 0x62: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2:
            instrTable[buildCycle++].func = hlt_instr;
        break;
*/

        case INCz: case INCzx: case INCa: case INCax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::inc_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case INXn:
            instrTable[buildCycle++].func = &MOS6510::inx_instr;
        break;

        case INYn:
            instrTable[buildCycle++].func = &MOS6510::iny_instr;
        break;

        case ISBz: case ISBzx: case ISBa: case ISBax: case ISBay: case ISBix:
        case ISBiy: // Also known as INS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::ins_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case JSRw:
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushHighPC;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushLowPC;
            instrTable[buildCycle++].func = &MOS6510::FetchHighAddr;
        case JMPw: case JMPi:
            instrTable[buildCycle++].func = &MOS6510::jmp_instr;
        break;

        case LASay:
            instrTable[buildCycle++].func = &MOS6510::las_instr;
        break;

        case LAXz: case LAXzy: case LAXa: case LAXay: case LAXix: case LAXiy:
            instrTable[buildCycle++].func = &MOS6510::lax_instr;
        break;

        case LDAz:  case LDAzx: case LDAa: case LDAax: case LDAay: case LDAix:
        case LDAiy: case LDAb:
            instrTable[buildCycle++].func = &MOS6510::lda_instr;
        break;

        case LDXz: case LDXzy: case LDXa: case LDXay: case LDXb:
            instrTable[buildCycle++].func = &MOS6510::ldx_instr;
        break;

        case LDYz: case LDYzx: case LDYa: case LDYax: case LDYb:
            instrTable[buildCycle++].func = &MOS6510::ldy_instr;
        break;

        case LSRn:
            instrTable[buildCycle++].func = &MOS6510::lsra_instr;
        break;

        case LSRz: case LSRzx: case LSRa: case LSRax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::lsr_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case NOPn_: case NOPb_:
        case NOPz_: case NOPzx_: case NOPa: case NOPax_:
        // NOPb NOPz NOPzx - Also known as SKBn
        // NOPa NOPax      - Also known as SKWn
        break;

        case LXAb: // Also known as OAL
            instrTable[buildCycle++].func = &MOS6510::oal_instr;
        break;

        case ORAz:  case ORAzx: case ORAa: case ORAax: case ORAay: case ORAix:
        case ORAiy: case ORAb:
            instrTable[buildCycle++].func = &MOS6510::ora_instr;
        break;

        case PHAn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::pha_instr;
        break;

        case PHPn:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PushSR;
        break;

        case PLAn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::pla_instr;
        break;

        case PLPn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopSR;
            instrTable[buildCycle++].func = &MOS6510::plp_instr;
        break;

        case RLAz: case RLAzx: case RLAix: case RLAa: case RLAax: case RLAay:
        case RLAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rla_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case ROLn:
            instrTable[buildCycle++].func = &MOS6510::rola_instr;
        break;

        case ROLz: case ROLzx: case ROLa: case ROLax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rol_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case RORn:
            instrTable[buildCycle++].func = &MOS6510::rora_instr;
        break;

        case RORz: case RORzx: case RORa: case RORax:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::ror_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case RRAa: case RRAax: case RRAay: case RRAz: case RRAzx: case RRAix:
        case RRAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::rra_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case RTIn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopSR;
            instrTable[buildCycle++].func = &MOS6510::PopLowPC;
            instrTable[buildCycle++].func = &MOS6510::PopHighPC;
            instrTable[buildCycle++].func = &MOS6510::rti_instr;
        break;

        case RTSn:
            // should read the value at current stack register.
            // Truly side-effect free.
            instrTable[buildCycle++].func = &MOS6510::WasteCycle;
            instrTable[buildCycle++].func = &MOS6510::PopLowPC;
            instrTable[buildCycle++].func = &MOS6510::PopHighPC;
            instrTable[buildCycle++].func = &MOS6510::rts_instr;
        break;

        case SAXz: case SAXzy: case SAXa: case SAXix: // Also known as AXS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::axs_instr;
        break;

        case SBCz:  case SBCzx: case SBCa: case SBCax: case SBCay: case SBCix:
        case SBCiy: case SBCb_:
            instrTable[buildCycle++].func = &MOS6510::sbc_instr;
        break;

        case SBXb:
            instrTable[buildCycle++].func = &MOS6510::sbx_instr;
        break;

        case SECn:
            instrTable[buildCycle++].func = &MOS6510::sec_instr;
        break;

        case SEDn:
            instrTable[buildCycle++].func = &MOS6510::sed_instr;
        break;

        case SEIn:
            instrTable[buildCycle++].func = &MOS6510::sei_instr;
        break;

        case SHAay: case SHAiy: // Also known as AXA
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::axa_instr;
        break;

        case SHSay: // Also known as TAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::shs_instr;
        break;

        case SHXay: // Also known as XAS
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::xas_instr;
        break;

        case SHYax: // Also known as SAY
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::say_instr;
        break;

        case SLOz: case SLOzx: case SLOa: case SLOax: case SLOay: case SLOix:
        case SLOiy: // Also known as ASO
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::aso_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case SREz: case SREzx: case SREa: case SREax: case SREay: case SREix:
        case SREiy: // Also known as LSE
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::lse_instr;
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::PutEffAddrDataByte;
        break;

        case STAz: case STAzx: case STAa: case STAax: case STAay: case STAix:
        case STAiy:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::sta_instr;
        break;

        case STXz: case STXzy: case STXa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::stx_instr;
        break;

        case STYz: case STYzx: case STYa:
            instrTable[buildCycle].nosteal = true;
            instrTable[buildCycle++].func = &MOS6510::sty_instr;
        break;

        case TAXn:
            instrTable[buildCycle++].func = &MOS6510::tax_instr;
        break;

        case TAYn:
            instrTable[buildCycle++].func = &MOS6510::tay_instr;
        break;

        case TSXn:
            instrTable[buildCycle++].func = &MOS6510::tsx_instr;
        break;

        case TXAn:
            instrTable[buildCycle++].func = &MOS6510::txa_instr;
        break;

        case TXSn:
            instrTable[buildCycle++].func = &MOS6510::txs_instr;
        break;

        case TYAn:
            instrTable[buildCycle++].func = &MOS6510::tya_instr;
        break;

        default:
            legalInstr = false;
        break;
        }

        /* Missing an addressing mode or implementation makes opcode invalid.
        * These are normally called HLT instructions. In the hardware, the
        * CPU state machine locks up and will never recover. */
        if (!(legalMode && legalInstr))
        {
            instrTable[buildCycle++].func = &MOS6510::illegal_instr;
        }

        /* check for IRQ triggers or fetch next opcode... */
        instrTable[buildCycle].func = &MOS6510::interruptsAndNextOpcode;

#if MOS6510_DEBUG > 1
        printf (".");
#endif

#if MOS6510_DEBUG > 1
        printf ("Done [%d Cycles]\n", buildCycle);
#endif
    }

    //----------------------------------------------------------------------
    // Build interrupts
    for (int i = 0; i < 3; i++)
    {
#if MOS6510_DEBUG > 1
        printf ("Building Interrupt %d[%02x]..", i, i);
#endif

        const int offset = i + 0x100;
        int buildCycle = offset << 3;

        /* common interrupt handling lead-in:
        * 2x read from PC, store pc hi, pc lo, sr to stack.
        */
        instrTable[buildCycle++].func = &MOS6510::throwAwayFetch;
        instrTable[buildCycle++].func = &MOS6510::throwAwayFetch;
        instrTable[buildCycle].nosteal = true;
        instrTable[buildCycle++].func = &MOS6510::PushHighPC;
        instrTable[buildCycle].nosteal = true;
        instrTable[buildCycle++].func = &MOS6510::PushLowPC;
        instrTable[buildCycle].nosteal = true;
        instrTable[buildCycle++].func = &MOS6510::IRQRequest;

        switch (offset)
        {
        case oRST:
            instrTable[buildCycle++].func = &MOS6510::RSTLoRequest;
            instrTable[buildCycle++].func = &MOS6510::RSTHiRequest;
        break;

        case oNMI:
            instrTable[buildCycle++].func = &MOS6510::NMILoRequest;
            instrTable[buildCycle++].func = &MOS6510::NMIHiRequest;
        break;

        case oIRQ:
            instrTable[buildCycle++].func = &MOS6510::IRQLoRequest;
            instrTable[buildCycle++].func = &MOS6510::IRQHiRequest;
        break;
        }

        instrTable[buildCycle].func = &MOS6510::FetchOpcode;

#if MOS6510_DEBUG > 1
        printf (".");
#endif

#if MOS6510_DEBUG > 1
        printf ("Done [%d Cycles]\n", buildCycle);
#endif
    }

    // Intialise Processor Registers
    Register_Accumulator   = 0;
    Register_X             = 0;
    Register_Y             = 0;

    Cycle_EffectiveAddress = 0;
    Cycle_Data             = 0;

    dodump = false;
    Initialise ();
}

/**
* Initialise CPU Emulation (Registers)
*/
void MOS6510::Initialise (void)
{
    // Reset stack
    Register_StackPointer = 0xFF;

    // Reset Cycle Count
    cycleCount = (NOPn << 3) + 1;

    // Reset Status Register
    flagB = true;
    flagN = flagC = flagD = flagV = flagZ = flagI = false;

    // Set PC to some value
    Register_ProgramCounter = 0;

    // IRQs pending check
    irqAsserted = false;
    irqCycle = MAX;
    nmiCycle = MAX;

    // Signals
    rdy = true;

    eventContext.schedule (m_nosteal, 0, EVENT_CLOCK_PHI2);
}

/**
* Reset CPU Emulation
*/
void MOS6510::reset (void)
{   // Internal Stuff
    Initialise ();

    // Requires External Bits
    // Read from reset vector for program entry point
    endian_16lo8 (Cycle_EffectiveAddress, env->cpuRead (0xFFFC));
    endian_16hi8 (Cycle_EffectiveAddress, env->cpuRead (0xFFFD));
    Register_ProgramCounter = Cycle_EffectiveAddress;
//    filepos = 0;
}

//-------------------------------------------------------------------------//
// Module Credits                                                          //
void MOS6510::credits (char *sbuffer)
{   // Copy credits to buffer
    strcat (sbuffer, "MOS6510 Cycle Exact Emulation\n");
    strcat (sbuffer, "\t(C) 2000 Simon A. White\n");
    strcat (sbuffer, "\t(C) 2008-2010 Antti S. Lankila\n");
    strcat (sbuffer, "\t(C) 2011-2012 Leandro Nini\n");
}

void MOS6510::debug (const bool enable, FILE *out)
{
    dodump = enable;
    if (!(out && enable))
        m_fdbg = stdout;
    else
        m_fdbg = out;
}
