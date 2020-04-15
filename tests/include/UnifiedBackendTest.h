#pragma once

#include <string>
#include <functional>

class UnifiedBackendTest {
 public:
  /**
   *  Test basic exception handling behavior
   * 
   *  cdd: device descriptor to use for the test
   *  forceExceptionsReadWrite: functor, which will do whatever necessary that the backend will throw a
   *                            ChimeraTK::runtime_error for any read and write operation.
   */
  void basicExceptionHandling(
      std::string cdd, std::string registerName, std::function<void(void)> forceExceptionsReadWrite);
};
