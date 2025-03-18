#ifndef __LOG_H
#define __LOG_H

enum log_type {
    LOG_VERBOSE,
    LOG_INFO,
    LOG_ERR
};

#ifndef NDEBUG
extern enum log_type _cur_log_lvl;
void _log(const char *fn, enum log_type type, const char *fmt, ...);
#define LOG(...) _log(__FUNCTION__, __VA_ARGS__)
#else
#define LOG(...) (void)(0)
#endif /* NDEBUG */

#endif /* __LOG_H */
