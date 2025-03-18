#ifndef __CART_H
#define __CART_H

#include <gbemu/errors.h>
#include <stdint.h>
#include <stdio.h>

/* cartridge type. it may have different read and write functions depending on
 * the MBC type */
typedef struct cart {
    void    *mbc_data;
    uint8_t (*read_rom)(struct cart *cart, uint16_t addr);
    void    (*write_rom)(struct cart *cart, uint16_t addr, uint8_t val);
    uint8_t (*read_ram)(struct cart *cart, uint16_t addr);
    void    (*write_ram)(struct cart *cart, uint16_t addr, uint8_t val);
    uint8_t bank0[0x4000];
} cart_t;

enum gb_err cart_init(cart_t *cart, FILE *file);
enum gb_err cart_create(cart_t **pcart, const char *filename);
void        cart_destroy(cart_t *cart);

#endif /* __CART_H */
