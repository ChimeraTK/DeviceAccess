/**
 *      @file           libptime.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides class for time measurement.                 
 */

#ifndef LIBTIME_H
#define	LIBTIME_H

#include <time.h>
#include <sys/time.h>
/**
 * @brief  Provides method to measure time between execution of tp_start and tp_stop functions.
 * 
 * Class handles functionality of time measurement. Functionallity is based on the system-wide 
 * real-time clock, which is identified by CLOCK_REALTIME.  Its time represents seconds and 
 * nanoseconds since the Epoch. 
 * Example: 
 * @snippet test-libptime.cpp TIME time measurement example
 */
class timePeriod {
private:
    struct timespec tp_begin; /**< value of second and nanoseconf from the beginning of Epoch after execution of tp_start function*/
    struct timespec tp_end; /**< value of second and nanoseconf from the beginning of Epoch after execution of tp_stop function*/
    struct timespec tp_result; /**< difference between tp_end and tp_begin*/
private:
    /**
     * @brief Calculate difference between tp_end and tp_begin expressed as a struct timespec.      
     * 
     * @return Difference between two times
     */
    struct timespec sub();
public:

    /**
     * @brief  Defines unit of meaurement of time calculation
     */
    typedef enum _M {
        MS = 1000000, /**< result will be represented as a miliseconds*/
        US = 1000, NS = 1 /**< result will be represented as a microseconds*/
    } TIME_MEAS;
    /**
     * @brief Store informatin about current seconds and nanoseconds in tp_begin structure.      
     */
    void tp_start();
    /**
     * @brief Store informatin about current seconds and nanoseconds in tp_end structure.      
     */
    void tp_stop();
    /**
     * Calculate time from execution of tp_start to tp_stop
     * 
     * @param tm set unit of measurement of returned value - seconds or miliseconds
     * @return time that elapse between execution of tp_start and tp_stop
     */
    double tp_getperiod(TIME_MEAS tm);
};

/**
 * @example test-libptime.cpp
 */


#endif	/* LIBTIME_H */

