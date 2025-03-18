#include "log.h"

#include <stdarg.h>
#include <stdio.h>

enum log_type _cur_log_lvl = LOG_ERR;

void
_log(const char *fn, enum log_type type, const char *fmt, ...)
{
    /* if log type is less than current level, return immediately */
    if (type < _cur_log_lvl)
        return;

    /* get variable args */
    va_list ap;
    va_start(ap, fmt);

    /* print function name and log */
    fprintf(stderr, "[%s]: ", fn);
    vfprintf(stderr, fmt, ap);

    /* end variable args */
    va_end(ap);

    /* append new line */
    putc('\n', stderr);
}
