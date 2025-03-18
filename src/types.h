#ifndef __TYPES_H
#define __TYPES_H

#include <assert.h>
#include <errno.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility macros */
#define gb_die(err)                                                 \
    do {                                                            \
        fprintf(stderr, "(" __FILE__ ":%s@%u) fatal error: %s\n",   \
                __func__, __LINE__, strerror(err));                 \
        abort();                                                    \
    } while (0)

/* this is for filling a switch's default path */
#define unreachable()   __builtin_unreachable()

/* this is for converting a number to a bool */
#define to_bool(x)      (!!(x))

/* get one bit in position n plus masking */
#define BIT(n)          (1 << n)
#define MASK(n)         (BIT(n) - 1)

/* this represents a 16-bit value and its 8-bit components (low and high) */
typedef union reg {
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
    struct {
        uint8_t lo;
        uint8_t hi;
    };
#elif __BYTE_ORDER__ == __BIG_ENDIAN
    struct {
        uint8_t hi;
        uint8_t lo;
    }
#else
#error "Byte order is unknown"
#endif
    uint16_t val;
} reg_t;

/* a pixel in RGBA8888 format. by accessing its 32bit field "val", it is
 * possible to set it comfortably in any endianness.
 * example: pixel_t px = 0xFF25FFFF; (R=0xFF, G=0x25, B=0xFF, A=0xFF) */
typedef struct pixel {
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
    union {
        struct __attribute__((packed)) {
            uint8_t a, b, g, r;
        };
#elif __BYTE_ORDER__ == __BIG_ENDIAN
        struct __attribute__((packed)) {
            uint8_t r, g, b, a;
        };
#else
#error "Byte order is unknown"
#endif
        uint32_t val;
    };
} pixel_t;

/* take lower byte from a 16-bit value */
static inline uint8_t _low_byte(uint16_t x)
{
    return (uint8_t)(x & 0xFF);
}

/* take higher byte from a 16-bit value */
static inline uint8_t _high_byte(uint16_t x)
{
    return (uint8_t)((x & 0xFF00) >> 16);
}

#endif /* __TYPES_H */
