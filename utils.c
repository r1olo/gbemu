#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "gbemu.h"

void __attribute__((noreturn))
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    
    if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fputc('\n', stderr);
    }

    exit(1);
}

void *
malloc_or_die(uint size, const char *fname, const char *name)
{
    void *ret = alloc_mem(size);

    if (!ret)
        die("[%s] can't allocate %s:", fname, name);

    return ret;
}

pqueue_t *
pqueue_create()
{
    pqueue_t *q = malloc_or_die(sizeof(pqueue_t), "pqueue_create", "queue");
    q->begin = q->end = 0;
    q->len = 0;
    return q;
}

void
pqueue_push(pqueue_t *queue, pixel_t pixel)
{
    size_t next_begin = queue->begin + 1;

    if (next_begin == sizeof(queue->queue) / sizeof(pixel_t))
        next_begin = 0;

    if (next_begin == queue->end)
        die("[pqueue_push] pixel queue is full");

    queue->queue[queue->begin] = pixel;
    queue->begin = next_begin;
    ++queue->len;
}

pixel_t
pqueue_pop(pqueue_t *queue)
{
    pixel_t ret;
    size_t next_end;

    if (queue->end == queue->begin)
        die("[pqueue_pop] pixel queue is empty");

    next_end = queue->end + 1;
    if (queue->end == sizeof(queue->queue) / sizeof(pixel_t))
        next_end = 0;

    ret = queue->queue[queue->end];
    queue->end = next_end;
    --queue->len;
    return ret;
}

void
pqueue_clear(pqueue_t *queue)
{
    queue->begin = queue->end = 0;
    queue->len = 0;
}

