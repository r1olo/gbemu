#ifndef __CPU_COMMON_H
#define __CPU_COMMON_H

#include "soc/soc.h"

#include <stdbool.h>
#include <stdint.h>

/* ALU flag macros */
#define FLAG(n) \
    ((cpu->af.lo & BIT(n)) >> n)
#define SET_FLAG(n) \
    cpu->af.lo = BIT(n) | cpu->af.lo
#define UNSET_FLAG(n) \
    cpu->af.lo = ~(BIT(n) | ~cpu->af.lo)
#define FLAG_ZERO()     FLAG(7)
#define SET_ZERO()      SET_FLAG(7)
#define UNSET_ZERO()    UNSET_FLAG(7)
#define FLAG_SUB()      FLAG(6)
#define SET_SUB()       SET_FLAG(6)
#define UNSET_SUB()     UNSET_FLAG(6)
#define FLAG_HALF()     FLAG(5)
#define SET_HALF()      SET_FLAG(5)
#define UNSET_HALF()    UNSET_FLAG(5)
#define FLAG_CARRY()    FLAG(4)
#define SET_CARRY()     SET_FLAG(4)
#define UNSET_CARRY()   UNSET_FLAG(4)

/* ALU operation macros */
#define INC8(x) \
    if ((((x) & 0xf) + 1) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    (x)++; \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define DEC8(x) \
    if ((((x) & 0xf) - 1) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    (x)--; \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define ADD8(x, y) \
    if ((((x) & 0xf) + ((y) & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if ((((x) & 0xff) + ((y) & 0xff)) & 0x100) SET_CARRY(); \
    else UNSET_CARRY(); \
    (x) += (y); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define ADC8(x, y, tmp) \
    (tmp) = FLAG_CARRY(); \
    if ((((x) & 0xf) + ((y) & 0xf) + (tmp)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if ((((x) & 0xff) + ((y) & 0xff) + (tmp)) & 0x100) SET_CARRY(); \
    else UNSET_CARRY(); \
    (x) += (y) + (tmp); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define SUB8(x, y) \
    if ((((x) & 0xf) - ((y) & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if ((((x) & 0xff) - ((y) & 0xff)) & 0x100) SET_CARRY(); \
    else UNSET_CARRY(); \
    (x) -= y; \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define SBC8(x, y, tmp) \
    (tmp) = FLAG_CARRY(); \
    if ((((x) & 0xf) - ((y) & 0xf) - (tmp)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if ((((x) & 0xff) - ((y) & 0xff) - (tmp)) & 0x100) SET_CARRY(); \
    else UNSET_CARRY(); \
    (x) -= (y) + (tmp); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define AND8(x, y) \
    (x) &= (y); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    SET_HALF(); \
    UNSET_CARRY()

#define XOR8(x, y) \
    (x) ^= (y); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define OR8(x, y) \
    (x) |= (y); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define CP8(x, y) \
    if ((x) == (y)) SET_ZERO(); \
    else UNSET_ZERO(); \
    if ((((x) & 0xff) - ((y) & 0xff)) & 0x100) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((((x) & 0xf) - ((y) & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    SET_SUB()

#define RLC8(x, tmp) \
    (tmp) = (x) >> 7; \
    (x) = ((x) << 1) | (tmp); \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RRC8(x, tmp) \
    (tmp) = (x) & 1; \
    (x) = ((x) >> 1) | ((tmp) << 7); \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RL8(x, tmp) \
    (tmp) = (x) >> 7; \
    (x) = ((x) << 1) | FLAG_CARRY(); \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RR8(x, tmp) \
    (tmp) = (x) & 1; \
    (x) = ((x) >> 1) | (FLAG_CARRY() << 7); \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SLA8(x, tmp) \
    (tmp) = (x) >> 7; \
    (x) = (x) << 1; \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SRA8(x, tmp) \
    (tmp) = (x) & 1; \
    (x) = ((x) >> 1) | ((x) & 0x80); \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SWAP8(x, tmp) \
    (tmp) = (x) & 0xf; \
    (x) = ((x) >> 4) | ((tmp) << 4); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define SRL8(x, tmp) \
    (tmp) = (x) & 1; \
    (x) = (x) >> 1; \
    if ((tmp)) SET_CARRY(); \
    else UNSET_CARRY(); \
    if ((x) == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define BIT8(b, x) \
    if (!((x) & (1 << b))) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    SET_HALF()

#define RES8(b, x) \
    (x) &= ~(1 << b)

#define SET8(b, x) \
    (x) |= (1 << b)

/* translate address and read/write from memory (cpu.c) */
bool _cpu_read_byte(cpu_t *cpu, uint8_t *dst, uint16_t addr);
void _cpu_write_byte(cpu_t *cpu, uint16_t addr, uint8_t val);

/* read immediate (cpu.c) */
bool _cpu_read_imm8(cpu_t *cpu, uint8_t *dst);

/* the IDU performed a write. this is used for the OAM bug */
static inline void
_idu_write(cpu_t *cpu, uint16_t val)
{
    /* TODO: this is needed for proper OAM bug implementation. */
}

/* check which interrupts are pending */
static inline uint8_t
_pending_interrupts(cpu_t *cpu)
{
    /* this will give us the currently triggered interrupts. we need to select
     * the lower 5 bits rigorously otherwise we might get phantom interrupts if
     * the upper bits are somehow set */
    return (cpu->ie & cpu->iflag) & 0x1F;
}

/* return whether we are interrupted (pending + IME) */
static inline bool
_is_interrupted(cpu_t *cpu)
{
    /* IME disabled -> no interrupts */
    if (!cpu->ime)
        return false;

    /* depends on whether we have pending interrupts */
    return _pending_interrupts(cpu);
}


#endif /* __CPU_COMMON_H */
