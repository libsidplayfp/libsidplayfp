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

#ifndef MOS6510_H
#define MOS6510_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>

#include "sidplayfp/sidendian.h"
#include "sidplayfp/component.h"
#include "sidplayfp/EventScheduler.h"

/** @internal
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
    friend class MOS6510Debug;
private:
	static const char *credit;

private:
    /**
    * IRQ/NMI magic limit values.
    * Need to be larger than about 0x103 << 3,
    * but can't be min/max for Integer type.
    */
    static const int MAX = 65536;

    /** Stack page location */
    static const uint8_t SP_PAGE = 0x01;

public:
    /** Status register interrupt bit. */
    static const int SR_INTERRUPT = 2;

protected:
    struct ProcessorCycle
    {
        void (MOS6510::*func)(void);
        bool nosteal;
        ProcessorCycle ()
            :func(0), nosteal(false) {}
    };

protected:
    /** Our event context copy. */
    EventContext &eventContext;

    /** Current instruction and subcycle within instruction */
    int cycleCount;

    /** IRQ asserted on CPU */
    bool irqAssertedOnPin;

     /** When IRQ was triggered. -MAX means "during some previous instruction", MAX means "no IRQ" */
    int interruptCycle;

    /** NMI requested? */
    bool nmiFlag;

    /** RST requested? */
    bool rstFlag;

    /** RDY pin state (stop CPU on read) */
    bool rdy;

    bool flagN;
    bool flagC;
    bool flagD;
    bool flagZ;
    bool flagV;
    bool flagI;
    bool flagB;

    /** Data regarding current instruction */
    uint_least16_t Register_ProgramCounter;
    uint_least16_t Cycle_EffectiveAddress;
    uint_least16_t Cycle_HighByteWrongEffectiveAddress;
    uint_least16_t Cycle_Pointer;

    uint8_t Cycle_Data;
    uint8_t Register_StackPointer;
    uint8_t Register_Accumulator;
    uint8_t Register_X;
    uint8_t Register_Y;

#ifdef DEBUG
    /** Debug info */
    uint_least16_t instrStartPC;
    uint_least16_t instrOperand;

    FILE *m_fdbg;

    bool dodump;
#endif

    /** Table of CPU opcode implementations */
    struct ProcessorCycle  instrTable[0x101 << 3];

protected:
    /** Represents an instruction subcycle that writes */
    EventCallback<MOS6510> m_nosteal;

    /** Represents an instruction subcycle that reads */
    EventCallback<MOS6510> m_steal;

    void eventWithoutSteals  (void);
    void eventWithSteals     (void);

    void Initialise          (void);

    // Flag utility functions
    inline void setFlagsNZ(const uint8_t value);
    inline uint8_t getStatusRegister(void);
    inline void setStatusRegister(const uint8_t sr);

    // Declare Interrupt Routines
    inline void IRQLoRequest     (void);
    inline void IRQHiRequest     (void);
    inline void interruptsAndNextOpcode (void);
    inline void calculateInterruptTriggerCycle();

    // Declare Instruction Routines
    inline void fetchNextOpcode      (void);
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
    inline void PushSR               (void);
    inline void PopLowPC             (void);
    inline void PopHighPC            (void);
    inline void PopSR                (void);
    inline void brkPushLowPC         (void);
    inline void WasteCycle           (void);

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
    inline void tax_instr     (void);
    inline void tay_instr     (void);
    inline void tsx_instr     (void);
    inline void txa_instr     (void);
    inline void txs_instr     (void);
    inline void tya_instr     (void);
    inline void xas_instr     (void);
    void        illegal_instr (void);

    // Declare Arithmetic Operations
    inline void doADC   (void);
    inline void doSBC   (void);

    inline void doJSR   (void);

    /**
    * Get data from system environment
    *
    * @param address
    * @return data byte CPU requested
    */
    virtual uint8_t cpuRead(const uint_least16_t addr) =0;

    /**
    * Write data to system environment
    *
    * @param address
    * @param data
    */
    virtual void cpuWrite(const uint_least16_t addr, const uint8_t data) =0;

#ifdef PC64_TESTSUITE
    virtual void   loadFile (const char *file) =0;
#endif

public:
    MOS6510 (EventContext *context);
    virtual ~MOS6510 () {}
    virtual void reset     (void);
    const char  *credits(void) { return credit; }
    void         debug     (const bool enable, FILE *out);
    void         setRDY    (const bool newRDY);

    // Non-standard functions
    void triggerRST (void);
    void triggerNMI (void);
    void triggerIRQ (void);
    void clearIRQ   (void);
};

#endif // MOS6510_H
