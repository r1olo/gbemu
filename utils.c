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
    void *ret = malloc(size);

    if (!ret)
        die("[%s] can't allocate %s:", fname, name);

    return ret;
}

pqueue_t *
pqueue_create()
{
    pqueue_t *queue = malloc_or_die(sizeof(pqueue_t), "pqueue_create", "queue");
    queue->len = 0;
    queue->head = NULL;
    return queue;
}

void
pqueue_put(pqueue_t *queue, pixel_t pixel)
{
    pnode_t *node = malloc_or_die(sizeof(pnode_t), "pqueue_put", "node");
    node->pixel = pixel;
    node->next = NULL;
    pnode_t *cur = queue->head;
    while (cur && cur->next)
        cur = cur->next;
    if (!cur)
        queue->head = node;
    else
        cur->next = node;
    queue->len++;
}

pixel_t
pqueue_remove(pqueue_t *queue)
{
    pixel_t ret;
    pnode_t *node;
    if (!queue->len)
        die("[pqueue_remove] pixel queue is empty");
    node = queue->head;
    ret = node->pixel;
    queue->head = node->next;
    queue->len--;
    free(node);
    return ret;
}

void
pqueue_clear(pqueue_t *queue)
{
    pnode_t *cur;
    while (queue->head) {
        cur = queue->head;
        queue->head = cur->next;
        free(cur);
    }
    queue->len = 0;
}
