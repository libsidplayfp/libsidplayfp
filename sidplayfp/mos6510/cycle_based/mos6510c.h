/***************************************************************************
                          mos6510c.h  -  Cycle Accurate 6510 Emulation
                             -------------------
    begin                : Thu May 11 2000
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
 *  $Log: mos6510c.h,v $
 *  Revision 1.26  2004/06/26 11:10:47  s_a_white
 *  Changes to support new calling convention for event scheduler.
 *
 *  Revision 1.25  2004/04/23 01:01:37  s_a_white
 *  Added debug clock to record cycle instructions starts on.
 *
 *  Revision 1.24  2004/03/07 22:38:57  s_a_white
 *  Rename write to nosteal since it needs setting for non memory access cycles to
 *
 *  Revision 1.23  2004/03/06 21:07:12  s_a_white
 *  Don't start a new cycle stealing sequence if one is already started!  This can
 *  happen if an interrupt occurs during an optimised sleep.
 *
 *  Revision 1.22  2004/02/29 14:33:59  s_a_white
 *  If an interrupt occurs during a branch instruction but after the decision
 *  has occured then it should not be delayed.
 *
 *  Revision 1.21  2003/10/29 22:18:03  s_a_white
 *  IRQs are now only taken in on phase 1 as previously they could be clocked
 *  in on both phases of the cycle resulting in them sometimes not being
 *  delayed long enough.
 *
 *  Revision 1.20  2003/10/28 00:22:53  s_a_white
 *  getTime now returns a time with respect to the clocks desired phase.
 *
 *  Revision 1.19  2003/10/16 07:48:32  s_a_white
 *  Allow redirection of debug information of file.
 *
 *  Revision 1.18  2003/01/20 23:10:48  s_a_white
 *  Fixed RMW instructions to perform memory re-write on the correct cycle.
 *
 *  Revision 1.17  2003/01/20 18:37:08  s_a_white
 *  Stealing update.  Apparently the cpu does a memory read from any non
 *  write cycle (whether it needs to or not) resulting in those cycles
 *  being stolen.
 *
 *  Revision 1.16  2003/01/17 08:42:09  s_a_white
 *  Event scheduler phase support.  Better handling the operation of IRQs
 *  during stolen cycles.
 *
 *  Revision 1.15  2002/11/28 20:35:06  s_a_white
 *  Reduced number of thrown exceptions when dma occurs.
 *
 *  Revision 1.14  2002/11/25 20:10:55  s_a_white
 *  A bus access failure should stop the CPU dead like the cycle never started.
 *  This is currently simulated using throw (execption handling) for now.
 *
 *  Revision 1.13  2002/11/21 19:52:48  s_a_white
 *  CPU upgraded to be like other components.  Theres nolonger a clock call,
 *  instead events are registered to occur at a specific time.
 *
 *  Revision 1.12  2002/11/19 22:57:33  s_a_white
 *  Initial support for external DMA to steal cycles away from the CPU.
 *
 *  Revision 1.11  2002/11/01 17:35:27  s_a_white
 *  Frame based support for old sidplay1 modes.
 *
 *  Revision 1.10  2001/08/05 15:46:02  s_a_white
 *  No longer need to check on which cycle an instruction ends or when to print
 *  debug information.
 *
 *  Revision 1.9  2001/07/14 16:48:03  s_a_white
 *  cycleCount and related must roject.Syn
 *
 *  Revision 1.8  2001/07/14 13:15:30  s_a_white
 *  Accumulator is now unsigned, which improves code readability.  Emulation
 *  tested with testsuite 2.15.  Various instructions required modification.
 *
 *  Revision 1.7  2001/03/28 21:17:34  s_a_white
 *  Added support for proper RMW instructions.
 *
 *  Revision 1.6  2001/03/24 18:09:17  s_a_white
 *  On entry to interrupt routine the first instruction in the handler is now always
 *  executed before pending interrupts are re-checked.
 *
 *  Revision 1.5  2001/03/19 23:48:21  s_a_white
 *  Interrupts made virtual to allow for redefintion for Sidplay1 compatible
 *  interrupts.
 *
 *  Revision 1.4  2001/03/09 22:28:03  s_a_white
 *  Speed optimisation update.
 *
 *  Revision 1.3  2001/02/13 21:03:33  s_a_white
 *  Changed inlines to non-inlines due to function bodies not being in header.
 *
 *  Revision 1.2  2000/12/11 19:04:32  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _mos6510c_h_
#define _mos6510c_h_

#include <stdio.h>
#include <stdint.h>

#include "sidplayfp/sidendian.h"

/** @internal
/**
* Cycle-exact 6502/6510 emulation core.
*
* Code is based on work by Simon A. White <sidplay2@yahoo.com>.
* Original Java port by Ken Händel. Later on, it has been hacked to
* improve compatibility with Lorenz suite on VICE's test suite.
*
* @author alankila
*/
class MOS6510
{
private:
    /**
    * IRQ/NMI magic limit values.
    * Need to be larger than about 0x103 << 3,
    * but can't be min/max for Integer type.
    */
    static const int MAX = 65536;

protected:
    struct ProcessorCycle
    {
        void (MOS6510::*func)(void);
        bool nosteal;
        ProcessorCycle ()
            :func(0), nosteal(false) {}
    };

protected:
    C64Environment *env;

