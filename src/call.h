#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    enum
    {
        CALL_ACTIONS = 2048,
        CALL_CYCLES = 65536
    };
    /**
     * A recorded write to physical RAM, SID, VIC colour registers or the CPU port.
     * The original call APIs use write kind zero.
     */
    typedef struct
    {
        unsigned cycle;
        uint16_t address;
        uint8_t value, kind;
    } CallWrite;
    typedef struct
    {
        uint8_t memory[65536];
        uint16_t pc;
        uint8_t a, x, y, p, sp;
        unsigned cycles, count, error;
        CallWrite writes[CALL_ACTIONS];
    } PlayerCall;
    typedef struct
    {
        uint8_t dir, data, pins, sid[25], colours[15];
        unsigned seed;
    } PlayerCallBank;
    /**
     * Prepare a bounded player call with banked physical RAM.
     * The caller supplies port direction, data/readback, MMU bus seed and
     * independent SID and VIC colour registers. Only stable input bits and
     * paired writes of 34-37 to port 0001 are supported. The caller installs
     * the private state after its driver and timing checks.
     */
    int prepare_player_call_banked(PlayerCall*, PlayerCallBank*);
    int prepare_player_call(PlayerCall*);
    /**
     * Prepare a bounded player call with RAM mapped beneath BASIC ROM.
     * The caller must verify that BASIC ROM is hidden and I/O remains mapped.
     */
    int prepare_player_call_basic_ram(PlayerCall*);
    /**
     * Prepare a bounded player call with VIC colour writes.
     * The caller verifies I/O mapping and initialises D020-D02E with colour values.
     */
    int prepare_player_call_colours(PlayerCall*, int basicRam);
#ifdef __cplusplus
}
#endif
