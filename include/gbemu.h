/*
 *      GBEmu Public API
 */

#ifndef GBEMU_H
#define GBEMU_H

/* main GameBoy type */
typedef struct gb gb_t;

/* perform one step */
void gb_step(gb_t *gb);

#endif /* GBEMU_H */