    /** Our event context copy. */
    EventContext &eventContext;

    /** Current instruction and subcycle within instruction */
    int cycleCount;

    /** Pointers to the current instruction cycle */
    uint_least16_t Cycle_EffectiveAddress;
    uint_least16_t Cycle_HighByteWrongEffectiveAddress;
    uint8_t        Cycle_Data;
    uint_least16_t Cycle_Pointer;

    uint8_t        Register_Accumulator;
    uint8_t        Register_X;
    uint8_t        Register_Y;
    uint_least32_t Register_ProgramCounter;
    bool           flagN;
    bool           flagC;
    bool           flagD;
    bool           flagZ;
    bool           flagV;
    bool           flagI;
    bool           flagB;
    uint_least16_t Register_StackPointer;

    /* Interrupts */

    /** IRQ asserted on CPU */
     bool irqAsserted;

     /** When IRQ was triggered. -MAX means "during some previous instruction", MAX means "no IRQ" */
    int  irqCycle;

    /** When NMI was triggered. -MAX means "during some previous instruction", MAX means "no IRQ" */
    int  nmiCycle;

    /** Address Controller, blocks reads */
    bool rdy;

    /** Debug info */
    uint_least16_t instrStartPC;
    uint_least16_t instrOperand;

    FILE *m_fdbg;

    event_clock_t m_dbgClk;

    bool dodump;

    /** Table of CPU opcode implementations */
    struct ProcessorCycle  instrTable[0x103 << 3];

protected:
    EventCallback<MOS6510> m_nosteal;
    EventCallback<MOS6510> m_steal;

    void eventWithoutSteals  (void);
    void eventWithSteals     (void);

    void Initialise          (void);

    // Flag utility functions
    inline void setFlagsNZ(const uint8_t value);
    inline uint8_t getStatusRegister(void);
    inline void setStatusRegister(const uint8_t sr);

    // Declare Interrupt Routines
    inline void RSTLoRequest     (void);
    inline void RSTHiRequest     (void);
    inline void NMILoRequest     (void);
    inline void NMIHiRequest     (void);
    inline void IRQRequest       (void);
    inline void IRQLoRequest     (void);
    inline void IRQHiRequest     (void);
    void interruptsAndNextOpcode (void);

    // Declare Instruction Routines
    virtual void FetchOpcode         (void);
    inline void throwAwayFetch       (void);
    inline void throwAwayRead        (void);
    inline void FetchDataByte        (void);
    inline void FetchLowAddr         (void);
    inline void FetchLowAddrX        (void);
    inline void FetchLowAddrY        (void);
    inline void FetchHighAddr        (void);
    inline void FetchHighAddrX       (void);
    inline void FetchHighAddrX2      (void);
    inline void FetchHighAddrY       (void);
    inline void FetchHighAddrY2      (void);
    inline void FetchLowEffAddr      (void);
    inline void FetchHighEffAddr     (void);
    inline void FetchHighEffAddrY    (void);
    inline void FetchHighEffAddrY2   (void);
    inline void FetchLowPointer      (void);
    inline void FetchLowPointerX     (void);
    inline void FetchHighPointer     (void);
    inline void FetchEffAddrDataByte (void);
    inline void PutEffAddrDataByte   (void);
    inline void PushLowPC            (void);
    inline void PushHighPC           (void);
    inline void PushSR               (const bool b_flag);
    inline void PushSR               (void);
    inline void PopLowPC             (void);
    inline void PopHighPC            (void);
    inline void PopSR                (void);
    inline void WasteCycle           (void);
    inline void DebugCycle           (void);

