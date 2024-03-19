#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gbemu.h"

/* use our custom pre-allocated memory for allocations (for enhanced locality) */
static char my_memory[0x20000];

static block_t blocks = {
    .size = sizeof(my_memory),
    .ptr = my_memory,
    .free = 1,
    .next = NULL,
};

static void *use_block(block_t *blk, size_t size)
{
    block_t *first_half = blk, *second_half;

    second_half = malloc(sizeof(block_t));
    second_half->size = first_half->size - size;
    second_half->ptr = first_half->ptr + size;
    second_half->free = 1;
    second_half->next = first_half->next;

    first_half->size = size;
    first_half->free = 0;
    first_half->next = second_half;

    return first_half->ptr;
}

static void merge_all_blocks(block_t *list)
{
    block_t *cur = list, *to_free;

    for (; cur; cur = cur->next) {
        if (!cur->free)
            continue;
        while (cur->next && cur->next->free) {
            to_free = cur->next;
            cur->size += cur->next->size;
            cur->next = cur->next->next;
            free(to_free);
        }
    }
}

static void *_malloc(block_t *list, size_t size)
{
    block_t *cur = list;

    while (cur && (!cur->free || size > cur->size))
        cur = cur->next;

    if (!cur)
        return NULL;
    
    return use_block(cur, size);
}

static void _free(block_t *list, void *ptr)
{
    block_t *cur = list;

    while (cur && ptr != cur->ptr)
        cur = cur->next;

    if (ptr != cur->ptr)
        return;

    cur->free = 1;

    merge_all_blocks(list);
}

void *alloc_mem(size_t size)
{
    return _malloc(&blocks, size);
}

void free_mem(void *ptr)
{
    _free(&blocks, ptr);
}
