#ifndef CHECK_TIMEOUT_H
#define CHECK_TIMEOUT_H

#include <boost/test/test_tools.hpp>

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                      \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(!(condition)) {                                                                                              \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)0

#define CHECK_EQUAL_TIMEOUT(left, right, maxMilliseconds)                                                              \
  {                                                                                                                    \
    CHECK_TIMEOUT(left == right, maxMilliseconds);                                                                     \
    BOOST_CHECK_EQUAL(left, right);                                                                                    \
  }                                                                                                                    \
  (void)0

#endif // CHECK_TIMEOUT_H
