#include "gbemu.h"

static byte
mmu_readb(cpu_t *cpu, ushort addr)
{
    byte ret = 0xff;
    BOOL only_hram = dma_running(cpu->bus->dma);

    /* 0x0000 - 0x7FFF (cart) */
    if (only_hram && addr < 0x8000)
        ret = cpu->bus->cart->read(cpu->bus->cart, addr);

    /* 0x8000 - 0x9FFF (ppu) */
    else if (only_hram && addr >= 0x8000 && addr < 0xA000)
        ; /* ppu readb */

    /* 0xA000 - 0xBFFF (cart) */
    else if (only_hram && addr >= 0xA000 && addr < 0xC000)
        ret = cpu->bus->cart->read(cpu->bus->cart, addr);

    /* 0xC000 - 0xFDFF (mem) */
    else if (only_hram && addr >= 0xC000 && addr < 0xFE00)
        ret = mem_readb(cpu->bus->mem, addr);

    /* 0xFE00 - 0xFE9F (ppu) */
    else if (only_hram && addr >= 0xFE00 && addr < 0xFEA0)
        ; /* ppu readb */

    /* 0xFF00 - 0xFF7F (i/o) */
    else if (only_hram && addr >= 0xFF00 && addr < 0xFF80)
        ; /* TODO: static iomem functions */

    /* 0xFF80 - 0xFFFE (mem) */
    else if (addr >= 0xFF80 && addr < 0xFFFF)
        ret = mem_readb(cpu->bus->mem, addr);

    /* 0xFFFF (cpu ime) */
    else
        ; /* TODO: cpu intr enable */

    return ret;
}

static void
mmu_writeb(cpu_t *cpu, ushort addr, byte val)
{
    BOOL only_hram = dma_running(cpu->bus->dma);

    /* 0x0000 - 0x7FFF (cart) */
    if (only_hram && addr < 0x8000)
        cpu->bus->cart->write(cpu->bus->cart, addr, val);

    /* 0x8000 - 0x9FFF (ppu) */
    else if (only_hram && addr >= 0x8000 && addr < 0xA000)
        ; /* ppu readb */

    /* 0xA000 - 0xBFFF (cart) */
    else if (only_hram && addr >= 0xA000 && addr < 0xC000)
        cpu->bus->cart->write(cpu->bus->cart, addr, val);

    /* 0xC000 - 0xFDFF (mem) */
    else if (only_hram && addr >= 0xC000 && addr < 0xFE00)
        mem_writeb(cpu->bus->mem, addr, val);

    /* 0xFE00 - 0xFE9F (ppu) */
    else if (only_hram && addr >= 0xFE00 && addr < 0xFEA0)
        ; /* ppu readb */

    /* 0xFF00 - 0xFF7F (i/o) */
    else if (only_hram && addr >= 0xFF00 && addr < 0xFF80)
        ; /* TODO: static iomem functions */

    /* 0xFF80 - 0xFFFE (mem) */
    else if (addr >= 0xFF80 && addr < 0xFFFF)
        mem_writeb(cpu->bus->mem, addr, val);

    /* 0xFFFF (cpu ime) */
    else
        ; /* TODO: cpu intr enable */
}

static ushort
mmu_reads(cpu_t *cpu, ushort addr)
{
    reg_t r;
    r.lo = mmu_readb(cpu, addr++);
    r.hi = mmu_readb(cpu, addr);
    return r.val;
}

static void
mmu_writes(cpu_t *cpu, ushort addr, ushort val)
{
    reg_t r;
    r.val = val;
    mmu_writeb(cpu, addr++, r.lo);
    mmu_writeb(cpu, addr, r.hi);
}

/* flag macros */
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

