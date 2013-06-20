#ifndef _DEBUGG_MODE_H
#define	_DEBUGG_MODE_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

/** \file dbg_print.h
 *  \brief Defines dbg_print function used to display debug information in console
 */

#ifdef __DEBUG_MODE__
    #define dbg_print(format, ...)  do {                                                                                            \
                                    time_t t;                                                                                       \
                                    struct tm r;                                                                                    \
                                    t = time(NULL);                                                                                 \
                                    localtime_r(&t, &r);                                                                            \
                                    fprintf(stderr, "[%08lu] %4d-%02d-%02d %02d:%02d:%02d %s:%d %s - " format, syscall(SYS_gettid), r.tm_year + 1900, r.tm_mon + 1, r.tm_mday, r.tm_hour + 1, r.tm_min, r.tm_sec, __FILE__, __LINE__, __FUNCTION__,## __VA_ARGS__); \
                                    } while (0);
#else
    #define dbg_print(...)
#endif

#endif	/* _DEBUGG_MODE_H */

