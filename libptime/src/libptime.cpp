#include "libptime.h"
#include <stdio.h>

void timePeriod::tp_start() {
    clock_gettime(CLOCK_MONOTONIC, &tp_begin);
}

void timePeriod::tp_stop() {
    clock_gettime(CLOCK_MONOTONIC, &tp_end);
    tp_result = sub();
}

double timePeriod::tp_getperiod(TIME_MEAS tm) {
    return (double) ((tp_result.tv_nsec + tp_result.tv_sec * 1e9)) / tm;
}

struct timespec timePeriod::sub() {
    struct timespec tp_result;
    if ((tp_end.tv_sec < tp_begin.tv_sec) || ((tp_end.tv_sec == tp_begin.tv_sec) && (tp_end.tv_nsec <= tp_begin.tv_nsec))) {
        tp_result.tv_sec = tp_result.tv_nsec = 0;
    } else { /* TIME1 > TIME2 */
        tp_result.tv_sec = tp_end.tv_sec - tp_begin.tv_sec;
        if (tp_end.tv_nsec < tp_begin.tv_nsec) {
            tp_result.tv_nsec = tp_end.tv_nsec + 1000000000L - tp_begin.tv_nsec;
            tp_result.tv_sec--; /* Borrow a second. */
        } else {
            tp_result.tv_nsec = tp_end.tv_nsec - tp_begin.tv_nsec;
        }
    }
    return (tp_result);
}