/* instruction macros */
#define INC8(x) \
    if (((x & 0xf) + 1) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    x++; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define DEC8(x) \
    if (((x & 0xf) - 1) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    x--; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define ADDREG(r1, r2) \
    if (((r1.val & 0xfff) + (r2.val & 0xfff)) & 0x1000) SET_HALF(); \
    else UNSET_HALF(); \
    if (r1.val + r2.val < r1.val) SET_CARRY(); \
    else UNSET_CARRY(); \
    r1.val += r2.val; \
    UNSET_SUB()

#define ADD8(x, y) \
    if (((x & 0xf) + (y & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if (x + y < x) SET_CARRY(); \
    else UNSET_CARRY(); \
    x += y; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define ADC8(x, y, tmp) \
    tmp = FLAG_CARRY(); \
    if (((x & 0xf) + (y & 0xf) + tmp) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if (x + y + tmp < x) SET_CARRY(); \
    else UNSET_CARRY(); \
    x += y + tmp; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB()

#define SUB8(x, y) \
    if (((x & 0xf) - (y & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if (x - y > x) SET_CARRY(); \
    else UNSET_CARRY(); \
    x -= y; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define SBC8(x, y, tmp) \
    tmp = FLAG_CARRY(); \
    if (((x & 0xf) - (y & 0xf) - tmp) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    if (x - y - tmp > x) SET_CARRY(); \
    else UNSET_CARRY(); \
    x -= y + tmp; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    SET_SUB()

#define AND8(x, y) \
    x &= y; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    SET_HALF(); \
    UNSET_CARRY()

#define XOR8(x, y) \
    x ^= y; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define OR8(x, y) \
    x |= y; \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define CP8(x, y) \
    if (x == y) SET_ZERO(); \
    else UNSET_ZERO(); \
    if (x < y) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (((x & 0xf) - (y & 0xf)) & 0x10) SET_HALF(); \
    else UNSET_HALF(); \
    SET_SUB()

#define RLC8(x, tmp) \
    tmp = x >> 7; \
    x = (x << 1) | tmp; \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RRC8(x, tmp) \
    tmp = x & 1; \
    x = (x >> 1) | (tmp << 7); \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RL8(x, tmp) \
    tmp = x >> 7; \
    x = (x << 1) | FLAG_CARRY(); \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define RR8(x, tmp) \
    tmp = x & 1; \
    x = (x >> 1) | (FLAG_CARRY() << 7); \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SLA8(x, tmp) \
    tmp = x >> 7; \
    x = x << 1; \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SRA8(x, tmp) \
    tmp = x & 1; \
    x = (x >> 1) | (x & 0x80); \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define SWAP8(x, tmp) \
    tmp = x & 0xf; \
    x = (x >> 4) | (tmp << 4); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF(); \
    UNSET_CARRY()

#define SRL8(x, tmp) \
    tmp = x & 1; \
    x = x >> 1; \
    if (tmp) SET_CARRY(); \
    else UNSET_CARRY(); \
    if (x == 0) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    UNSET_HALF()

#define BIT8(b, x) \
    if (!(x & (1 << b))) SET_ZERO(); \
    else UNSET_ZERO(); \
    UNSET_SUB(); \
    SET_HALF()

#define RES8(b, x) \
    x = x & ~(1 << b)

#define SET8(b, x) \
    x = x | (1 << b)

#define PUSH(x) \
    cpu->sp.val -= 2; mmu_writes(cpu, cpu->sp.val, x)
#define POP() \
    mmu_reads(cpu, cpu->sp.val); cpu->sp.val += 2
#define SINGLE_OP() \
    mmu_readb(cpu, cpu->pc.val++)
#define DOUBLE_OP() \
    mmu_reads(cpu, cpu->pc.val); cpu->pc.val += 2
#define CALL() \
    PUSH(cpu->pc.val + 2); \
    cpu->pc.val = mmu_reads(cpu, cpu->pc.val)

/* interrupts */
#define CHECKINT(n) \
    ((ieflag & BIT(n)) && (ifflag & BIT(n)))
#define CLEARIF(n) \
    (~(BIT(n) | ~ifflag))

static int
enter_isr(cpu_t *cpu)
{
    byte ieflag = mmu_readb(cpu, 0xFFFF),
         ifflag = mmu_readb(cpu, 0xFF0F);

    if (!cpu->ime || !(ieflag & ifflag))
        return 0;

    cpu->ime = 0;

    if (CHECKINT(0)) { /* VBlank: bit 0 */
        mmu_writeb(cpu, 0xFF0F, CLEARIF(0));
        PUSH(cpu->pc.val);
        cpu->pc.val = 0x40;
    } else if (CHECKINT(1)) { /* LCD STAT: bit 1 */
        mmu_writeb(cpu, 0xFF0F, CLEARIF(1));
        PUSH(cpu->pc.val);
        cpu->pc.val = 0x48;
    } else if (CHECKINT(2)) { /* Timer: bit 2 */
        mmu_writeb(cpu, 0xFF0F, CLEARIF(2));
        PUSH(cpu->pc.val);
        cpu->pc.val = 0x50;
    } else if (CHECKINT(3)) { /* Serial: bit 3 */
        mmu_writeb(cpu, 0xFF0F, CLEARIF(3));
        PUSH(cpu->pc.val);
        cpu->pc.val = 0x58;
    } else { /* Joypad: bit 4 */
        mmu_writeb(cpu, 0xFF0F, CLEARIF(4));
        PUSH(cpu->pc.val);
        cpu->pc.val = 0x60;
    }

    return 1;
}

static int
prefix_instr(cpu_t *cpu)
{
    int c;
    byte op, tmpb, tmpb2;

    op = SINGLE_OP();
    switch(op) {
        case 0x00: /* RLC B */
            RLC8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x01: /* RLC C */
            RLC8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x02: /* RLC D */
            RLC8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x03: /* RLC E */
            RLC8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x04: /* RLC H */
            RLC8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x05: /* RLC L */
            RLC8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x06: /* RLC (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RLC8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x07: /* RLC A */
            RLC8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x08: /* RRC B */
            RRC8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x09: /* RRC C */
            RRC8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x0A: /* RRC D */
            RRC8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x0B: /* RRC E */
            RRC8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x0C: /* RRC H */
            RRC8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x0D: /* RRC L */
            RRC8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x0E: /* RRC (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RRC8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x0F: /* RRC A */
            RRC8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x10: /* RL B */
            RL8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x11: /* RL C */
            RL8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x12: /* RL D */
            RL8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x13: /* RL E */
            RL8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x14: /* RL H */
            RL8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x15: /* RL L */
            RL8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x16: /* RL (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RL8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x17: /* RL A */
            RL8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x18: /* RR B */
            RR8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x19: /* RR C */
            RR8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x1A: /* RR D */
            RR8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x1B: /* RR E */
            RR8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x1C: /* RR H */
            RR8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x1D: /* RR L */
            RR8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x1E: /* RR (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RR8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x1F: /* RR A */
            RR8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x20: /* SLA B */
            SLA8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x21: /* SLA C */
            SLA8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x22: /* SLA D */
            SLA8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x23: /* SLA E */
            SLA8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x24: /* SLA H */
            SLA8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x25: /* SLA L */
            SLA8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x26: /* SLA (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SLA8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x27: /* SLA A */
            SLA8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x28: /* SRA B */
            SRA8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x29: /* SRA C */
            SRA8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x2A: /* SRA D */
            SRA8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x2B: /* SRA E */
            SRA8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x2C: /* SRA H */
            SRA8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x2D: /* SRA L */
            SRA8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x2E: /* SRA (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SRA8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x2F: /* SRA A */
            SRA8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x30: /* SWAP B */
            SWAP8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x31: /* SWAP C */
            SWAP8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x32: /* SWAP D */
            SWAP8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x33: /* SWAP E */
            SWAP8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x34: /* SWAP H */
            SWAP8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x35: /* SWAP L */
            SWAP8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x36: /* SWAP (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SWAP8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x37: /* SWAP A */
            SWAP8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x38: /* SRL B */
            SRL8(cpu->bc.hi, tmpb);
            c = 8;
            break;
        case 0x39: /* SRL C */
            SRL8(cpu->bc.lo, tmpb);
            c = 8;
            break;
        case 0x3A: /* SRL D */
            SRL8(cpu->de.hi, tmpb);
            c = 8;
            break;
        case 0x3B: /* SRL E */
            SRL8(cpu->de.lo, tmpb);
            c = 8;
            break;
        case 0x3C: /* SRL H */
            SRL8(cpu->hl.hi, tmpb);
            c = 8;
            break;
        case 0x3D: /* SRL L */
            SRL8(cpu->hl.lo, tmpb);
            c = 8;
            break;
        case 0x3E: /* SRL (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SRL8(tmpb, tmpb2);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x3F: /* SRL A */
            SRL8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x40: /* BIT 0,B */
            BIT8(0, cpu->bc.hi);
            c = 8;
            break;
        case 0x41: /* BIT 0,C */
            BIT8(0, cpu->bc.lo);
            c = 8;
            break;
        case 0x42: /* BIT 0,D */
            BIT8(0, cpu->de.hi);
            c = 8;
            break;
        case 0x43: /* BIT 0,E */
            BIT8(0, cpu->de.lo);
            c = 8;
            break;
        case 0x44: /* BIT 0,H */
            BIT8(0, cpu->hl.hi);
            c = 8;
            break;
        case 0x45: /* BIT 0,L */
            BIT8(0, cpu->hl.lo);
            c = 8;
            break;
        case 0x46: /* BIT 0,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(0, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x47: /* BIT 0,A */
            BIT8(0, cpu->af.hi);
            c = 8;
            break;
        case 0x48: /* BIT 1,B */
            BIT8(1, cpu->bc.hi);
            c = 8;
            break;
        case 0x49: /* BIT 1,C */
            BIT8(1, cpu->bc.lo);
            c = 8;
            break;
        case 0x4A: /* BIT 1,D */
            BIT8(1, cpu->de.hi);
            c = 8;
            break;
        case 0x4B: /* BIT 1,E */
            BIT8(1, cpu->de.lo);
            c = 8;
            break;
        case 0x4C: /* BIT 1,H */
            BIT8(1, cpu->hl.hi);
            c = 8;
            break;
        case 0x4D: /* BIT 1,L */
            BIT8(1, cpu->hl.lo);
            c = 8;
            break;
        case 0x4E: /* BIT 1,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(1, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x4F: /* BIT 1,A */
            BIT8(1, cpu->af.hi);
            c = 8;
            break;
        case 0x50: /* BIT 2,B */
            BIT8(2, cpu->bc.hi);
            c = 8;
            break;
        case 0x51: /* BIT 2,C */
            BIT8(2, cpu->bc.lo);
            c = 8;
            break;
        case 0x52: /* BIT 2,D */
            BIT8(2, cpu->de.hi);
            c = 8;
            break;
        case 0x53: /* BIT 2,E */
            BIT8(2, cpu->de.lo);
            c = 8;
            break;
        case 0x54: /* BIT 2,H */
            BIT8(2, cpu->hl.hi);
            c = 8;
            break;
        case 0x55: /* BIT 2,L */
            BIT8(2, cpu->hl.lo);
            c = 8;
            break;
        case 0x56: /* BIT 2,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(2, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x57: /* BIT 2,A */
            BIT8(2, cpu->af.hi);
            c = 8;
            break;
        case 0x58: /* BIT 3,B */
            BIT8(3, cpu->bc.hi);
            c = 8;
            break;
        case 0x59: /* BIT 3,C */
            BIT8(3, cpu->bc.lo);
            c = 8;
            break;
        case 0x5A: /* BIT 3,D */
            BIT8(3, cpu->de.hi);
            c = 8;
            break;
        case 0x5B: /* BIT 3,E */
            BIT8(3, cpu->de.lo);
            c = 8;
            break;
        case 0x5C: /* BIT 3,H */
            BIT8(3, cpu->hl.hi);
            c = 8;
            break;
        case 0x5D: /* BIT 3,L */
            BIT8(3, cpu->hl.lo);
            c = 8;
            break;
        case 0x5E: /* BIT 3,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(3, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x5F: /* BIT 3,A */
            BIT8(3, cpu->af.hi);
            c = 8;
            break;
        case 0x60: /* BIT 4,B */
            BIT8(4, cpu->bc.hi);
            c = 8;
            break;
        case 0x61: /* BIT 4,C */
            BIT8(4, cpu->bc.lo);
            c = 8;
            break;
        case 0x62: /* BIT 4,D */
            BIT8(4, cpu->de.hi);
            c = 8;
            break;
        case 0x63: /* BIT 4,E */
            BIT8(4, cpu->de.lo);
            c = 8;
            break;
        case 0x64: /* BIT 4,H */
            BIT8(4, cpu->hl.hi);
            c = 8;
            break;
        case 0x65: /* BIT 4,L */
            BIT8(4, cpu->hl.lo);
            c = 8;
            break;
        case 0x66: /* BIT 4,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(4, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x67: /* BIT 4,A */
            BIT8(4, cpu->af.hi);
            c = 8;
            break;
        case 0x68: /* BIT 5,B */
            BIT8(5, cpu->bc.hi);
            c = 8;
            break;
        case 0x69: /* BIT 5,C */
            BIT8(5, cpu->bc.lo);
            c = 8;
            break;
        case 0x6A: /* BIT 5,D */
            BIT8(5, cpu->de.hi);
            c = 8;
            break;
        case 0x6B: /* BIT 5,E */
            BIT8(5, cpu->de.lo);
            c = 8;
            break;
        case 0x6C: /* BIT 5,H */
            BIT8(5, cpu->hl.hi);
            c = 8;
            break;
        case 0x6D: /* BIT 5,L */
            BIT8(5, cpu->hl.lo);
            c = 8;
            break;
        case 0x6E: /* BIT 5,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(5, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x6F: /* BIT 5,A */
            BIT8(5, cpu->af.hi);
            c = 8;
            break;
        case 0x70: /* BIT 6,B */
            BIT8(6, cpu->bc.hi);
            c = 8;
            break;
        case 0x71: /* BIT 6,C */
            BIT8(6, cpu->bc.lo);
            c = 8;
            break;
        case 0x72: /* BIT 6,D */
            BIT8(6, cpu->de.hi);
            c = 8;
            break;
        case 0x73: /* BIT 6,E */
            BIT8(6, cpu->de.lo);
            c = 8;
            break;
        case 0x74: /* BIT 6,H */
            BIT8(6, cpu->hl.hi);
            c = 8;
            break;
        case 0x75: /* BIT 6,L */
            BIT8(6, cpu->hl.lo);
            c = 8;
            break;
        case 0x76: /* BIT 6,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(6, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x77: /* BIT 6,A */
            BIT8(6, cpu->af.hi);
            c = 8;
            break;
        case 0x78: /* BIT 7,B */
            BIT8(7, cpu->bc.hi);
            c = 8;
            break;
        case 0x79: /* BIT 7,C */
            BIT8(7, cpu->bc.lo);
            c = 8;
            break;
        case 0x7A: /* BIT 7,D */
            BIT8(7, cpu->de.hi);
            c = 8;
            break;
        case 0x7B: /* BIT 7,E */
            BIT8(7, cpu->de.lo);
            c = 8;
            break;
        case 0x7C: /* BIT 7,H */
            BIT8(7, cpu->hl.hi);
            c = 8;
            break;
        case 0x7D: /* BIT 7,L */
            BIT8(7, cpu->hl.lo);
            c = 8;
            break;
        case 0x7E: /* BIT 7,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            BIT8(7, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x7F: /* BIT 7,A */
            BIT8(7, cpu->af.hi);
            c = 8;
            break;
        case 0x80: /* RES 0,B */
            RES8(0, cpu->bc.hi);
            c = 8;
            break;
        case 0x81: /* RES 0,C */
            RES8(0, cpu->bc.lo);
            c = 8;
            break;
        case 0x82: /* RES 0,D */
            RES8(0, cpu->de.hi);
            c = 8;
            break;
        case 0x83: /* RES 0,E */
            RES8(0, cpu->de.lo);
            c = 8;
            break;
        case 0x84: /* RES 0,H */
            RES8(0, cpu->hl.hi);
            c = 8;
            break;
        case 0x85: /* RES 0,L */
            RES8(0, cpu->hl.lo);
            c = 8;
            break;
        case 0x86: /* RES 0,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(0, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x87: /* RES 0,A */
            RES8(0, cpu->af.hi);
            c = 8;
            break;
        case 0x88: /* RES 1,B */
            RES8(1, cpu->bc.hi);
            c = 8;
            break;
        case 0x89: /* RES 1,C */
            RES8(1, cpu->bc.lo);
            c = 8;
            break;
        case 0x8A: /* RES 1,D */
            RES8(1, cpu->de.hi);
            c = 8;
            break;
        case 0x8B: /* RES 1,E */
            RES8(1, cpu->de.lo);
            c = 8;
            break;
        case 0x8C: /* RES 1,H */
            RES8(1, cpu->hl.hi);
            c = 8;
            break;
        case 0x8D: /* RES 1,L */
            RES8(1, cpu->hl.lo);
            c = 8;
            break;
        case 0x8E: /* RES 1,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(1, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x8F: /* RES 1,A */
            RES8(1, cpu->af.hi);
            c = 8;
            break;
        case 0x90: /* RES 2,B */
            RES8(2, cpu->bc.hi);
            c = 8;
            break;
        case 0x91: /* RES 2,C */
            RES8(2, cpu->bc.lo);
            c = 8;
            break;
        case 0x92: /* RES 2,D */
            RES8(2, cpu->de.hi);
            c = 8;
            break;
        case 0x93: /* RES 2,E */
            RES8(2, cpu->de.lo);
            c = 8;
            break;
        case 0x94: /* RES 2,H */
            RES8(2, cpu->hl.hi);
            c = 8;
            break;
        case 0x95: /* RES 2,L */
            RES8(2, cpu->hl.lo);
            c = 8;
            break;
        case 0x96: /* RES 2,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(2, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x97: /* RES 2,A */
            RES8(2, cpu->af.hi);
            c = 8;
            break;
        case 0x98: /* RES 3,B */
            RES8(3, cpu->bc.hi);
            c = 8;
            break;
        case 0x99: /* RES 3,C */
            RES8(3, cpu->bc.lo);
            c = 8;
            break;
        case 0x9A: /* RES 3,D */
            RES8(3, cpu->de.hi);
            c = 8;
            break;
        case 0x9B: /* RES 3,E */
            RES8(3, cpu->de.lo);
            c = 8;
            break;
        case 0x9C: /* RES 3,H */
            RES8(3, cpu->hl.hi);
            c = 8;
            break;
        case 0x9D: /* RES 3,L */
            RES8(3, cpu->hl.lo);
            c = 8;
            break;
        case 0x9E: /* RES 3,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(3, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0x9F: /* RES 3,A */
            RES8(3, cpu->af.hi);
            c = 8;
            break;
        case 0xA0: /* RES 4,B */
            RES8(4, cpu->bc.hi);
            c = 8;
            break;
        case 0xA1: /* RES 4,C */
            RES8(4, cpu->bc.lo);
            c = 8;
            break;
        case 0xA2: /* RES 4,D */
            RES8(4, cpu->de.hi);
            c = 8;
            break;
        case 0xA3: /* RES 4,E */
            RES8(4, cpu->de.lo);
            c = 8;
            break;
        case 0xA4: /* RES 4,H */
            RES8(4, cpu->hl.hi);
            c = 8;
            break;
        case 0xA5: /* RES 4,L */
            RES8(4, cpu->hl.lo);
            c = 8;
            break;
        case 0xA6: /* RES 4,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(4, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xA7: /* RES 4,A */
            RES8(4, cpu->af.hi);
            c = 8;
            break;
        case 0xA8: /* RES 5,B */
            RES8(5, cpu->bc.hi);
            c = 8;
            break;
        case 0xA9: /* RES 5,C */
            RES8(5, cpu->bc.lo);
            c = 8;
            break;
        case 0xAA: /* RES 5,D */
            RES8(5, cpu->de.hi);
            c = 8;
            break;
        case 0xAB: /* RES 5,E */
            RES8(5, cpu->de.lo);
            c = 8;
            break;
        case 0xAC: /* RES 5,H */
            RES8(5, cpu->hl.hi);
            c = 8;
            break;
        case 0xAD: /* RES 5,L */
            RES8(5, cpu->hl.lo);
            c = 8;
            break;
        case 0xAE: /* RES 5,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(5, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xAF: /* RES 5,A */
            RES8(5, cpu->af.hi);
            c = 8;
            break;
        case 0xB0: /* RES 6,B */
            RES8(6, cpu->bc.hi);
            c = 8;
            break;
        case 0xB1: /* RES 6,C */
            RES8(6, cpu->bc.lo);
            c = 8;
            break;
        case 0xB2: /* RES 6,D */
            RES8(6, cpu->de.hi);
            c = 8;
            break;
        case 0xB3: /* RES 6,E */
            RES8(6, cpu->de.lo);
            c = 8;
            break;
        case 0xB4: /* RES 6,H */
            RES8(6, cpu->hl.hi);
            c = 8;
            break;
        case 0xB5: /* RES 6,L */
            RES8(6, cpu->hl.lo);
            c = 8;
            break;
        case 0xB6: /* RES 6,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(6, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xB7: /* RES 6,A */
            RES8(6, cpu->af.hi);
            c = 8;
            break;
        case 0xB8: /* RES 7,B */
            RES8(7, cpu->bc.hi);
            c = 8;
            break;
        case 0xB9: /* RES 7,C */
            RES8(7, cpu->bc.lo);
            c = 8;
            break;
        case 0xBA: /* RES 7,D */
            RES8(7, cpu->de.hi);
            c = 8;
            break;
        case 0xBB: /* RES 7,E */
            RES8(7, cpu->de.lo);
            c = 8;
            break;
        case 0xBC: /* RES 7,H */
            RES8(7, cpu->hl.hi);
            c = 8;
            break;
        case 0xBD: /* RES 7,L */
            RES8(7, cpu->hl.lo);
            c = 8;
            break;
        case 0xBE: /* RES 7,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            RES8(7, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xBF: /* RES 7,A */
            RES8(7, cpu->af.hi);
            c = 8;
            break;
        case 0xC0: /* SET 0,B */
            SET8(0, cpu->bc.hi);
            c = 8;
            break;
        case 0xC1: /* SET 0,C */
            SET8(0, cpu->bc.lo);
            c = 8;
            break;
        case 0xC2: /* SET 0,D */
            SET8(0, cpu->de.hi);
            c = 8;
            break;
        case 0xC3: /* SET 0,E */
            SET8(0, cpu->de.lo);
            c = 8;
            break;
        case 0xC4: /* SET 0,H */
            SET8(0, cpu->hl.hi);
            c = 8;
            break;
        case 0xC5: /* SET 0,L */
            SET8(0, cpu->hl.lo);
            c = 8;
            break;
        case 0xC6: /* SET 0,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(0, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xC7: /* SET 0,A */
            SET8(0, cpu->af.hi);
            c = 8;
            break;
        case 0xC8: /* SET 1,B */
            SET8(1, cpu->bc.hi);
            c = 8;
            break;
        case 0xC9: /* SET 1,C */
            SET8(1, cpu->bc.lo);
            c = 8;
            break;
        case 0xCA: /* SET 1,D */
            SET8(1, cpu->de.hi);
            c = 8;
            break;
        case 0xCB: /* SET 1,E */
            SET8(1, cpu->de.lo);
            c = 8;
            break;
        case 0xCC: /* SET 1,H */
            SET8(1, cpu->hl.hi);
            c = 8;
            break;
        case 0xCD: /* SET 1,L */
            SET8(1, cpu->hl.lo);
            c = 8;
            break;
        case 0xCE: /* SET 1,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(1, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xCF: /* SET 1,A */
            SET8(1, cpu->af.hi);
            c = 8;
            break;
        case 0xD0: /* SET 2,B */
            SET8(2, cpu->bc.hi);
            c = 8;
            break;
        case 0xD1: /* SET 2,C */
            SET8(2, cpu->bc.lo);
            c = 8;
            break;
        case 0xD2: /* SET 2,D */
            SET8(2, cpu->de.hi);
            c = 8;
            break;
        case 0xD3: /* SET 2,E */
            SET8(2, cpu->de.lo);
            c = 8;
            break;
        case 0xD4: /* SET 2,H */
            SET8(2, cpu->hl.hi);
            c = 8;
            break;
        case 0xD5: /* SET 2,L */
            SET8(2, cpu->hl.lo);
            c = 8;
            break;
        case 0xD6: /* SET 2,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(2, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xD7: /* SET 2,A */
            SET8(2, cpu->af.hi);
            c = 8;
            break;
        case 0xD8: /* SET 3,B */
            SET8(3, cpu->bc.hi);
            c = 8;
            break;
        case 0xD9: /* SET 3,C */
            SET8(3, cpu->bc.lo);
            c = 8;
            break;
        case 0xDA: /* SET 3,D */
            SET8(3, cpu->de.hi);
            c = 8;
            break;
        case 0xDB: /* SET 3,E */
            SET8(3, cpu->de.lo);
            c = 8;
            break;
        case 0xDC: /* SET 3,H */
            SET8(3, cpu->hl.hi);
            c = 8;
            break;
        case 0xDD: /* SET 3,L */
            SET8(3, cpu->hl.lo);
            c = 8;
            break;
        case 0xDE: /* SET 3,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(3, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xDF: /* SET 3,A */
            SET8(3, cpu->af.hi);
            c = 8;
            break;
        case 0xE0: /* SET 4,B */
            SET8(4, cpu->bc.hi);
            c = 8;
            break;
        case 0xE1: /* SET 4,C */
            SET8(4, cpu->bc.lo);
            c = 8;
            break;
        case 0xE2: /* SET 4,D */
            SET8(4, cpu->de.hi);
            c = 8;
            break;
        case 0xE3: /* SET 4,E */
            SET8(4, cpu->de.lo);
            c = 8;
            break;
        case 0xE4: /* SET 4,H */
            SET8(4, cpu->hl.hi);
            c = 8;
            break;
        case 0xE5: /* SET 4,L */
            SET8(4, cpu->hl.lo);
            c = 8;
            break;
        case 0xE6: /* SET 4,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(4, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xE7: /* SET 4,A */
            SET8(4, cpu->af.hi);
            c = 8;
            break;
        case 0xE8: /* SET 5,B */
            SET8(5, cpu->bc.hi);
            c = 8;
            break;
        case 0xE9: /* SET 5,C */
            SET8(5, cpu->bc.lo);
            c = 8;
            break;
        case 0xEA: /* SET 5,D */
            SET8(5, cpu->de.hi);
            c = 8;
            break;
        case 0xEB: /* SET 5,E */
            SET8(5, cpu->de.lo);
            c = 8;
            break;
        case 0xEC: /* SET 5,H */
            SET8(5, cpu->hl.hi);
            c = 8;
            break;
        case 0xED: /* SET 5,L */
            SET8(5, cpu->hl.lo);
            c = 8;
            break;
        case 0xEE: /* SET 5,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(5, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xEF: /* SET 5,A */
            SET8(5, cpu->af.hi);
            c = 8;
            break;
        case 0xF0: /* SET 6,B */
            SET8(6, cpu->bc.hi);
            c = 8;
            break;
        case 0xF1: /* SET 6,C */
            SET8(6, cpu->bc.lo);
            c = 8;
            break;
        case 0xF2: /* SET 6,D */
            SET8(6, cpu->de.hi);
            c = 8;
            break;
        case 0xF3: /* SET 6,E */
            SET8(6, cpu->de.lo);
            c = 8;
            break;
        case 0xF4: /* SET 6,H */
            SET8(6, cpu->hl.hi);
            c = 8;
            break;
        case 0xF5: /* SET 6,L */
            SET8(6, cpu->hl.lo);
            c = 8;
            break;
        case 0xF6: /* SET 6,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(6, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xF7: /* SET 6,A */
            SET8(6, cpu->af.hi);
            c = 8;
            break;
        case 0xF8: /* SET 7,B */
            SET8(7, cpu->bc.hi);
            c = 8;
            break;
        case 0xF9: /* SET 7,C */
            SET8(7, cpu->bc.lo);
            c = 8;
            break;
        case 0xFA: /* SET 7,D */
            SET8(7, cpu->de.hi);
            c = 8;
            break;
        case 0xFB: /* SET 7,E */
            SET8(7, cpu->de.lo);
            c = 8;
            break;
        case 0xFC: /* SET 7,H */
            SET8(7, cpu->hl.hi);
            c = 8;
            break;
        case 0xFD: /* SET 7,L */
            SET8(7, cpu->hl.lo);
            c = 8;
            break;
        case 0xFE: /* SET 7,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SET8(7, tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 16;
            break;
        case 0xFF: /* SET 7,A */
            SET8(7, cpu->af.hi);
            c = 8;
            break;
        default:
            c = 0;
    }

    return c;
}

int
cpu_step(cpu_t *cpu)
{
    int c;
    byte op, tmpb, tmpb2;
    ushort tmps;

    if (cpu->ei) {
        cpu->ime = 1;
        cpu->ei = 0;
    }

    if (enter_isr(cpu)) {
        cpu->halt = 0;
        return 5;
    }

    if (cpu->halt)
        return 1;

    op = SINGLE_OP();
    switch (op) {
        case 0x00: /* NOP */
            c = 4;
            break;
        case 0x01: /* LD BC,nn */
            cpu->bc.val = DOUBLE_OP();
            c = 12;
            break;
        case 0x02: /* LD (BC),A */
            mmu_writeb(cpu, cpu->bc.val, cpu->af.hi);
            c = 8;
            break;
        case 0x03: /* INC BC */
            cpu->bc.val++;
            c = 8;
            break;
        case 0x04: /* INC B */
            INC8(cpu->bc.hi);
            c = 4;
            break;
        case 0x05: /* DEC B */
            DEC8(cpu->bc.hi);
            c = 4;
            break;
        case 0x06: /* LD B,n */
            cpu->bc.hi = SINGLE_OP();
            c = 8;
            break;
        case 0x07: /* RLCA */
            tmpb = cpu->af.hi >> 7;
            cpu->af.hi = (cpu->af.hi << 1) | tmpb;
            if (tmpb) SET_CARRY();
            else UNSET_CARRY();
            UNSET_ZERO();
            UNSET_SUB();
            UNSET_HALF();
            c = 4;
            break;
        case 0x08: /* LD (nn),SP */
            tmps = DOUBLE_OP();
            mmu_writes(cpu, tmps, cpu->sp.val);
            c = 20;
            break;
        case 0x09: /* ADD HL,BC */
            ADDREG(cpu->hl, cpu->bc);
            c = 8;
            break;
        case 0x0A: /* LD A,(BC) */
            cpu->af.hi = mmu_readb(cpu, cpu->bc.val);
            c = 8;
            break;
        case 0x0B: /* DEC BC */
            cpu->bc.val--;
            c = 8;
            break;
        case 0x0C: /* INC C */
            INC8(cpu->bc.lo);
            c = 4;
            break;
        case 0x0D: /* DEC C */
            DEC8(cpu->bc.lo);
            c = 4;
            break;
        case 0x0E: /* LD C,n */
            cpu->bc.lo = SINGLE_OP();
            c = 8;
            break;
        case 0x0F: /* RRCA */
            tmpb = cpu->af.hi & 1;
            cpu->af.hi = (cpu->af.hi >> 1) | (tmpb << 7);
            if (tmpb) SET_CARRY();
            else UNSET_CARRY();
            UNSET_ZERO();
            UNSET_SUB();
            UNSET_HALF();
            c = 4;
            break;
        case 0x10: /* STOP */
            /* TODO: implement stop logic */
            c = 0;
            break;
        case 0x11: /* LD DE,nn */
            cpu->de.val = DOUBLE_OP();
            c = 12;
            break;
        case 0x12: /* LD (DE),A */
            mmu_writeb(cpu, cpu->de.val, cpu->af.hi);
            c = 8;
            break;
        case 0x13: /* INC DE */
            cpu->de.val++;
            c = 8;
            break;
        case 0x14: /* INC D */
            INC8(cpu->de.hi);
            c = 4;
            break;
        case 0x15: /* DEC D */
            DEC8(cpu->de.hi);
            c = 4;
            break;
        case 0x16: /* LD D,n */
            cpu->de.hi = SINGLE_OP();
            c = 8;
            break;
        case 0x17: /* RLA */
            tmpb = cpu->af.hi >> 7;
            cpu->af.hi = (cpu->af.hi << 1) | FLAG_CARRY();
            if (tmpb) SET_CARRY();
            else UNSET_CARRY();
            UNSET_ZERO();
            UNSET_SUB();
            UNSET_HALF();
            c = 4;
            break;
        case 0x18: /* JR n */
            cpu->pc.val += (offset)mmu_readb(cpu, cpu->pc.val) + 1;
            c = 12;
            break;
        case 0x19: /* ADD HL,DE */
            ADDREG(cpu->hl, cpu->de);
            c = 8;
            break;
        case 0x1A: /* LD A,(DE) */
            cpu->af.hi = mmu_readb(cpu, cpu->de.val);
            c = 8;
            break;
        case 0x1B: /* DEC DE */
            cpu->de.val--;
            c = 8;
            break;
        case 0x1C: /* INC E */
            INC8(cpu->de.lo);
            c = 4;
            break;
        case 0x1D: /* DEC E */
            DEC8(cpu->de.lo);
            c = 4;
            break;
        case 0x1E: /* LD E,n */
            cpu->de.lo = SINGLE_OP();
            c = 8;
            break;
        case 0x1F: /* RRA */
            tmpb = cpu->af.hi & 1;
            cpu->af.hi = (cpu->af.hi >> 1) | (FLAG_CARRY() << 7);
            if (tmpb) SET_CARRY();
            else UNSET_CARRY();
            UNSET_ZERO();
            UNSET_SUB();
            UNSET_HALF();
            c = 4;
            break;
        case 0x20: /* JR NZ,n */
            if (!FLAG_ZERO()) {
                cpu->pc.val += (offset)mmu_readb(cpu, cpu->pc.val) + 1;
                c = 12;
            } else {
                cpu->pc.val++;
                c = 8;
            }
            break;
        case 0x21: /* LD HL,nn */
            cpu->hl.val = DOUBLE_OP();
            c = 12;
            break;
        case 0x22: /* LDI (HL),A */
            mmu_writeb(cpu, cpu->hl.val++, cpu->af.hi);
            c = 8;
            break;
        case 0x23: /* INC HL */
            cpu->hl.val++;
            c = 8;
            break;
        case 0x24: /* INC H */
            INC8(cpu->hl.hi);
            c = 4;
            break;
        case 0x25: /* DEC H */
            DEC8(cpu->hl.hi);
            c = 4;
            break;
        case 0x26: /* LD H,n */
            cpu->hl.hi = SINGLE_OP();
            c = 8;
            break;
        case 0x27: /* DAA */
            tmpb = 0;
            if (FLAG_HALF() || (cpu->af.hi & 0xf) > 0x9)
                tmpb |= 0x6;
            if (FLAG_CARRY() || ((cpu->af.hi & 0xf0) >> 4) > 0x9)
                tmpb |= 0x60;
            if (FLAG_SUB())
                cpu->af.hi -= tmpb;
            else
                cpu->af.hi += tmpb;
            if (cpu->af.hi == 0) SET_ZERO();
            else UNSET_ZERO();
            if (cpu->af.hi > 0x99) SET_CARRY();
            else UNSET_CARRY();
            UNSET_HALF();
            c = 4;
            break;
        case 0x28: /* JR Z,n */
            if (FLAG_ZERO()) {
                cpu->pc.val += (offset)mmu_readb(cpu, cpu->pc.val) + 1;
                c = 12;
            } else {
                cpu->pc.val++;
                c = 8;
            }
            break;
        case 0x29: /* ADD HL,HL */
            ADDREG(cpu->hl, cpu->hl);
            c = 8;
            break;
        case 0x2A: /* LDI A,(HL) */
            cpu->af.hi = mmu_readb(cpu, cpu->hl.val++);
            c = 8;
            break;
        case 0x2B: /* DEC HL */
            cpu->hl.val--;
            c = 8;
            break;
        case 0x2C: /* INC L */
            INC8(cpu->hl.lo);
            c = 4;
            break;
        case 0x2D: /* DEC L */
            DEC8(cpu->hl.lo);
            c = 4;
            break;
        case 0x2E: /* LD L,n */
            cpu->hl.lo = SINGLE_OP();
            c = 8;
            break;
        case 0x2F: /* CPL */
            cpu->af.hi = ~cpu->af.hi;
            SET_SUB();
            SET_HALF();
            c = 4;
            break;
        case 0x30: /* JR NC,n */
            if (!FLAG_CARRY()) {
                cpu->pc.val += (offset)mmu_readb(cpu, cpu->pc.val) + 1;
                c = 12;
            } else {
                cpu->pc.val++;
                c = 8;
            }
            break;
        case 0x31: /* LD SP,nn */
            cpu->sp.val = DOUBLE_OP();
            c = 12;
            break;
        case 0x32: /* LDD (HL),A */
            mmu_writeb(cpu, cpu->hl.val--, cpu->af.hi);
            c = 8;
            break;
        case 0x33: /* INC SP */
            cpu->sp.val++;
            c = 8;
            break;
        case 0x34: /* INC (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            INC8(tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 12;
            break;
        case 0x35: /* DEC (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            DEC8(tmpb);
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 12;
            break;
        case 0x36: /* LD (HL),n */
            tmpb = SINGLE_OP();
            mmu_writeb(cpu, cpu->hl.val, tmpb);
            c = 12;
            break;
        case 0x37: /* SCF */
            UNSET_SUB();
            UNSET_HALF();
            SET_CARRY();
            c = 4;
            break;
        case 0x38: /* JR C,n */
            if (FLAG_CARRY()) {
                cpu->pc.val += (offset)mmu_readb(cpu, cpu->pc.val) + 1;
                c = 12;
            } else {
                cpu->pc.val++;
                c = 8;
            }
            break;
        case 0x39: /* ADD HL,SP */
            ADDREG(cpu->hl, cpu->sp);
            c = 8;
            break;
        case 0x3A: /* LDD A,(HL) */
            cpu->af.hi = mmu_readb(cpu, cpu->hl.val--);
            c = 8;
            break;
        case 0x3B: /* DEC SP */
            cpu->sp.val--;
            c = 8;
            break;
        case 0x3C: /* INC A */
            INC8(cpu->af.hi);
            c = 4;
            break;
        case 0x3D: /* DEC A */
            DEC8(cpu->af.hi);
            c = 4;
            break;
        case 0x3E: /* LD A,n */
            cpu->af.hi = SINGLE_OP();
            c = 8;
            break;
        case 0x3F: /* CCF */
            UNSET_SUB();
            UNSET_HALF();
            if (FLAG_CARRY()) UNSET_CARRY();
            else SET_CARRY();
            c = 4;
            break;
        case 0x40: /* LD B,B */
            c = 4;
            break;
        case 0x41: /* LD B,C */
            cpu->bc.hi = cpu->bc.lo;
            c = 4;
            break;
        case 0x42: /* LD B,D */
            cpu->bc.hi = cpu->de.hi;
            c = 4;
            break;
        case 0x43: /* LD B,E */
            cpu->bc.hi = cpu->de.lo;
            c = 4;
            break;
        case 0x44: /* LD B,H */
            cpu->bc.hi = cpu->hl.hi;
            c = 4;
            break;
        case 0x45: /* LD B,L */
            cpu->bc.hi = cpu->hl.lo;
            c = 4;
            break;
        case 0x46: /* LD B,(HL) */
            cpu->bc.hi = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x47: /* LD B,A */
            cpu->bc.hi = cpu->af.hi;
            c = 4;
            break;
        case 0x48: /* LD C,B */
            cpu->bc.lo = cpu->bc.hi;
            c = 4;
            break;
        case 0x49: /* LD C,C */
            c = 4;
            break;
        case 0x4A: /* LD C,D */
            cpu->bc.lo = cpu->de.hi;
            c = 4;
            break;
        case 0x4B: /* LD C,E */
            cpu->bc.lo = cpu->de.lo;
            c = 4;
            break;
        case 0x4C: /* LD C,H */
            cpu->bc.lo = cpu->hl.hi;
            c = 4;
            break;
        case 0x4D: /* LD C,L */
            cpu->bc.lo = cpu->hl.lo;
            c = 4;
            break;
        case 0x4E: /* LD C,(HL) */
            cpu->bc.lo = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x4F: /* LD C,A */
            cpu->bc.lo = cpu->af.hi;
            c = 4;
            break;
        case 0x50: /* LD D,B */
            cpu->de.hi = cpu->bc.hi;
            c = 4;
            break;
        case 0x51: /* LD D,C */
            cpu->de.hi = cpu->bc.lo;
            c = 4;
            break;
        case 0x52: /* LD D,D */
            c = 4;
            break;
        case 0x53: /* LD D,E */
            cpu->de.hi = cpu->de.lo;
            c = 4;
            break;
        case 0x54: /* LD D,H */
            cpu->de.hi = cpu->hl.hi;
            c = 4;
            break;
        case 0x55: /* LD D,L */
            cpu->de.hi = cpu->hl.lo;
            c = 4;
            break;
        case 0x56: /* LD D,(HL) */
            cpu->de.hi = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x57: /* LD D,A */
            cpu->de.hi = cpu->af.hi;
            c = 4;
            break;
        case 0x58: /* LD E,B */
            cpu->de.lo = cpu->bc.hi;
            c = 4;
            break;
        case 0x59: /* LD E,C */
            cpu->de.lo = cpu->bc.lo;
            c = 4;
            break;
        case 0x5A: /* LD E,D */
            cpu->de.lo = cpu->de.hi;
            c = 4;
            break;
        case 0x5B: /* LD E,E */
            c = 4;
            break;
        case 0x5C: /* LD E,H */
            cpu->de.lo = cpu->hl.hi;
            c = 4;
            break;
        case 0x5D: /* LD E,L */
            cpu->de.lo = cpu->hl.lo;
            c = 4;
            break;
        case 0x5E: /* LD E,(HL) */
            cpu->de.lo = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x5F: /* LD E,A */
            cpu->de.lo = cpu->af.hi;
            c = 4;
            break;
        case 0x60: /* LD H,B */
            cpu->hl.hi = cpu->bc.hi;
            c = 4;
            break;
        case 0x61: /* LD H,C */
            cpu->hl.hi = cpu->bc.lo;
            c = 4;
            break;
        case 0x62: /* LD H,D */
            cpu->hl.hi = cpu->de.hi;
            c = 4;
            break;
        case 0x63: /* LD H,E */
            cpu->hl.hi = cpu->de.lo;
            c = 4;
            break;
        case 0x64: /* LD H,H */
            c = 4;
            break;
        case 0x65: /* LD H,L */
            cpu->hl.hi = cpu->hl.lo;
            c = 4;
            break;
        case 0x66: /* LD H,(HL) */
            cpu->hl.hi = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x67: /* LD H,A */
            cpu->hl.hi = cpu->af.hi;
            c = 4;
            break;
        case 0x68: /* LD L,B */
            cpu->hl.lo = cpu->bc.hi;
            c = 4;
            break;
        case 0x69: /* LD L,C */
            cpu->hl.lo = cpu->bc.lo;
            c = 4;
            break;
        case 0x6A: /* LD L,D */
            cpu->hl.lo = cpu->de.hi;
            c = 4;
            break;
        case 0x6B: /* LD L,E */
            cpu->hl.lo = cpu->de.lo;
            c = 4;
            break;
        case 0x6C: /* LD L,H */
            cpu->hl.lo = cpu->hl.hi;
            c = 4;
            break;
        case 0x6D: /* LD L,L */
            c = 4;
            break;
        case 0x6E: /* LD L,(HL) */
            cpu->hl.lo = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x6F: /* LD L,A */
            cpu->hl.lo = cpu->af.hi;
            c = 4;
            break;
        case 0x70: /* LD (HL),B */
            mmu_writeb(cpu, cpu->hl.val, cpu->bc.hi);
            c = 8;
            break;
        case 0x71: /* LD (HL),C */
            mmu_writeb(cpu, cpu->hl.val, cpu->bc.lo);
            c = 8;
            break;
        case 0x72: /* LD (HL),D */
            mmu_writeb(cpu, cpu->hl.val, cpu->de.hi);
            c = 8;
            break;
        case 0x73: /* LD (HL),E */
            mmu_writeb(cpu, cpu->hl.val, cpu->de.lo);
            c = 8;
            break;
        case 0x74: /* LD (HL),H */
            mmu_writeb(cpu, cpu->hl.val, cpu->hl.hi);
            c = 8;
            break;
        case 0x75: /* LD (HL),L */
            mmu_writeb(cpu, cpu->hl.val, cpu->hl.lo);
            c = 8;
            break;
        case 0x76: /* HALT */
            cpu->halt = 1;
            c = 4;
            break;
        case 0x77: /* LD (HL),A */
            mmu_writeb(cpu, cpu->hl.val, cpu->af.hi);
            c = 8;
            break;
        case 0x78: /* LD A,B */
            cpu->af.hi = cpu->bc.hi;
            c = 4;
            break;
        case 0x79: /* LD A,C */
            cpu->af.hi = cpu->bc.lo;
            c = 4;
            break;
        case 0x7A: /* LD A,D */
            cpu->af.hi = cpu->de.hi;
            c = 4;
            break;
        case 0x7B: /* LD A,E */
            cpu->af.hi = cpu->de.lo;
            c = 4;
            break;
        case 0x7C: /* LD A,H */
            cpu->af.hi = cpu->hl.hi;
            c = 4;
            break;
        case 0x7D: /* LD A,L */
            cpu->af.hi = cpu->hl.lo;
            c = 4;
            break;
        case 0x7E: /* LD A,(HL) */
            cpu->af.hi = mmu_readb(cpu, cpu->hl.val);
            c = 8;
            break;
        case 0x7F: /* LD A,A */
            c = 4;
            break;
        case 0x80: /* ADD A,B */
            ADD8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0x81: /* ADD A,C */
            ADD8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0x82: /* ADD A,D */
            ADD8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0x83: /* ADD A,E */
            ADD8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0x84: /* ADD A,H */
            ADD8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0x85: /* ADD A,L */
            ADD8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0x86: /* ADD A,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            ADD8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x87: /* ADD A,A */
            ADD8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0x88: /* ADC A,B */
            ADC8(cpu->af.hi, cpu->bc.hi, tmpb);
            c = 4;
            break;
        case 0x89: /* ADC A,C */
            ADC8(cpu->af.hi, cpu->bc.lo, tmpb);
            c = 4;
            break;
        case 0x8A: /* ADC A,D */
            ADC8(cpu->af.hi, cpu->de.hi, tmpb);
            c = 4;
            break;
        case 0x8B: /* ADC A,E */
            ADC8(cpu->af.hi, cpu->de.lo, tmpb);
            c = 4;
            break;
        case 0x8C: /* ADC A,H */
            ADC8(cpu->af.hi, cpu->hl.hi, tmpb);
            c = 4;
            break;
        case 0x8D: /* ADC A,L */
            ADC8(cpu->af.hi, cpu->hl.lo, tmpb);
            c = 4;
            break;
        case 0x8E: /* ADC A,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            ADC8(cpu->af.hi, tmpb, tmpb2);
            c = 8;
            break;
        case 0x8F: /* ADC A,A */
            ADC8(cpu->af.hi, cpu->af.hi, tmpb);
            c = 4;
            break;
        case 0x90: /* SUB A,B */
            SUB8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0x91: /* SUB A,C */
            SUB8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0x92: /* SUB A,D */
            SUB8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0x93: /* SUB A,E */
            SUB8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0x94: /* SUB A,H */
            SUB8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0x95: /* SUB A,L */
            SUB8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0x96: /* SUB A,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SUB8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0x97: /* SUB A,A */
            SUB8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0x98: /* SBC A,B */
            SBC8(cpu->af.hi, cpu->bc.hi, tmpb);
            c = 4;
            break;
        case 0x99: /* SBC A,C */
            SBC8(cpu->af.hi, cpu->bc.lo, tmpb);
            c = 4;
            break;
        case 0x9A: /* SBC A,D */
            SBC8(cpu->af.hi, cpu->de.hi, tmpb);
            c = 4;
            break;
        case 0x9B: /* SBC A,E */
            SBC8(cpu->af.hi, cpu->de.lo, tmpb);
            c = 4;
            break;
        case 0x9C: /* SBC A,H */
            SBC8(cpu->af.hi, cpu->hl.hi, tmpb);
            c = 4;
            break;
        case 0x9D: /* SBC A,L */
            SBC8(cpu->af.hi, cpu->hl.lo, tmpb);
            c = 4;
            break;
        case 0x9E: /* SBC A,(HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            SBC8(cpu->af.hi, tmpb, tmpb2);
            c = 8;
            break;
        case 0x9F: /* SBC A,A */
            SBC8(cpu->af.hi, cpu->af.hi, tmpb);
            c = 4;
            break;
        case 0xA0: /* AND B */
            AND8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0xA1: /* AND C */
            AND8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0xA2: /* AND D */
            AND8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0xA3: /* AND E */
            AND8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0xA4: /* AND H */
            AND8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0xA5: /* AND L */
            AND8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0xA6: /* AND (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            AND8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xA7: /* AND A */
            AND8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0xA8: /* XOR B */
            XOR8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0xA9: /* XOR C */
            XOR8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0xAA: /* XOR D */
            XOR8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0xAB: /* XOR E */
            XOR8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0xAC: /* XOR H */
            XOR8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0xAD: /* XOR L */
            XOR8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0xAE: /* XOR (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            XOR8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xAF: /* XOR A */
            XOR8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0xB0: /* OR B */
            OR8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0xB1: /* OR C */
            OR8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0xB2: /* OR D */
            OR8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0xB3: /* OR E */
            OR8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0xB4: /* OR H */
            OR8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0xB5: /* OR L */
            OR8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0xB6: /* OR (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            OR8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xB7: /* OR A */
            OR8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0xB8: /* CP B */
            CP8(cpu->af.hi, cpu->bc.hi);
            c = 4;
            break;
        case 0xB9: /* CP C */
            CP8(cpu->af.hi, cpu->bc.lo);
            c = 4;
            break;
        case 0xBA: /* CP D */
            CP8(cpu->af.hi, cpu->de.hi);
            c = 4;
            break;
        case 0xBB: /* CP E */
            CP8(cpu->af.hi, cpu->de.lo);
            c = 4;
            break;
        case 0xBC: /* CP H */
            CP8(cpu->af.hi, cpu->hl.hi);
            c = 4;
            break;
        case 0xBD: /* CP L */
            CP8(cpu->af.hi, cpu->hl.lo);
            c = 4;
            break;
        case 0xBE: /* CP (HL) */
            tmpb = mmu_readb(cpu, cpu->hl.val);
            CP8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xBF: /* CP A */
            CP8(cpu->af.hi, cpu->af.hi);
            c = 4;
            break;
        case 0xC0: /* RET NZ */
            if (!FLAG_ZERO()) {
                cpu->pc.val = POP();
                c = 20;
            } else {
                c = 8;
            }
            break;
        case 0xC1: /* POP BC */
            cpu->bc.val = POP();
            c = 12;
            break;
        case 0xC2: /* JP NZ,nn */
            if (!FLAG_ZERO()) {
                cpu->pc.val = mmu_reads(cpu, cpu->pc.val);
                c = 16;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xC3: /* JP nn */
            cpu->pc.val = mmu_reads(cpu, cpu->pc.val);
            c = 16;
            break;
        case 0xC4: /* CALL NZ,nn */
            if (!FLAG_ZERO()) {
                CALL();
                c = 24;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xC5: /* PUSH BC */
            PUSH(cpu->bc.val);
            c = 16;
            break;
        case 0xC6: /* ADD A,n */
            tmpb = SINGLE_OP();
            ADD8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xC7: /* RST 0 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x0;
            c = 16;
            break;
        case 0xC8: /* RET Z */
            if (FLAG_ZERO()) {
                cpu->pc.val = POP();
                c = 20;
            } else {
                c = 8;
            }
            break;
        case 0xC9: /* RET */
            cpu->pc.val = POP();
            c = 16;
            break;
        case 0xCA: /* JP Z,nn */
            if (FLAG_ZERO()) {
                cpu->pc.val = mmu_reads(cpu, cpu->pc.val);
                c = 16;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xCB: /* Two-byte operations prefix */
            c = prefix_instr(cpu);
            break;
        case 0xCC: /* CALL Z,nn */
            if (FLAG_ZERO()) {
                CALL();
                c = 24;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xCD: /* CALL nn */
            CALL();
            c = 24;
            break;
        case 0xCE: /* ADC A,n */
            tmpb = SINGLE_OP();
            ADC8(cpu->af.hi, tmpb, tmpb2);
            c = 8;
            break;
        case 0xCF: /* RST 8 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x8;
            c = 16;
            break;
        case 0xD0: /* RET NC */
            if (!FLAG_CARRY()) {
                cpu->pc.val = POP();
                c = 20;
            } else {
                c = 8;
            }
            break;
        case 0xD1: /* POP DE */
            cpu->de.val = POP();
            c = 12;
            break;
        case 0xD2: /* JP NC,nn */
            if (!FLAG_CARRY()) {
                cpu->pc.val = mmu_reads(cpu, cpu->pc.val);
                c = 16;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xD3: /* Not implemented */
            c = 0;
            break;
        case 0xD4: /* CALL NC,nn */
            if (!FLAG_CARRY()) {
                CALL();
                c = 24;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xD5: /* PUSH DE */
            PUSH(cpu->de.val);
            c = 16;
            break;
        case 0xD6: /* SUB A,n */
            tmpb = SINGLE_OP();
            SUB8(cpu->af.hi, tmpb);
            c = 16;
            break;
        case 0xD7: /* RST 10 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x10;
            c = 16;
            break;
        case 0xD8: /* RET C */
            if (FLAG_CARRY()) {
                cpu->pc.val = POP();
                c = 20;
            } else {
                c = 8;
            }
            break;
        case 0xD9: /* RETI */
            cpu->ei = 1;
            cpu->pc.val = POP();
            c = 16;
            break;
        case 0xDA: /* JP C,nn */
            if (FLAG_CARRY()) {
                cpu->pc.val = mmu_reads(cpu, cpu->pc.val);
                c = 16;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xDB: /* Not implemented */
            c = 0;
            break;
        case 0xDC: /* CALL C,nn */
            if (FLAG_CARRY()) {
                CALL();
                c = 24;
            } else {
                cpu->pc.val += 2;
                c = 12;
            }
            break;
        case 0xDD: /* Not implemented */
            c = 0;
            break;
        case 0xDE: /* SBC A,n */
            tmpb = SINGLE_OP();
            SBC8(cpu->af.hi, tmpb, tmpb2);
            c = 8;
            break;
        case 0xDF: /* RST 18 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x18;
            c = 16;
            break;
        case 0xE0: /* LDH (n),A */
            tmpb = SINGLE_OP();
            mmu_writeb(cpu, 0xff00 + tmpb, cpu->af.hi);
            c = 12;
            break;
        case 0xE1: /* POP HL */
            cpu->hl.val = POP();
            c = 12;
            break;
        case 0xE2: /* LDH (C),A */
            mmu_writeb(cpu, 0xff00 + cpu->bc.lo, cpu->af.hi);
            c = 8;
            break;
        case 0xE3: /* Not implemented */
        case 0xE4: /* Not implemented */
            c = 0;
            break;
        case 0xE5: /* PUSH HL */
            PUSH(cpu->hl.val);
            c = 16;
            break;
        case 0xE6: /* AND n */
            tmpb = SINGLE_OP();
            AND8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xE7: /* RST 20 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x20;
            c = 16;
            break;
        case 0xE8: /* ADD SP,d */
            tmpb = SINGLE_OP();
            if (((cpu->sp.lo & 0xf) + (tmpb & 0xf)) & 0x10) SET_HALF();
            else UNSET_HALF();
            if (cpu->sp.lo + tmpb < cpu->sp.lo) SET_CARRY();
            else UNSET_CARRY();
            cpu->sp.val += (offset)tmpb;
            UNSET_ZERO();
            UNSET_SUB();
            c = 16;
            break;
        case 0xE9: /* JP HL */
            cpu->pc.val = cpu->hl.val;
            c = 4;
            break;
        case 0xEA: /* LD (nn),A */
            tmps = DOUBLE_OP();
            mmu_writeb(cpu, tmps, cpu->af.hi);
            c = 16;
            break;
        case 0xEB: /* Not implemented */
        case 0xEC: /* Not implemented */
        case 0xED: /* Not implemented */
            c = 0;
            break;
        case 0xEE: /* XOR n */
            tmpb = SINGLE_OP();
            XOR8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xEF: /* RST 28 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x28;
            c = 16;
            break;
        case 0xF0: /* LDH A,(n) */
            tmpb = SINGLE_OP();
            cpu->af.hi = mmu_readb(cpu, 0xff00 + tmpb);
            c = 12;
            break;
        case 0xF1: /* POP AF */
            cpu->af.val = POP();
            c = 12;
            break;
        case 0xF2: /* LDH A,(C) */
            cpu->af.hi = mmu_readb(cpu, 0xff00 + cpu->bc.lo);
            c = 8;
            break;
        case 0xF3: /* DI */
            cpu->ei = 0;
            cpu->ime = 0;
            c = 4;
            break;
        case 0xF4: /* Not implemented */
            c = 0;
            break;
        case 0xF5: /* PUSH AF */
            PUSH(cpu->af.val);
            c = 16;
            break;
        case 0xF6: /* OR n */
            tmpb = SINGLE_OP();
            OR8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xF7: /* RST 30 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x30;
            c = 16;
            break;
        case 0xF8: /* LDHL SP,d */
            tmpb = SINGLE_OP();
            if (((cpu->sp.lo & 0xf) + (tmpb & 0xf)) & 0x10) SET_HALF();
            else UNSET_HALF();
            if (cpu->sp.lo + tmpb < cpu->sp.lo) SET_CARRY();
            else UNSET_CARRY();
            cpu->hl.val = cpu->sp.val + (offset)tmpb;
            UNSET_ZERO();
            UNSET_SUB();
            c = 12;
            break;
        case 0xF9: /* LD SP,HL */
            cpu->sp.val = cpu->hl.val;
            c = 8;
            break;
        case 0xFA: /* LD A,(nn) */
            tmps = DOUBLE_OP();
            cpu->af.hi = mmu_readb(cpu, tmps);
            c = 16;
            break;
        case 0xFB: /* EI */
            cpu->ei = 1;
            c = 4;
            break;
        case 0xFC: /* Not implemented */
        case 0xFD: /* Not implemented */
            c = 0;
            break;
        case 0xFE: /* CP n */
            tmpb = SINGLE_OP();
            CP8(cpu->af.hi, tmpb);
            c = 8;
            break;
        case 0xFF: /* RST 38 */
            PUSH(cpu->pc.val);
            cpu->pc.val = 0x38;
            c = 16;
            break;
        default:
            c = 0;
    }

    if (!c)
        die("[cpu_step] unknown opcode: %x", op);
    return c;
}

static void
cpu_setup(cpu_t *cpu)
{
    cpu->af.hi = 0x1;
    cpu->af.lo = 0x80;
    cpu->bc.hi = 0;
    cpu->bc.lo = 0x13;
    cpu->de.hi = 0;
    cpu->de.lo = 0xd8;
    cpu->hl.hi = 0x1;
    cpu->hl.lo = 0x4d;
    cpu->pc.val = 0x0100;
    cpu->sp.val = 0xfffe;
    cpu->ime = FALSE;
}

void
cpu_init(cpu_t *cpu, gb_t *bus)
{
    cpu->bus = bus;
    cpu->halt = cpu->ei = FALSE;
    cpu_setup(cpu);
}
