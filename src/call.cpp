// CPU operations adapted from Mihály 'Hermit' Horváth, cRSID.
// Original permission: libcRSID.c, SID-Wizard 1.93, 2022.
#include "call.h"
template <bool BasicRam, bool Colours = false, bool Banks = false>
static int execute_player_call(PlayerCall* call, bool basicRam = false,
                               PlayerCallBank* bank = nullptr)
{ //the CPU emulation for SID/PRG playback (ToDo: RESET vector, complete BCD-mode)

    enum StatusFlagBitValues
    {
        N = 0x80,
        V = 0x40,
        B = 0x10,
        D = 0x08,
        I = 0x04,
        Z = 0x02,
        C = 0x01
    };

    static const uint8_t FlagSwitches[] = {0x01, 0x21, 0x04, 0x24, 0x00, 0x40, 0x08, 0x28},
                         BranchFlags[] = {0x80, 0x40, 0x01, 0x02};

    uint8_t* memory = call->memory;
    signed char Cycles, SamePage;
    uint8_t IR, ST = call->p, X = call->x, Y = call->y;
    short A = call->a, SP = call->sp, T;
    unsigned PC = call->pc, Addr, PrevPC;
    unsigned stop =
        ((memory[0x100 | (uint8_t)(SP + 1)] | (memory[0x100 | (uint8_t)(SP + 2)] << 8)) + 1) &
        65535;
    unsigned finalSP = (SP + 2) & 255;
    CallWrite pending[2];
    unsigned pendingCount = 0;
    const unsigned ramLimit = (BasicRam || (Colours && basicRam)) ? 0xd000 : 0xa000;
    const auto io = [&]() -> bool { return (bank->data & 3) != 0; };
    const auto safe = [&](unsigned a) -> int
    {
        if (Banks)
        {
            if (a < 2)
                return false;
            if (a >= 0xa000 && a < 0xc000 && (bank->data & 3) == 3)
                return false;
            if (a >= 0xe000 && (bank->data & 2))
                return false;
            if (a >= 0xd000 && a < 0xe000 && io())
                return false;
            return true;
        }
        return (a >= 2 && a < ramLimit) || (a >= 0xc000 && a < 0xd000);
    };
    const auto rd = [&](uint16_t a) -> uint8_t
    {
        if (static_cast<unsigned>(a - 2) < 0x9ffe)
            return memory[a];
        if (Banks)
        {
            if (a == 0)
                return bank->dir;
            if (a == 1)
                return (bank->data & bank->dir) | (bank->pins & ~bank->dir);
            if (a >= 0xd020 && a < 0xd02f && io())
                return bank->colours[a - 0xd020] | 0xf0;
        }
        else if (Colours && a >= 0xd020 && a < 0xd02f)
            return memory[a] | 0xf0;
        if (!safe(a))
            call->error = 1;
        return memory[a];
    };
    const auto wr = [&](uint16_t a, uint8_t v) -> void
    {
        uint8_t kind = 0, old = memory[a];
        if (Banks)
        {
            if (a == 0)
            {
                call->error = 1;
                return;
            }
            if (a == 1)
            {
                if (IR != 0x85 || v < 0x34 || v > 0x37)
                {
                    call->error = 1;
                    return;
                }
                bank->data = v;
                bank->seed = bank->seed * 1664525u + 1013904223u;
                kind = 3;
            }
            else if (a >= 0xd000 && a < 0xe000 && io())
            {
                if (a >= 0xd400 && a < 0xd419)
                {
                    kind = 1;
                    old = bank->sid[a - 0xd400];
                }
                else if (a >= 0xd020 && a < 0xd02f)
                {
                    kind = 2;
                    old = bank->colours[a - 0xd020] | 0xf0;
                }
                else
                {
                    call->error = 1;
                    return;
                }
            }
        }
        else
        {
            if (!safe(a) && !(a >= 0xd400 && a < 0xd480) && !(Colours && a >= 0xd020 && a < 0xd02f))
            {
                call->error = 1;
                return;
            }
            if (Colours && a >= 0xd020 && a < 0xd02f)
                old |= 0xf0;
        }
        if (pendingCount == 2)
        {
            call->error = 1;
            return;
        }
        if ((IR & 3) == 2 && (IR & 0x0f) != 0x0a && (IR >> 5) != 4 && (IR >> 5) != 5)
            pending[pendingCount++] = CallWrite{0, a, old, kind};
        if (pendingCount == 2)
        {
            call->error = 1;
            return;
        }
        pending[pendingCount++] = CallWrite{0, a, v, kind};
        if (Banks)
        {
            if (kind == 1)
                bank->sid[a - 0xd400] = v;
            else if (kind == 2)
                bank->colours[a - 0xd020] = v;
            else
                memory[a] = kind == 3 ? bank->seed & 255 : v;
        }
        else
            memory[a] = v;
    };
    const auto wr2 = [&](uint16_t a, uint8_t v) -> void { wr(a, v); };
    const auto addrModeImmediate = [&](void) -> void
    {
        ++PC;
        Addr = PC;
        Cycles = 2;
    }; //imm.
    const auto addrModeZeropage = [&](void) -> void
    {
        ++PC;
        Addr = rd(PC);
        Cycles = 3;
    }; //zp
    const auto addrModeAbsolute = [&](void) -> void
    {
        ++PC;
        Addr = rd(PC);
        ++PC;
        Addr += rd(PC) << 8;
        Cycles = 4;
    }; //abs
    const auto addrModeZeropageXindexed = [&](void) -> void
    {
        ++PC;
        Addr = (rd(PC) + X) & 0xFF;
        Cycles = 4;
    }; //zp,x (with zeropage-wraparound of 6502)
    const auto addrModeZeropageYindexed = [&](void) -> void
    {
        ++PC;
        Addr = (rd(PC) + Y) & 0xFF;
        Cycles = 4;
    }; //zp,y (with zeropage-wraparound of 6502)

    const auto addrModeXindexed = [&](void) -> void { // abs,x (only STA is 5 cycles, others are 4 if page not crossed, RMW:7)
  ++PC; Addr = rd(PC) + X; ++PC; SamePage = (Addr <= 0xFF); Addr += rd(PC)<<8; Cycles=5;
 };

    const auto addrModeYindexed = [&](void) -> void { // abs,y (only STA is 5 cycles, others are 4 if page not crossed, RMW:7)
  ++PC; Addr = rd(PC) + Y; ++PC; SamePage = (Addr <= 0xFF); Addr += rd(PC)<<8; Cycles=5;
 };

    const auto addrModeIndirectYindexed = [&](void) -> void { // (zp),y (only STA is 6 cycles, others are 5 if page not crossed, RMW:8)
  ++PC; Addr = rd(rd(PC)) + Y; SamePage = (Addr <= 0xFF); Addr += rd( (rd(PC)+1)&0xFF ) << 8; Cycles=6;
 };

    const auto addrModeXindexedIndirect = [&](void) -> void { // (zp,x)
  ++PC; Addr = ( rd((rd(PC)+X)&255)&0xFF ) + ( ( rd((rd(PC)+X+1)&255)&0xFF ) << 8 ); Cycles=6;
 };

    const auto clrC = [&](void) -> void { ST &= ~C; }; //clear Carry-flag
    const auto setC = [&](uint8_t expr) -> void
    {
        ST &= ~C;
        ST |= (expr != 0);
    }; //set Carry-flag if expression is not zero
    const auto clrNZC = [&](void) -> void { ST &= ~(N | Z | C); };      //clear flags
    const auto clrNVZC = [&](void) -> void { ST &= ~(N | V | Z | C); }; //clear flags
    const auto setNZbyA = [&](void) -> void
    {
        ST &= ~(N | Z);
        ST |= ((!A) << 1) | (A & N);
    }; //set Negative-flag and Zero-flag based on result in Accumulator
    const auto setNZbyT = [&](void) -> void
    {
        T &= 0xFF;
        ST &= ~(N | Z);
        ST |= ((!T) << 1) | (T & N);
    };
    const auto setNZbyX = [&](void) -> void
    {
        ST &= ~(N | Z);
        ST |= ((!X) << 1) | (X & N);
    }; //set Negative-flag and Zero-flag based on result in X-register
    const auto setNZbyY = [&](void) -> void
    {
        ST &= ~(N | Z);
        ST |= ((!Y) << 1) | (Y & N);
    }; //set Negative-flag and Zero-flag based on result in Y-register
    const auto setNZCbyAdd = [&](void) -> void
    {
        ST &= ~(N | Z | C);
        ST |= (A & N) | (A > 255);
        A &= 0xFF;
        ST |= (!A) << 1;
    }; //after increase/addition
    const auto setVbyAdd = [&](uint8_t M) -> void
    {
        ST &= ~V;
        ST |= ((~(T ^ M)) & (T ^ A) & N) >> 1;
    }; //calculate V-flag from A and T (previous A) and input2 (Memory)
    const auto setNZCbySub = [&](signed short* obj) -> void
    {
        ST &= ~(N | Z | C);
        ST |= (*obj & N) | (*obj >= 0);
        *obj &= 0xFF;
        ST |= ((!(*obj)) << 1);
    };

    const auto push = [&](uint8_t value) -> void
    {
        wr(0x100 + SP, value);
        --SP;
        SP &= 0xFF;
    }; //push a value to stack
    const auto pop = [&](void) -> uint8_t
    {
        ++SP;
        SP &= 0xFF;
        return memory[0x100 + SP];
    }; //pop a value from stack

    call->cycles = call->count = call->error = 0;
    unsigned executed = 0;
nextInstruction:
    if (PC == stop && SP == finalSP)
    {
        call->pc = PC;
        call->a = A;
        call->x = X;
        call->y = Y;
        call->p = ST;
        call->sp = SP;
        return !call->error;
    }
    if (++executed == 10000 || call->cycles > CALL_CYCLES - 10 || call->count > CALL_ACTIONS - 4 ||
        call->error)
        return 0;
    pendingCount = 0;
    PrevPC = PC;
    static const uint8_t allowed[256] = {
        0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0,
        1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1,
        0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1,
        0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0,
        0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
        1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
        1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1,
        1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0,
        1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0};
    IR = rd(PC);
    if ((!allowed[IR] && !(Banks && IR == 0xf8)) || (Banks ? !(ST & 4) : (ST & 12) != 4))
    {
        call->error = 256 + IR;
        return 0;
    }
    Cycles = 2;
    SamePage = 0; //if(PC<0xE000) //if(PC>=0x5e8e && PC<=0x5F19)
    //printf("%4.4X-%2.2X \n",PC,IR); //'Cycles': ensure smallest 6510 runtime (for implied/instructions)
    //wr(0xE000+(PC&0x3FF),0x5A);

    if (IR & 1)
    { //nybble2:  1/5/9/D:accu.instructions, 3/7/B/F:illegal opcodes

        switch ((IR & 0x1F) >> 1)
        { //value-forming to cause jump-table //PC wraparound not handled inside to save codespace
        case 0:
        case 1:
            addrModeXindexedIndirect();
            break; //(zp,x)
        case 2:
        case 3:
            addrModeZeropage();
            break;
        case 4:
        case 5:
            addrModeImmediate();
            break;
        case 6:
        case 7:
            addrModeAbsolute();
            break;
        case 8:
        case 9:
            addrModeIndirectYindexed();
            break; //(zp),y (5..6 cycles, 8 for R-M-W)
        case 0xA:
            addrModeZeropageXindexed();
            break; //zp,x
        case 0xB:
            if ((IR & 0xC0) != 0x80)
                addrModeZeropageXindexed(); //zp,x for illegal opcodes
            else
                addrModeZeropageYindexed(); //zp,y for LAX/SAX illegal opcodes
            break;
        case 0xC:
        case 0xD:
            addrModeYindexed();
            break;
        case 0xE:
            addrModeXindexed();
            break;
        case 0xF:
            if ((IR & 0xC0) != 0x80)
                addrModeXindexed(); //abs,x for illegal opcodes
            else
                addrModeYindexed(); //abs,y for LAX/SAX illegal opcodes
            break;
        }
        Addr &= 0xFFFF;

        switch ((IR & 0xE0) >> 5)
        { //value-forming to cause gapless case-values and faster jump-table creation from switch-case

        case 0:
            if ((IR & 0x1F) != 0xB)
            { //ORA / SLO(ASO)=ASL+ORA
                if ((IR & 3) == 3)
                {
                    clrNZC();
                    setC(rd(Addr) >= N);
                    wr(Addr, rd(Addr) << 1);
                    Cycles += 2;
                } //for SLO
                else
                    Cycles -= SamePage;
                A |= rd(Addr);
                setNZbyA(); //ORA
            }
            else
            {
                A &= rd(Addr);
                setNZbyA();
                setC(A >= N);
            } //ANC (AND+Carry=bit7)
            break;

        case 1:
            if ((IR & 0x1F) != 0xB)
            { //AND / RLA (ROL+AND)
                if ((IR & 3) == 3)
                { //for RLA
                    T = (rd(Addr) << 1) + (ST & C);
                    clrNZC();
                    setC(T > 255);
                    T &= 0xFF;
                    wr(Addr, T);
                    Cycles += 2;
                }
                else
                    Cycles -= SamePage;
                A &= rd(Addr);
                setNZbyA(); //AND
            }
            else
            {
                A &= rd(Addr);
                setNZbyA();
                setC(A >= N);
            } //ANC (AND+Carry=bit7)
            break;

        case 2:
            if ((IR & 0x1F) != 0xB)
            { //EOR / SRE(LSE)=LSR+EOR
                if ((IR & 3) == 3)
                {
                    clrNZC();
                    setC(rd(Addr) & 1);
                    wr(Addr, rd(Addr) >> 1);
                    Cycles += 2;
                } //for SRE
                else
                    Cycles -= SamePage;
                A ^= rd(Addr);
                setNZbyA(); //EOR
            }
            else
            {
                A &= rd(Addr);
                setC(A & 1);
                A >>= 1;
                A &= 0xFF;
                setNZbyA();
            } //ALR(ASR)=(AND+LSR)
            break;

        case 3:
            if ((IR & 0x1F) != 0xB)
            { //RRA (ROR+ADC) / ADC
                if ((IR & 3) == 3)
                { //for RRA
                    T = (rd(Addr) >> 1) + ((ST & C) << 7);
                    clrNZC();
                    setC(T & 1);
                    wr(Addr, T);
                    Cycles += 2;
                }
                else
                    Cycles -= SamePage;
                if (Banks)
                {
                    if (ST & D)
                    {
                        const unsigned old = A, v = rd(Addr), sum = old + v + (ST & C);
                        unsigned lo = (old & 15) + (v & 15) + (ST & C),
                                 hi = (old & 0xf0) + (v & 0xf0);
                        if (lo > 9)
                            lo += 6;
                        if (lo > 15)
                            hi += 16;
                        ST &= ~(N | V | Z | C);
                        ST |= (!(sum & 255)) * Z | (hi & N) |
                              ((((hi ^ old) & 128) && !((old ^ v) & 128)) * V);
                        if (hi > 0x90)
                            hi += 0x60;
                        ST |= hi > 255;
                        A = (hi | (lo & 15)) & 255;
                    }
                    else
                    {
                        T = A;
                        A += rd(Addr) + (ST & C);
                        setNZCbyAdd();
                        setVbyAdd(rd(Addr));
                    }
                }
                else
                {
                    T = A;
                    A += rd(Addr) + (ST & C);
                    setNZCbyAdd();
                    setVbyAdd(rd(Addr));
                }
            }
            else
            { // ARR (AND+ROR, bit0 not going to C, but C and bit7 get exchanged.)
                A &= rd(Addr);
                T += rd(Addr) + (ST & C);
                clrNVZC();
                setC(T > 255);
                setVbyAdd(rd(Addr)); //V-flag set by intermediate ADC mechanism: (A&mem)+mem
                T = A;
                A = (A >> 1) + ((ST & C) << 7);
                setC(T >= N);
                setNZbyA();
            }
            break;

        case 4:
            if ((IR & 0x1F) == 0xB)
            {
                A = X & rd(Addr);
                setNZbyA();
            } //XAA (TXA+AND), highly unstable on real 6502!
            else if ((IR & 0x1F) == 0x1B)
            {
                SP = A & X;
                wr(Addr, SP & ((Addr >> 8) + 1));
            } //TAS(SHS) (SP=A&X, mem=S&H} - unstable on real 6502
            else
            {
                wr2(Addr, A & (((IR & 3) == 3) ? X : 0xFF));
            } //STA / SAX (at times same as AHX/SHX/SHY) (illegal)
            break;

        case 5:
            if ((IR & 0x1F) != 0x1B)
            {
                A = rd(Addr);
                if ((IR & 3) == 3)
                    X = A;
            } //LDA / LAX (illegal, used by my 1 rasterline player) (LAX #imm is unstable on C64)
            else
            {
                A = X = SP = rd(Addr) & SP;
            } //LAS(LAR)
            setNZbyA();
            Cycles -= SamePage;
            break;

        case 6:
            if ((IR & 0x1F) != 0xB)
            { // CMP / DCP(DEC+CMP)
                if ((IR & 3) == 3)
                {
                    wr(Addr, rd(Addr) - 1);
                    Cycles += 2;
                } //DCP
                else
                    Cycles -= SamePage;
                T = A - rd(Addr);
            }
            else
            {
                X = T = (A & X) - rd(Addr);
            } //SBX(AXS)  //SBX (AXS) (CMP+DEX at the same time)
            setNZCbySub(&T);
            break;

        case 7:
            if ((IR & 3) == 3 && (IR & 0x1F) != 0xB)
            {
                wr(Addr, rd(Addr) + 1);
                Cycles += 2;
            } //ISC(ISB)=INC+SBC / SBC
            else
                Cycles -= SamePage;
            if (Banks)
            {
                if (ST & D)
                {
                    const unsigned old = A, v = rd(Addr), borrow = !(ST & C),
                                   result = old - v - borrow;
                    unsigned lo = (old & 15) - (v & 15) - borrow, hi = (old & 0xf0) - (v & 0xf0);
                    ST &= ~(N | V | Z | C);
                    ST |= (result < 256) | ((((result ^ old) & 128) && ((old ^ v) & 128)) * V) |
                          (result & N) | (!(result & 255)) * Z;
                    if (lo & 16)
                    {
                        lo -= 6;
                        hi -= 16;
                    }
                    if (hi & 256)
                        hi -= 0x60;
                    A = (hi | (lo & 15)) & 255;
                }
                else
                {
                    T = A;
                    A -= rd(Addr) + !(ST & C);
                    setNZCbySub(&A);
                    setVbyAdd(~rd(Addr));
                }
            }
            else
            {
                T = A;
                A -= rd(Addr) + !(ST & C);
                setNZCbySub(&A);
                setVbyAdd(~rd(Addr));
            }
            break;
        }
    }

    else if (IR & 2)
    { //nybble2:  2:illegal/LDX, 6:A/X/INC/DEC, A:Accu-shift/reg.transfer/NOP, E:shift/X/INC/DEC

        switch (IR & 0x1F)
        { //Addressing modes
        case 2:
            addrModeImmediate();
            break;
        case 6:
            addrModeZeropage();
            break;
        case 0xE:
            addrModeAbsolute();
            break;
        case 0x16:
            if ((IR & 0xC0) != 0x80)
                addrModeZeropageXindexed(); //zp,x
            else
                addrModeZeropageYindexed(); //zp,y
            break;
        case 0x1E:
            if ((IR & 0xC0) != 0x80)
                addrModeXindexed(); //abs,x
            else
                addrModeYindexed(); //abs,y
            break;
        }
        Addr &= 0xFFFF;

        switch ((IR & 0xE0) >> 5)
        {

        case 0:
            clrC();
        case 1:
            if ((IR & 0xF) == 0xA)
            {
                A = (A << 1) + (ST & C);
                setNZCbyAdd();
            } //ASL/ROL (Accu)
            else
            {
                T = (rd(Addr) << 1) + (ST & C);
                setC(T > 255);
                setNZbyT();
                wr(Addr, T);
                Cycles += 2;
            } //RMW (Read-Write-Modify)
            break;

        case 2:
            clrC();
        case 3:
            if ((IR & 0xF) == 0xA)
            {
                T = A;
                A = (A >> 1) + ((ST & C) << 7);
                setC(T & 1);
                A &= 0xFF;
                setNZbyA();
            } //LSR/ROR (Accu)
            else
            {
                T = (rd(Addr) >> 1) + ((ST & C) << 7);
                setC(rd(Addr) & 1);
                setNZbyT();
                wr(Addr, T);
                Cycles += 2;
            } //memory (RMW)
            break;

        case 4:
            if (IR & 4)
            {
                wr2(Addr, X);
            } //STX
            else if (IR & 0x10)
                SP = X; //TXS
            else
            {
                A = X;
                setNZbyA();
            } //TXA
            break;

        case 5:
            if ((IR & 0xF) != 0xA)
            {
                X = rd(Addr);
                Cycles -= SamePage;
            } //LDX
            else if (IR & 0x10)
                X = SP; //TSX
            else
                X = A; //TAX
            setNZbyX();
            break;

        case 6:
            if (IR & 4)
            {
                T = rd(Addr) - 1;
                setNZbyT();
                wr(Addr, T);
                Cycles += 2;
            } //DEC
            else
            {
                --X;
                setNZbyX();
            } //DEX
            break;

        case 7:
            if (IR & 4)
            {
                T = rd(Addr) + 1;
                setNZbyT();
                wr(Addr, T);
                Cycles += 2;
            } //INC/NOP
            break;
        }
    }

    else if ((IR & 0xC) == 8)
    { //nybble2:  8:register/statusflag
        if (IR & 0x10)
        {
            if (IR == 0x98)
            {
                A = Y;
                setNZbyA();
            } //TYA
            else
            { //CLC/SEC/CLI/SEI/CLV/CLD/SED
                if (FlagSwitches[IR >> 5] & 0x20)
                    ST |= (FlagSwitches[IR >> 5] & 0xDF);
                else
                    ST &= ~(FlagSwitches[IR >> 5] & 0xDF);
            }
        }
        else
        {
            switch ((IR & 0xF0) >> 5)
            {
            case 0:
                push(ST | 0x30);
                Cycles = 3;
                break; //PHP
            case 1:
                ST = pop();
                Cycles = 4;
                break; //PLP
            case 2:
                push(A);
                Cycles = 3;
                break; //PHA
            case 3:
                A = pop();
                setNZbyA();
                Cycles = 4;
                break; //PLA
            case 4:
                --Y;
                setNZbyY();
                break; //DEY
            case 5:
                Y = A;
                setNZbyY();
                break; //TAY
            case 6:
                ++Y;
                setNZbyY();
                break; //INY
            case 7:
                ++X;
                setNZbyX();
                break; //INX
            }
        }
    }

    else
    { //nybble2:  0: control/branch/Y/compare  4: Y/compare  C:Y/compare/JMP

        if ((IR & 0x1F) == 0x10)
        { //BPL/BMI/BVC/BVS/BCC/BCS/BNE/BEQ  relative branch
            ++PC;
            T = rd(PC);
            if (T & 0x80)
                T -= 0x100;
            if (IR & 0x20)
            {
                if (ST & BranchFlags[IR >> 6])
                {
                    Cycles = 3 + (((PC + 1) & 0xff00) != ((PC + T + 1) & 0xff00));
                    PC += T;
                }
            }
            else
            {
                if (!(ST & BranchFlags[IR >> 6]))
                {
                    Cycles = 3 + (((PC + 1) & 0xff00) != ((PC + T + 1) & 0xff00));
                    PC += T;
                } //plus 1 cycle if page is crossed?
            }
        }

        else
        { //nybble2:  0:Y/control/Y/compare  4:Y/compare  C:Y/compare/JMP
            switch (IR & 0x1F)
            { //Addressing modes
            case 0:
                addrModeImmediate();
                break; //imm. (or abs.low for JSR/BRK)
            case 4:
                addrModeZeropage();
                break;
            case 0xC:
                addrModeAbsolute();
                break;
            case 0x14:
                addrModeZeropageXindexed();
                break; //zp,x
            case 0x1C:
                addrModeXindexed();
                break; //abs,x
            }
            Addr &= 0xFFFF;

            switch ((IR & 0xE0) >> 5)
            {

            case 0:
                if (!(IR & 4))
                { //BRK / NOP-absolute/abs,x/zp/zp,x
                    push((PC + 2 - 1) >> 8);
                    push((PC + 2 - 1) & 0xFF);
                    push(ST | B);
                    ST |= I; //BRK
                    PC = rd(0xFFFE) + (rd(0xFFFF) << 8) - 1;
                    Cycles = 7;
                }
                else if (IR == 0x1C)
                    Cycles -= SamePage; //NOP abs,x
                break;

            case 1:
                if (IR & 0xF)
                { //BIT / NOP-abs,x/zp,x
                    if (!(IR & 0x10))
                    {
                        ST &= 0x3D;
                        ST |= (rd(Addr) & 0xC0) | ((!(A & rd(Addr))) << 1);
                    } //BIT
                    else if (IR == 0x3C)
                        Cycles -= SamePage; //NOP abs,x
                }
                else
                { //JSR
                    push((PC + 2 - 1) >> 8);
                    push((PC + 2 - 1) & 0xFF);
                    PC = rd(Addr) + rd(Addr + 1) * 256 - 1;
                    Cycles = 6;
                }
                break;

            case 2:
                if (IR & 0xF)
                { //JMP / NOP-abs,x/zp/zp,x
                    if (IR == 0x4C)
                    { //JMP
                        PC = Addr - 1;
                        Cycles = 3;
                        rd(Addr +
                           1); //a read from jump-address highbyte is used in some tunes with jmp DD0C (e.g. Wonderland_XIII_tune_1.sid)
                        //if (Addr==PrevPC) {storeReg(); C64->Returned=1; return 0xFF;} //turn self-jump mainloop (after init) into idle time
                    }
                    else if (IR == 0x5C)
                        Cycles -= SamePage; //NOP abs,x
                }
                else
                { //RTI
                    ST = pop();
                    T = pop();
                    PC = (pop() << 8) + T - 1;
                    Cycles = 6;
                }
                break;

            case 3:
                if (IR & 0xF)
                { //JMP() (indirect) / NOP-abs,x/zp/zp,x
                    if (IR == 0x6C)
                    { //JMP() (indirect)
                        PC = rd((Addr & 0xFF00) +
                                ((Addr + 1) & 0xFF)); //(with highbyte-wraparound bug)
                        PC = (PC << 8) + rd(Addr) - 1;
                        Cycles = 5;
                    }
                    else if (IR == 0x7C)
                        Cycles -= SamePage; //NOP abs,x
                }
                else
                { //RTS
                    if (SP >= 0xFF)
                        return 0; //Init returns, provide idle-time between IRQs
                    T = pop();
                    PC = (pop() << 8) + T;
                    Cycles = 6;
                }
                break;

            case 4:
                if (IR & 4)
                {
                    wr2(Addr, Y);
                } //STY / NOP #imm
                break;

            case 5:
                Y = rd(Addr);
                setNZbyY();
                Cycles -= SamePage; //LDY
                break;

            case 6:
                if (!(IR & 0x10))
                { //CPY / NOP abs,x/zp,x
                    T = Y - rd(Addr);
                    setNZCbySub(&T); //CPY
                }
                else if (IR == 0xDC)
                    Cycles -= SamePage; //NOP abs,x
                break;

            case 7:
                if (!(IR & 0x10))
                { //CPX / NOP abs,x/zp,x
                    T = X - rd(Addr);
                    setNZCbySub(&T); //CPX
                }
                else if (IR == 0xFC)
                    Cycles -= SamePage; //NOP abs,x
                break;
            }
        }
    }

    if (call->error)
        return 0;
    for (unsigned i = 0; i < pendingCount; ++i)
    {
        CallWrite w = pending[i];
        if (IR == 0x20)
            w.cycle = call->cycles + 3 + i;
        else
            w.cycle = call->cycles + (uint8_t)Cycles - pendingCount + i;
        call->writes[call->count++] = w;
    }
    call->cycles += (uint8_t)Cycles;
    ++PC; //PC&=0xFFFF;

    goto nextInstruction;
}

int prepare_player_call(PlayerCall* call)
{
    return execute_player_call<false>(call);
}
int prepare_player_call_basic_ram(PlayerCall* call)
{
    return execute_player_call<true>(call);
}

int prepare_player_call_colours(PlayerCall* call, int basicRam)
{
    return execute_player_call<false, true>(call, basicRam != 0);
}

// Keep the bank-switching interpreter separate from ordinary playback code.
#if defined(__GNUC__) && defined(_WIN32)
template __attribute__((section(".sidseek"), noinline)) int
execute_player_call<false, false, true>(PlayerCall*, bool, PlayerCallBank*);
[[gnu::section(".sidseek"), gnu::noinline]]
#endif
int prepare_player_call_banked(PlayerCall* call, PlayerCallBank* bank)
{
    const auto port = bank->data, initialFlags = call->p;
    if ((bank->dir & 0xc7) != 7 || port < 0x34 || port > 0x37 ||
        ((bank->data & bank->dir) | (bank->pins & ~bank->dir)) != port)
        return 0;
    const int result = execute_player_call<false, false, true>(call, false, bank);
    if (bank->data != port || ((call->p ^ initialFlags) & 12))
    {
        call->error = 2;
        return 0;
    }
    return result;
}
