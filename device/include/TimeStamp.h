#ifndef MTCA4U_TIME_STAMP_H
#define MTCA4U_TIME_STAMP_H

#include <boost/date_time.hpp>

namespace mtca4u {

  /**
   * The MTCA4U time stamp consists of the unix time stamp (number of seconds
   * since 01.01.1970 00:00:00 UTC).
   * The unsigned 32 bit value will have an overflow on Sun, 7 February 2106.
   * If you need higher resolution than once per second, the nanoseconds field
   * can be used.
   * In addition two 32 bit user indices are provided, which can hold for
   * instance a run and an event number.
   */
  struct TimeStamp {

    /**
     * Default constructor. Initializes all fields with zeros.
     */
    inline TimeStamp() :
        seconds(0), nanoSeconds(0), index0(0), index1(0) {
    }

    /**
     * The constructor can set all parameters. Only the seconds have to be
     * specified, all other values default to 0.
     */
    inline TimeStamp(uint32_t seconds_, uint32_t nanoSeconds_ = 0,
        uint32_t index0_ = 0, uint32_t index1_ = 0) :
        seconds(seconds_), nanoSeconds(nanoSeconds_), index0(index0_), index1(
            index1_) {
    }

    uint32_t seconds; ///< Unix time in seconds
    uint32_t nanoSeconds; ///< Nano seconds should give enough resolution
    uint32_t index0; ///< An index to hold a unique number, for instance an event number.
    uint32_t index1; ///< Another index to hold a unique number, for instance a run number.

    inline bool operator==(TimeStamp const & right) {
      return ((seconds == right.seconds) && (nanoSeconds == right.nanoSeconds)
          && (index0 == right.index0) && (index1 == right.index1));
    }

    /**
     * Returns the time-stamp corresponding to the current system time.
     * Optionally, the two index numbers may be specified, which are simply
     * passed on to the constructor.
     */
    static inline TimeStamp currentTime(uint32_t index0 = 0,
        uint32_t index1 = 0) {
      // If we can use gettimeofday(), we prefer it because it is much more
      // efficient than doing all the time conversions.
#ifdef BOOST_HAS_GETTIMEOFDAY
      ::timeval now;
      // We intentionally do not check the return code. Boost does not check it
      // either and it should always be zero, unless our parameters are invalid.
      ::gettimeofday(&now, NULL);
      return TimeStamp(now.tv_sec, now.tv_usec * 1000, index0, index1);
#else // BOOST_HAS_GETTIMEOFDAY
      boost::posix_time::ptime now =
      boost::posix_time::microsec_clock::universal_time();
      boost::posix_time::ptime epoch = boost::posix_time::ptime(
          boost::gregorian::date(1970, 01, 01));
      boost::posix_time::time_duration diff = now - epoch;
      long seconds = diff.total_seconds();
      diff -= boost::posix_time::time_duration(0, 0, seconds, 0);
      long nanoseconds = diff.total_nanoseconds();
      return TimeStamp(seconds, nanoseconds, index0, index1);
#endif // BOOST_HAS_GETTIMEOFDAY
    }

  };

} // namespace mtca4u

#endif // MTCA4U_TIME_STAMP_H
