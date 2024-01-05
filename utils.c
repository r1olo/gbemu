#include <stdarg.h>

#include "utils.h"

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