    // Delcare Instruction Operation Routines
    inline void adc_instr     (void);
    inline void alr_instr     (void);
    inline void anc_instr     (void);
    inline void and_instr     (void);
    inline void ane_instr     (void);
    inline void arr_instr     (void);
    inline void asl_instr     (void);
    inline void asla_instr    (void);
    inline void aso_instr     (void);
    inline void axa_instr     (void);
    inline void axs_instr     (void);
    inline void bcc_instr     (void);
    inline void bcs_instr     (void);
    inline void beq_instr     (void);
    inline void bit_instr     (void);
    inline void bmi_instr     (void);
    inline void bne_instr     (void);
    inline void branch_instr  (const bool condition);
    inline void bpl_instr     (void);
    inline void brk_instr     (void);
    inline void bvc_instr     (void);
    inline void bvs_instr     (void);
    inline void clc_instr     (void);
    inline void cld_instr     (void);
    inline void cli_instr     (void);
    inline void clv_instr     (void);
    inline void cmp_instr     (void);
    inline void cpx_instr     (void);
    inline void cpy_instr     (void);
    inline void dcm_instr     (void);
    inline void dec_instr     (void);
    inline void dex_instr     (void);
    inline void dey_instr     (void);
    inline void eor_instr     (void);
    inline void inc_instr     (void);
    inline void ins_instr     (void);
    inline void inx_instr     (void);
    inline void iny_instr     (void);
    inline void jmp_instr     (void);
    inline void las_instr     (void);
    inline void lax_instr     (void);
    inline void lda_instr     (void);
    inline void ldx_instr     (void);
    inline void ldy_instr     (void);
    inline void lse_instr     (void);
    inline void lsr_instr     (void);
    inline void lsra_instr    (void);
    inline void oal_instr     (void);
    inline void ora_instr     (void);
    inline void pha_instr     (void);
    inline void pla_instr     (void);
    inline void plp_instr     (void);
    inline void rla_instr     (void);
    inline void rol_instr     (void);
    inline void rola_instr    (void);
    inline void ror_instr     (void);
    inline void rora_instr    (void);
    inline void rra_instr     (void);
    inline void rti_instr     (void);
    inline void rts_instr     (void);
    inline void sbx_instr     (void);
    inline void say_instr     (void);
    inline void sbc_instr     (void);
    inline void sec_instr     (void);
    inline void sed_instr     (void);
    inline void sei_instr     (void);
    inline void shs_instr     (void);
    inline void sta_instr     (void);
    inline void stx_instr     (void);
    inline void sty_instr     (void);
    inline void tas_instr     (void);
    inline void tax_instr     (void);
    inline void tay_instr     (void);
    inline void tsx_instr     (void);
    inline void txa_instr     (void);
    inline void txs_instr     (void);
    inline void tya_instr     (void);
    inline void xas_instr     (void);
    void        illegal_instr (void);

    // Declare Arithmatic Operations
    inline void doADC   (void);
    inline void doSBC   (void);

public:
    MOS6510 (EventContext *context);
    virtual ~MOS6510 () {}
    virtual void reset     (void);
    virtual void credits   (char *str);
    virtual void DumpState (void);
    void         debug     (const bool enable, FILE *out);
    void         setRDY    (const bool state);
    void         setEnvironment(C64Environment *env) { this->env = env; }

    // Non-standard functions
    virtual void triggerRST (void);
    virtual void triggerNMI (void);
    virtual void triggerIRQ (void);
    void         clearIRQ   (void);
};

#endif // _mos6510c_h_
