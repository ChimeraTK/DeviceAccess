#pragma once

#include <string>
#include <functional>
#include <list>

#include <boost/fusion/include/at_key.hpp>
#include <boost/test/unit_test.hpp>

#include "Device.h"

namespace ctk = ChimeraTK;

/**
 *  Class to test any backend for correct behavior. Instantiate this class and call all (!) preparatory functions to
 *  provide the tests with the backend-specific test actions etc. Finally call runTests() to execute all tests.
 *  Internally the BOOST unit test framework is used, so this shall be called inside a normal unit test.
 * 
 *  Failing to call all preparatory functions will result in an error. This allows a safe test schema evolution - if
 *  more backend specific actions for enabling and disabling test conditions are needed for the tests and the backend
 *  test has not yet been updated, tests will fail.
 * 
 *  Actions are usually speficied as list of pairs of functors. The pair's first element is always the action to enable
 *  the the test condition, the second is the action to disable it. By providing multiple entries in the lists it is
 *  possible to test several code paths the backend has to end up in the intended test condition.
 *  For example in case of forceRuntimeErrorOnRead(): runtime_errors in a read can be caused by a timeout in the
 *  communication channel, or by a bad reply of the device. Two list entries would be provided in this case, one to make
 *  read operations run into timeouts, and one to make the (dummy) device reply with garbage.
 *  If only one singe code path exists to get to the test condition, it is perfectly fine to have only a single entry
 *  in the list.
 * 
 *  In the same way as for the actions, names of registers etc. are provided as lists, so all test can be repeated for
 *  different registers, if required for full coverage.
 * 
 *  See factory function makeUnifiedBackendTest() how to generate instances.
 * 
 *  Note: This is work in progress. Tests are by far not yet complete. Interface changes of the test class are also
 *  likely.
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
class UnifiedBackendTest {
 public:
  /** See factory function makeUnifiedBackendTest() how to generate instances. */
  UnifiedBackendTest(
      GET_REMOTE_VALUE_CALLABLE_T getRemoteValueCallable, std::function<void(std::string)> setRemoteValueCallable)
  : _getRemoteValueCallable(getRemoteValueCallable), _setRemoteValueCallable(setRemoteValueCallable) {}

  /**
   *  "Strong typedef" for list of pairs of functors for enabling and disbaling a test condition.
   */
  class EnableDisableActionList : public std::list<std::pair<std::function<void(void)>, std::function<void(void)>>> {
   public:
    using std::list<std::pair<std::function<void(void)>, std::function<void(void)>>>::list;
  };

  /**
   *  Execute all tests. Call this function within a BOOST_AUTO_TEST_CASE after calling all preparatory functions below.
   *  The tests are executed for the backend identified by the given CDD.
   */
  void runTests(const std::string& cdd);

  /**
   *  Set list of enable/disable actions for the following test condition: Communication is broken, all reads fail with
   *  a runtime_error.
   */
  void forceRuntimeErrorOnRead(const EnableDisableActionList& list) { forceExceptionsRead = list; }

  /**
   *  Set functor, which will do whatever necessary that the backend will throw a ChimeraTK::runtime_error for any write
   *  operation.
   */
  void forceRuntimeErrorOnWrite(const EnableDisableActionList& list) { forceExceptionsWrite = list; }

  /**
   *  Set functors, which will do whatever necessary that data will be lost in write operations.
   *  The enable function returns the number of write operations to be executed before data loss occurs. If
   *  std::numeric_limits<size_t>::max() is returned, the test will be skipped for the given register and the disable
   *  function will not be called. Otherwise it is guaranteed that disable is called for each register enable
   *  was called before. The remote value will only be checked after disable has been called. Disable hence needs to
   *  block until the buffers actually have been flushed, so a subsequent remote value test will not fail supriously.
   */
  void forceDataLossWrite(std::function<size_t(std::string)> enable, std::function<void(std::string)> disable) {
    _enableForceDataLossWrite = enable;
    _disableForceDataLossWrite = disable;
  }

  /**
   *  Set functors, which will do whatever necessary that data last received via a push-type subscription is
   *  inconsistent with the actual value (as read by a synchronous read). This can e.g. be achieved by changing the
   *  value without publishng the update to the subscribers.
   *  
   *  The functor receives the register name to be put into an inconsistent state as an argument. The test will use the
   *  getRemoteValueCallable to obtain the true value which the accessor should eventually become consistent to, so
   *  the implementation is free to change the actual value of the register.
   * 
   *  If it is impossible to create an inconsistent state (e.g. because the used protocol already implements measures
   *  to prevent this), this function shall not be called. The corresponding tests will then be disabled.
   */
  void forceAsyncReadInconsistency(std::function<void(std::string)> callable) {
    _forceAsyncReadInconsistency = callable;
  }

  /**
   *  Quirk hook: Call this functor after each call to activateAsyncRead().
   * 
   *  Note: When any quirk hook needs to be used to pass the test, the backend is *not* complying to the specifications.
   *        Hence, use quirk hooks *only* when it is impossible to implement the backed to fully comply to the
   *        specifications, because the implemented protocol is broken.
   */
  void quirkHookActivateAsyncRead(std::function<void(void)> hook) {
    std::cout << "WARNING: quirkHookActivateAsyncRead() has been used. The tested backend hence does NOT fully comply "
              << "to the specifications!" << std::endl;
    quirk_activateAsyncRead = hook;
  }

  /**
   *  Add a synchronous read register to be used for the tests. These registers must *not* support
   *  AccessMode::wait_for_new_data. The registers must be readable. Registers may not appear in the list set via
   *  setAsyncReadTestRegisters() as well, but they may appear in the list set through setWriteTestRegisters().
   *  
   *  Any number of synchronous read registers can be added.
   * 
   *  If supportsRaw == true, the specified UserType is assumed to be the raw UserType.
   */
  template<typename UserType>
  void addSyncReadTestRegister(const std::string& name, bool isReadOnly, bool supportsRaw) {
    boost::fusion::at_key<UserType>(syncReadRegisters.table).push_back(name);
    boost::fusion::at_key<UserType>(readRegisters.table).push_back(name);
    boost::fusion::at_key<UserType>(allRegisters.table).push_back(name);
    if(isReadOnly) readOnlyRegisters.push_back(name);
    if(supportsRaw) rawRegisters.push_back(name);
  }

  /**
   *  Add an asynchronous read register to be used for the tests. These registers must support
   *  AccessMode::wait_for_new_data. The registers must be readable. Registers may not appear in the list set via
   *  setSyncReadTestRegisters() as well, but they may appear in the list set through setWriteTestRegisters().
   *
   *  Any number of asynchronous read registers can be added.
   * 
   *  If supportsRaw == true, the specified UserType is assumed to be the raw UserType.
   */
  template<typename UserType>
  void addAsyncReadTestRegister(const std::string& name, bool isReadOnly, bool supportsRaw) {
    boost::fusion::at_key<UserType>(asyncReadRegisters.table).push_back(name);
    boost::fusion::at_key<UserType>(readRegisters.table).push_back(name);
    boost::fusion::at_key<UserType>(allRegisters.table).push_back(name);
    if(isReadOnly) readOnlyRegisters.push_back(name);
    if(supportsRaw) rawRegisters.push_back(name);
  }

  /**
   *  Add a write register to be used for the tests. Any number of write registers can be added.
   * 
   *  If supportsRaw == true, the specified UserType is assumed to be the raw UserType.
   */
  template<typename UserType>
  void addWriteTestRegister(const std::string& name, bool isWriteOnly, bool supportsRaw) {
    boost::fusion::at_key<UserType>(writeRegisters.table).push_back(name);
    boost::fusion::at_key<UserType>(allRegisters.table).push_back(name);
    if(isWriteOnly) writeOnlyRegisters.push_back(name);
    if(supportsRaw) rawRegisters.push_back(name);
  }

 protected:
  void test_B_3_1_2_1();
  void test_B_3_2_1_2();
  void test_B_3_2_2();
  void test_B_6_4();
  void test_B_7_2();
  void test_B_8_2();
  void test_B_8_2_1();
  void test_B_8_3();
  void test_B_8_4();
  void test_B_8_5();
  void test_B_8_5_1();
  void test_B_8_5_2();
  void test_B_8_5_3();
  void test_B_8_6();
  void test_B_9_1();
  void test_B_9_2_1();
  void test_B_9_2_2();
  void test_B_9_3_1();
  void test_B_9_4();
  void test_B_10_1_2();
  void test_B_11_2_1();
  void test_B_11_2_2();
  void test_B_11_6();
  void test_C_5_2_5();
  void test_NOSPEC_valueAfterConstruction();

  /// Utility functions for recurring tasks
  void recoverDevice(ChimeraTK::Device& d);

  /// Actions for enable exception throwing
  EnableDisableActionList forceExceptionsRead, forceExceptionsWrite;

  /// Action to provoke data loss in writes
  std::function<size_t(std::string)> _enableForceDataLossWrite;
  std::function<void(std::string)> _disableForceDataLossWrite;

  /// Action to provoke a value inconsistency in asynchronous read transfers
  std::function<void(std::string)> _forceAsyncReadInconsistency;

  /// Quirk hook: called right after each call to activateAsyncRead()
  std::function<void(void)> quirk_activateAsyncRead{[] {}};

  /// CDD for backend to test
  std::string cdd;

  /// See constructor description.
  GET_REMOTE_VALUE_CALLABLE_T _getRemoteValueCallable;
  std::function<void(std::string)> _setRemoteValueCallable;

  /// Names of registers used for tests
  ctk::FixedUserTypeMap<std::list<std::string>> syncReadRegisters, asyncReadRegisters, readRegisters, writeRegisters,
      allRegisters;

  /// Names of registers with special properties. Must also be in one of the above lists.
  std::list<std::string> readOnlyRegisters, writeOnlyRegisters, rawRegisters;

  /// Special DeviceBacked used for testing the exception reporting to the backend
  struct ExceptionReportingBackend : ctk::DeviceBackendImpl {
    ExceptionReportingBackend(const boost::shared_ptr<ctk::DeviceBackend>& target) : _target(target) {}
    ~ExceptionReportingBackend() override {}

    void setException() override {
      _hasSeenException = true;
      _target->setException();
    }

    /// Check whether setException() has been called since the last call to hasSeenException().
    bool hasSeenException() {
      bool ret = _hasSeenException;
      _hasSeenException = false;
      return ret;
    }

    void open() override{};
    void close() override{};
    bool isFunctional() const override { return false; };
    std::string readDeviceInfo() override { return ""; }

   private:
    boost::shared_ptr<ctk::DeviceBackend> _target;
    bool _hasSeenException{false};
  };
};

/** 
   *  Construct new UnifiedBackendTest object.
   * 
   *  The argument getRemoteValueCallable must be a lambda with the following signature:
   *  [] (std::string registerName, auto dummy) -> std::vector<std::vector<decltype(dummy)>> { ... }
   *  
   *  The dummy argument is merely used as a work-around for the lack of templated lambdas in the available C++ 17
   *  standard. It has no meaningful value, only the type is relevant.
   * 
   *  The getRemoteValueCallable shall execute the following actions:
   *   * Obtain the current value of the remote register,
   *   * convert it into the decltype(dummy), e.g. using NumericToUserType(), and
   *   * store the converted value into a vector of vectors in the same arrangement as it is supposed to appear in the
   *     register accessor and return it.
   *
   *  The setRemoteValueCallable shall execute the following actions:
   *   * Generate a new, distinct value which is possible value (in range and matching precision) for the specified
   *     register,
   *   * load the value into the register of the dummy device used for the test, and
   *   * if the register supports AccessMode::wait_for_new_data, send out (publish) the new value.
   * 
   *  If getRemoteValueCallable() is called after a call to setRemoteValueCallable() with the same register name,
   *  the value returned by getRemoteValueCallable() must be the same as the one which has been set via
   *  setRemoteValueCallable(). This is especially important for registers with AccessMode::wait_for_new_data.
   * 
   *  The type specified through decltype(dummy) in the getRemoteValueCallable() call will be one of the
   *  ChimeraTK-supported UserTypes, which is equal or bigger than the template argument in the corresponding
   *  setSyncReadTestRegisters()/setAsyncReadTestRegisters() call.
   */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T> makeUnifiedBackendTest(
    GET_REMOTE_VALUE_CALLABLE_T getRemoteValueCallable, std::function<void(std::string)> setRemoteValueCallable) {
  return UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>(getRemoteValueCallable, setRemoteValueCallable);
}

/********************************************************************************************************************/
/********************************************************************************************************************/
/* 
 * Implementations below this point.
 * 
 * This is made header-only to avoid the need to link against BOOST unit test libraries for the DeviceAccess 
 * runtime library, or to introduce another library shipped with DeviceAccess just for this class.
 */
/********************************************************************************************************************/
/********************************************************************************************************************/

// Helper template function to compare values appropriately for the type
template<typename UserType>
bool comparHelper(UserType a, UserType b) {
  return a == b;
}

template<>
bool comparHelper<double>(double a, double b) {
  return std::abs(a - b) < (std::max(a, b) / 1e6);
}

template<>
bool comparHelper<float>(float a, float b) {
  return std::abs(a - b) < (std::max(a, b) / 1e6);
}

template<>
bool comparHelper<std::string>(std::string a, std::string b) {
  return a == b;
}

namespace std {
  std::string to_string(const std::string& v) { return v; }
} // namespace std

/********************************************************************************************************************/

// Helper macro to compare the value on an accessor and the expected 2D value
// Note: we use a macro and not a function, so BOOST_ERROR prints us the line number of the actual test!
#define CHECK_EQUALITY(accessor, expectedValue)                                                                        \
  {                                                                                                                    \
    std::string fail;                                                                                                  \
    BOOST_REQUIRE(accessor.getNChannels() == expectedValue.size());                                                    \
    BOOST_REQUIRE(accessor.getNElementsPerChannel() == expectedValue[0].size());                                       \
    for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                    \
      for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue.size(); ++CHECK_EQUALITY_k) {                  \
        if(!comparHelper(                                                                                              \
               accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {     \
          if(fail.size() == 0) {                                                                                       \
            fail = "Accessor content differs from expected value. First difference at index [" +                       \
                std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                           \
                "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                                 \
                " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                            \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(fail != "") {                                                                                                   \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

// Simiar to CHECK_EQUALITY, but runs readLatest() on the accessor in a loop until the expected value has arrived, for
// at most maxMillisecods. If the expected value is not seen within that time, an error is risen.
#define CHECK_EQUALITY_TIMEOUT(accessor, expectedValue, maxMilliseconds)                                               \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(true) {                                                                                                      \
      accessor.readLatest();                                                                                           \
      std::string fail;                                                                                                \
      BOOST_REQUIRE(accessor.getNChannels() == expectedValue.size());                                                  \
      BOOST_REQUIRE(accessor.getNElementsPerChannel() == expectedValue[0].size());                                     \
      for(size_t CHECK_EQUALITY_i = 0; CHECK_EQUALITY_i < expectedValue.size(); ++CHECK_EQUALITY_i) {                  \
        for(size_t CHECK_EQUALITY_k = 0; CHECK_EQUALITY_k < expectedValue.size(); ++CHECK_EQUALITY_k) {                \
          if(!comparHelper(                                                                                            \
                 accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k], expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k])) {   \
            if(fail.size() == 0) {                                                                                     \
              fail = "Accessor content differs from expected value. First difference at index [" +                     \
                  std::to_string(CHECK_EQUALITY_i) + "][" + std::to_string(CHECK_EQUALITY_k) +                         \
                  "]: " + std::to_string(accessor[CHECK_EQUALITY_i][CHECK_EQUALITY_k]) +                               \
                  " != " + std::to_string(expectedValue[CHECK_EQUALITY_i][CHECK_EQUALITY_k]);                          \
            }                                                                                                          \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
      if(fail.size() == 0) break;                                                                                      \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK_MESSAGE(!timeout_reached, fail);                                                                     \
      if(timeout_reached) break;                                                                                       \
      usleep(10000);                                                                                                   \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

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

/********************************************************************************************************************/

template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::runTests(const std::string& backend) {
  cdd = backend;
  std::cout << "=== UnifiedBackendTest for " << cdd << std::endl;

  auto lambda = [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    boost::fusion::at_key<UserType>(syncReadRegisters.table).unique();
    boost::fusion::at_key<UserType>(asyncReadRegisters.table).unique();
    boost::fusion::at_key<UserType>(readRegisters.table).unique();
    boost::fusion::at_key<UserType>(writeRegisters.table).unique();
    boost::fusion::at_key<UserType>(allRegisters.table).unique();
  };
  ctk::for_each(allRegisters.table, lambda);

  // check inputs
  if(forceExceptionsRead.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnRead() not called with a non-empty list." << std::endl;
    std::exit(1);
  }
  if(forceExceptionsWrite.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnWrite() not called with a non-empty list." << std::endl;
    std::exit(1);
  }

  if(!_enableForceDataLossWrite || !_disableForceDataLossWrite) {
    std::cout << "WARNING: UnifiedBackendTest::forceDataLossWrite() not called. Disabling test for data loss in "
              << "write operations." << std::endl;
    _enableForceDataLossWrite = [](std::string) { return std::numeric_limits<size_t>::max(); };
    _disableForceDataLossWrite = [](std::string) {};
  }

  if(!_forceAsyncReadInconsistency) {
    std::cout << "WARNING: UnifiedBackendTest::forceAsyncReadInconsistency() not called. Disabling test for data "
              << "consistency heartbeat in asynchronous read operations." << std::endl;
  }

  size_t nSyncReadRegisters = 0;
  auto lambda1 = [&nSyncReadRegisters](auto pair) { nSyncReadRegisters += pair.second.size(); };
  ctk::for_each(syncReadRegisters.table, lambda1);
  if(nSyncReadRegisters == 0) {
    std::cout << "No synchronous read test registers specified." << std::endl;
    std::exit(1);
  }
  size_t nAsyncReadRegisters = 0;
  auto lambda2 = [&nAsyncReadRegisters](auto pair) { nAsyncReadRegisters += pair.second.size(); };
  ctk::for_each(asyncReadRegisters.table, lambda2);
  size_t nWriteRegisters = 0;
  auto lambda3 = [&nWriteRegisters](auto pair) { nWriteRegisters += pair.second.size(); };
  ctk::for_each(writeRegisters.table, lambda3);

  std::cout << "Using " << nSyncReadRegisters << " synchronous and " << nAsyncReadRegisters << " asynchronous read and "
            << nWriteRegisters << " write test registers." << std::endl;

  if(nAsyncReadRegisters == 0) {
    std::cout
        << "WARNING: No asynchronous read test registers specified. This is acceptable only if the backend does not "
        << "support AccessMode::wait_for_new_data at all." << std::endl;
  }
  if(nWriteRegisters == 0) {
    std::cout << "WARNING: No write test registers specified. This is acceptable only if the backend does not "
              << "support writing at all." << std::endl;
  }

  // run the tests
  test_B_3_1_2_1();
  test_B_3_2_1_2();
  test_B_3_2_2();
  test_B_6_4();
  test_B_7_2();
  test_B_8_2();
  test_B_8_2_1();
  test_B_8_3();
  test_B_8_4();
  test_B_8_5();
  test_B_8_5_1();
  test_B_8_5_2();
  test_B_8_5_3();
  test_B_8_6();
  test_B_9_1();
  test_B_9_2_1();
  test_B_9_2_2();
  test_B_9_3_1();
  test_B_9_4();
  test_B_10_1_2();
  test_B_11_2_1();
  test_B_11_2_2();
  test_B_11_6();
  test_C_5_2_5();
  test_NOSPEC_valueAfterConstruction();
}

/********************************************************************************************************************/

template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::recoverDevice(ChimeraTK::Device& d) {
  for(size_t i = 0;; ++i) {
    try {
      d.open();
      break;
    }
    catch(ChimeraTK::runtime_error&) {
      usleep(10000); // 10ms
      if(i > 6000) {
        BOOST_ERROR("Device did not recover within 60 seconds after forced ChimeraTK::runtime_error.");
      }
    }
    continue;
  }
}

/********************************************************************************************************************/

/**
 *  Test synchronous read.
 *  * \anchor UnifiedTest_TransferElement_B_3_1_2_1 \ref transferElement_B_3_1_2_1 "B.3.1.2.1"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_3_1_2_1() {
  std::cout << "--- test_B_3_1_2_1" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // open the device
      d.open();

      // Read value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);

      // Set an intermediate remote value to be overwritten next
      _setRemoteValueCallable(registerName);
      usleep(100000); // give potential race conditions a chance to pop up more easily...

      // Set another remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v2 = _getRemoteValueCallable(registerName, UserType());

      // Read second value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/// Helper function for write tests.
/// TODO: Function should be overridable for certain registers by the backend-dependent test (-> fixed point)
template<typename UserType>
std::vector<std::vector<UserType>> generateValue(ctk::TwoDRegisterAccessor<UserType>& reg, double& someValue) {
  double increment = 3.1415;
  std::vector<std::vector<UserType>> theValue;
  theValue.resize(reg.getNChannels());
  for(size_t i = 0; i < reg.getNChannels(); ++i) {
    for(size_t k = 0; k < reg.getNElementsPerChannel(); ++k) {
      someValue += increment;
      if(someValue > 126.) someValue -= 126.; // keep in range of every possible value type
      reg[i][k] = ctk::numericToUserType<UserType>(someValue);
      theValue[i].push_back(reg[i][k]);
    }
  }
  return theValue;
}

/********************************************************************************************************************/

/**
 *  Test write() does not destroy application buffer
 *  * \anchor UnifiedTest_TransferElement_B_3_2_1_2 \ref transferElement_B_3_2_1_2 "B.3.2.1.2"
 * 
 * (Exception case is covered by B.6.4)
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_3_2_1_2() {
  std::cout << "--- test_B_3_2_1_2" << std::endl;
  ctk::Device d(cdd);
  double someValue = 42;

  // open the device
  d.open();

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // write some value
      std::vector<std::vector<UserType>> theValue = generateValue(reg, someValue);
      ctk::VersionNumber ver;
      reg.write(ver);

      // check that application data buffer is not changed (non-destructive write, B.3.2.1.2)
      BOOST_CHECK(reg.getNChannels() == theValue.size());
      BOOST_CHECK(reg.getNElementsPerChannel() == theValue[0].size());
      CHECK_EQUALITY(reg, theValue);

      // check the version number
      BOOST_CHECK(reg.getVersionNumber() == ver);
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test destructive write.
 *  * \anchor UnifiedTest_TransferElement_B_3_2_2 \ref transferElement_B_3_2_2 "B.3.2.2"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_3_2_2() {
  std::cout << "--- test_B_3_2_2" << std::endl;
  ctk::Device d(cdd);
  double someValue = 42;

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // open the device
      d.open();

      // write some value
      std::vector<std::vector<UserType>> theValue = generateValue(reg, someValue);
      ctk::VersionNumber ver;
      reg.write(ver);

      // check that application data buffer is not changed (non-destructive write, B.3.2.1.2)
      BOOST_CHECK(reg.getNChannels() == theValue.size());
      BOOST_CHECK(reg.getNElementsPerChannel() == theValue[0].size());
      CHECK_EQUALITY(reg, theValue);

      // check the version number
      BOOST_CHECK(reg.getVersionNumber() == ver);

      // check remote value
      auto v1 = _getRemoteValueCallable(registerName, UserType());
      CHECK_EQUALITY(reg, v1);

      // write another value, this time destructively
      theValue = generateValue(reg, someValue);
      ver = {};
      reg.writeDestructively(ver);

      // check that application data buffer shape is not changed (content may be lost)
      BOOST_CHECK(reg.getNChannels() == theValue.size());
      BOOST_CHECK(reg.getNElementsPerChannel() == theValue[0].size());

      // check the version number
      BOOST_CHECK(reg.getVersionNumber() == ver);

      // check remote value
      v1 = _getRemoteValueCallable(registerName, UserType());
      CHECK_EQUALITY(reg, v1);

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test application buffer unchanged after exception
 *  * \anchor UnifiedTest_TransferElement_B_6_4 \ref transferElement_B_6_4 "B.6.4"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_6_4() {
  std::cout << "--- test_B_6_4" << std::endl;
  ctk::Device d(cdd);

  std::cout << "... synchronous read " << std::endl;
  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, then let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // trigger runtime_error
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);
      }

      // close device again
      d.close();
    }
  });

  std::cout << "... asynchronous read " << std::endl;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error via read()
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // trigger logic error via readNonBlocking()
      BOOST_CHECK_THROW(reg.readNonBlocking(), ctk::logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, then let it throw runtime_error exceptions
      d.open();
      d.activateAsyncRead();
      reg.read(); // initial value

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // alter the application buffer to make sure it is not changed under exception
        reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
        reg.setDataValidity(ctk::DataValidity::ok);
        auto ver = reg.getVersionNumber();

        // trigger runtime_error via read
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ver);

        // recover to get another exception
        testCondition.second();
        this->recoverDevice(d);
        d.activateAsyncRead();
        reg.read(); //initial value
        testCondition.first();

        // alter the application buffer to make sure it is not changed under exception
        reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
        reg.setDataValidity(ctk::DataValidity::ok);
        ver = reg.getVersionNumber();

        // trigger runtime_error via readNonBlocking
        try {
          while(!reg.readNonBlocking()) usleep(10000);
        }
        catch(ctk::runtime_error&) {
        }

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ver);

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);
      }

      // close device again
      d.close();
    }
  });

  std::cout << "... write " << std::endl;
  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;

      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // (quasi-assertion for the test)

      // trigger logic error
      BOOST_CHECK_THROW(reg.write(), ctk::logic_error); // (no check intended, just catch)

      // check that the application buffer has not changed after exception
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, then let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // trigger runtime_error
        BOOST_CHECK_THROW(reg.write(), ctk::runtime_error); // (no check intended, just catch)

        // check that the application buffer has not changed after exception
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);
      }

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test data loss in write
 *  * \anchor UnifiedTest_TransferElement_B_7_2 \ref transferElement_B_7_2 "B.7.2"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_7_2() {
  std::cout << "--- test_B_7_2" << std::endl;
  ctk::Device d(cdd);
  double someValue = 42;

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // enable test condition
      size_t attempts = _enableForceDataLossWrite(registerName);
      if(attempts == std::numeric_limits<size_t>::max()) {
        std::cout << "    (skipped)" << std::endl;
        continue;
      }

      // open the device
      d.open();

      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // write some value the requested number of attempts
      for(size_t i = 0; i < attempts; ++i) {
        std::vector<std::vector<UserType>> theValue = generateValue(reg, someValue);
        ctk::VersionNumber someVersion;
        bool dataLost = reg.write(someVersion);
        if(i < attempts - 1) {
          BOOST_CHECK(dataLost == false);
        }
        else {
          BOOST_CHECK(dataLost == true);
        }
        // User buffer must be intact even when value was lost somewhere
        CHECK_EQUALITY(reg, theValue);
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);
      }

      // disable test condition
      _disableForceDataLossWrite(registerName);

      // check remote value, must be the last written value
      auto v1 = _getRemoteValueCallable(registerName, UserType());
      CHECK_EQUALITY(reg, v1);

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test async read fills _readQueue
 *  * \anchor UnifiedTest_TransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2"
 * 
 *  Note: we do not care about read() vs. readNonBlocking() in this test, since that part of the implementation is in
 *  the base class and hence tested in testTransferElement.
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_2() {
  std::cout << "--- test_B_8_2" << std::endl;
  ctk::Device d(cdd);

  // open the device
  d.open();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Initial value (stays unchecked here)
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // Read the value
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Check application buffer
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Set multiple remote values in a row - they will be queued
      _setRemoteValueCallable(registerName);
      auto v2 = _getRemoteValueCallable(registerName, UserType());
      _setRemoteValueCallable(registerName);
      auto v3 = _getRemoteValueCallable(registerName, UserType());
      _setRemoteValueCallable(registerName);
      auto v4 = _getRemoteValueCallable(registerName, UserType());

      // Read and check second value
      reg.read();
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Read and check third value
      reg.read();
      CHECK_EQUALITY(reg, v3);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Read and check fourth value
      reg.read();
      CHECK_EQUALITY(reg, v4);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // No more data available
      BOOST_CHECK(reg.readNonBlocking() == false);
      CHECK_EQUALITY(reg, v4); // application buffer is unchanged (SPEC???)
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == someVersion);
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test _readQueue overrun
 *  * \anchor UnifiedTest_TransferElement_B_8_2_1 \ref transferElement_B_8_2_1 "B.8.2.1"
 * 
 *  FIXME: This test is not really complete. It tests whether the latest value survives. It does not test though which
 *  other values are surviving. For this the test needs to know the length of the implementation specific data transport
 *  queue, which is currently not known to it (the length of the _readQueue continuation is always 1).
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_2_1() {
  std::cout << "--- test_B_8_2_1" << std::endl;
  ctk::Device d(cdd);

  // open the device
  d.open();

  // Activate async read
  d.activateAsyncRead();
  quirk_activateAsyncRead();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Initial value (stays unchecked here)
      reg.read();
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Provoke queue overflow by filling many values. We are only interested in the last one.
      for(size_t i = 0; i < 10; ++i) {
        _setRemoteValueCallable(registerName);
      }
      auto v5 = _getRemoteValueCallable(registerName, UserType());

      // Read last written value (B.8.2.1)
      BOOST_CHECK(reg.readLatest() == true);
      CHECK_EQUALITY(reg, v5);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test new runtime errors are put to _readQueue in async reads
 *  * \anchor UnifiedTest_TransferElement_B_8_3 \ref transferElement_B_8_3 "B.8.3"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_3() {
  std::cout << "--- test_B_8_3" << std::endl;
  ctk::Device d(cdd);
  d.open();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // read initial value
      reg.read();

      for(auto& testCondition : forceExceptionsRead) {
        // execute preRead without exception state
        reg.getHighLevelImplElement()->preRead(ctk::TransferType::read);

        // enable exceptions on read
        testCondition.first();

        // Check for runtime_error as it is popped of the queue
        BOOST_CHECK_THROW(reg.getHighLevelImplElement()->readTransfer(), ChimeraTK::runtime_error);

        // complete the operation
        reg.getHighLevelImplElement()->postRead(ctk::TransferType::read, false);

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);
      }
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test async read consistency heartbeat
 *  * \anchor UnifiedTest_TransferElement_B_8_4 \ref transferElement_B_8_4 "B.8.4"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_4() {
  std::cout << "--- test_B_8_4" << std::endl;
  if(!_forceAsyncReadInconsistency) {
    std::cout << "    (skipped)" << std::endl;
    return;
  }

  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // open the device
      d.open();

      // Activate async read
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // Obtain accessor
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Read and check initial value
      reg.read();
      CHECK_EQUALITY(reg, v1);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // Provoke inconsistency
      _forceAsyncReadInconsistency(registerName);

      // Wait for the exception which informs about the problem
      BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

      // Recover the device
      this->recoverDevice(d);
      auto v2 = _getRemoteValueCallable(registerName, UserType());

      // Activate async read again
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // Read and check value
      reg.read();
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // close device again
      d.close();
    }
  });
}
/********************************************************************************************************************/

/**
 *  Test no async transfers until activateAsyncRead() for TEs created before open.
 *  * \anchor UnifiedTest_TransferElement_B_8_5 \ref transferElement_B_8_5 "B.8.5"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_5() {
  std::cout << "--- test_B_8_5" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // First step: measure time until initial value arrives, so we know how long to wait to exclude that an initial
      // value arrives wrongly.
      std::chrono::duration<double> timeToInitialValue;
      {
        // start time measurement
        auto t0 = std::chrono::steady_clock::now();

        // open the device
        d.open();

        // obtain accessor
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

        // make a successful read (initial value) to make sure the exception state is gone
        reg.read();

        // finish time measurement
        auto t1 = std::chrono::steady_clock::now();
        timeToInitialValue = t1 - t0;

        // close device again
        d.close();
      }

      // Second step: Check if no data arrives without activateAsyncRead()
      {
        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

        // open the device
        d.open();

        // wait 2 times longer than the time until initial value was received before
        std::this_thread::sleep_for(timeToInitialValue * 2);

        // no value must have arrived
        BOOST_CHECK(reg.readNonBlocking() == false);

        // close device again
        d.close();
      }
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test activateAsynchronousRead
 *  * \anchor UnifiedTest_TransferElement_B_8_5_1 \ref transferElement_B_8_5_1 "B.8.5.1"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_5_1() {
  std::cout << "--- test_B_8_5_1" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // open the device
      d.open();

      // Activate async read
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // Read initial value
      reg.read();

      // Check application buffer
      CHECK_EQUALITY(reg, v1);

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test initial value
 *  * \anchor UnifiedTest_TransferElement_B_8_5_2 \ref transferElement_B_8_5_2 "B.8.5.2",
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_5_2() {
  std::cout << "--- test_B_8_5_2" << std::endl;
  ctk::Device d(cdd);
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // First check: initial value is correctly arriving
      {
        // Set remote value to be read.
        _setRemoteValueCallable(registerName);
        auto v1 = _getRemoteValueCallable(registerName, UserType());

        // open the device
        d.open();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

        // Read initial value
        reg.read();

        // Check application buffer
        CHECK_EQUALITY(reg, v1);

        // close device again
        d.close();
      }

      // Second check: Concurrent updates do not cause inconsistency. Note: This test cannot possilby cover all
      // potential scenarios for race conditions, hence only one simple scenario is tested.
      {
        // Set initial remote value, to make sure it is different from the next remote value set below
        _setRemoteValueCallable(registerName);

        // open the device
        d.open();

        auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

        // Concurrently set another remote value to be read (while presumably the subscription is still being made).
        _setRemoteValueCallable(registerName);
        auto v2 = _getRemoteValueCallable(registerName, UserType());

        // Check that the second value arrives at some point (with timeout)
        CHECK_EQUALITY_TIMEOUT(reg, v2, 30000);

        // close device again
        d.close();
      }
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test no activation required for accessors created after open
 *  * \anchor UnifiedTest_TransferElement_B_8_5_3 \ref transferElement_B_8_5_3 "B.8.5.3"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_5_3() {
  std::cout << "--- test_B_8_5_3" << std::endl;
  ctk::Device d(cdd);

  // obtain deactivated accessors
  std::list<ctk::TransferElementAbstractor> deactivatedAccessors;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << " (deactivated async read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      deactivatedAccessors.push_back(reg);
    }
  });

  // open the device
  d.open();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << " (activated async read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Initial value should arrive
      CHECK_TIMEOUT(reg.readNonBlocking() == true, 30000);
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test interrupt()
 *  * \anchor UnifiedTest_TransferElement_B_8_6 \ref transferElement_B_8_6 "B.8.6" (with sub-points)
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_8_6() {
  std::cout << "--- test_B_8_6" << std::endl;

  ctk::Device d(cdd);
  auto backend = d.getBackend();
  d.open();

  // Activate async read
  d.activateAsyncRead();
  quirk_activateAsyncRead();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      for(size_t i = 0; i < 2; ++i) {
        // execute blocking read in another thread
        boost::thread anotherThread([&] {
          reg.read();
          BOOST_ERROR("boost::thread_interrupt exception expected but not thrown.");
        });

        // interrupt the blocking operation
        reg.getHighLevelImplElement()->interrupt();

        // make sure the other thread can terminate
        anotherThread.join();

        // check accessor is still working
        _setRemoteValueCallable(registerName);
        auto v1 = _getRemoteValueCallable(registerName, UserType());
        reg.read();
        CHECK_EQUALITY(reg, v1);
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    }
  });

  d.close();
}

/********************************************************************************************************************/

/**
 *  Test reporting exceptions to exception backend
 *  * \anchor UnifiedTest_TransferElement_B_9_1 \ref transferElement_B_9_1 "B.9.1"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_9_1() {
  std::cout << "--- test_B_9_1" << std::endl;
  ctk::Device d(cdd);

  // open the device, then let it throw runtime_error exceptions
  d.open();

  std::cout << "... synchronous read" << std::endl;
  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);

        // make a successful read to make sure the exception state is gone. no reporting must take place here.
        BOOST_CHECK_NO_THROW(reg.read());
        BOOST_CHECK(!erb->hasSeenException());
      }
    }
  });

  std::cout << "... asynchronous read" << std::endl;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      reg.read(); // initial value

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);

        // make a successful readNonBlocking (no data) to make sure the exception state is gone. no reporting must take
        // place here.
        BOOST_CHECK_NO_THROW(reg.readNonBlocking());
        BOOST_CHECK(!erb->hasSeenException());
      }
    }
  });

  std::cout << "... write" << std::endl;
  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      for(auto& testCondition : forceExceptionsWrite) {
        // enable exceptions on write
        testCondition.first();

        // Runtime error should be reported via setException()
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.write(), ctk::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // disable exceptions on write
        testCondition.second();

        // recover
        this->recoverDevice(d);

        // make a successful write to make sure the exception state is gone. no reporting must take place here.
        BOOST_CHECK_NO_THROW(reg.write());
        BOOST_CHECK(!erb->hasSeenException());
      }
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test setException() disables asynchronous read transfers
 *  * \anchor UnifiedTest_TransferElement_B_9_2_1 \ref transferElement_B_9_2_1 "B.9.2.1"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_9_2_1() {
  std::cout << "--- test_B_9_2_1" << std::endl;
  ctk::Device d(cdd);
  d.open();

  // obtain accessors and read initial value
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // read initial value
      reg.read();

      // enter exception state
      d.setException();

      // send value, must not be received
      _setRemoteValueCallable(registerName);

      // get the exception off the queue
      BOOST_CHECK_THROW(reg.read(), ctk::runtime_error); // (no test intended, just catch)

      // give potential race conditions a chance...
      usleep(100000);

      // no value expected
      BOOST_CHECK(reg.readNonBlocking() == false);

      // recover device
      this->recoverDevice(d);
    }
  });

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test exactly one runtime_error in the _readQueue per async read accessor
 *  * \anchor UnifiedTest_TransferElement_B_9_2_2 \ref transferElement_B_9_2_2 "B.9.2.2"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_9_2_2() {
  std::cout << "--- test_B_9_2_2" << std::endl;
  ctk::Device d(cdd);
  d.open();

  // obtain accessors and read initial value
  std::list<ctk::TransferElementAbstractor> accessors;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      accessors.push_back(reg);

      // read initial value
      reg.read();
    }
  });

  // enter exception state
  d.setException();

  usleep(10000); // give potential race conditions a chance...

  // each accessor must have exactly one exception in the queue
  for(auto& accessor : accessors) {
    // call the read stages explicitly to make sure the exception is thrown in the right place
    accessor.getHighLevelImplElement()->preRead(ctk::TransferType::read);
    BOOST_CHECK_THROW(accessor.getHighLevelImplElement()->readTransfer(), ctk::runtime_error);
    accessor.getHighLevelImplElement()->postRead(ctk::TransferType::read, false);
    // no more exceptions in the queue are allowed
    BOOST_CHECK(accessor.readNonBlocking() == false);
  }

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test doReadTransferSynchonously throws runtime_error after setException() until recovery
 *  * \anchor UnifiedTest_TransferElement_B_9_3_1 \ref transferElement_B_9_3_1 "B.9.3.1"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_9_3_1() {
  std::cout << "--- test_B_9_3_1" << std::endl;
  ctk::Device d(cdd);
  d.open();

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // put backend into exception state
      d.setException();

      // Check for runtime_error where it is now expected
      BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);

      // recover
      this->recoverDevice(d);

      // make a successful read to make sure the exception state is gone
      BOOST_CHECK_NO_THROW(reg.read());
    }
  });

  d.close();
}

/********************************************************************************************************************/

/**
 *  Test write operations throw after setException()
 *  * \anchor UnifiedTest_TransferElement_B_9_4 \ref transferElement_B_9_4 "B.9.4"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_9_4() {
  std::cout << "--- test_B_9_4" << std::endl;
  ctk::Device d(cdd);
  d.open();

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // put backend into exception state
      d.setException();

      // Check for runtime_error where it is now expected
      BOOST_CHECK_THROW(reg.write(), ctk::runtime_error);

      // recover
      this->recoverDevice(d);

      // make a successful read to make sure the exception state is gone
      BOOST_CHECK_NO_THROW(reg.write());
    }
  });

  d.close();
}

/********************************************************************************************************************/

/**
 *  Test repeated setException() has no effect (in particular, no additional exceptions in async transfers)
 *  * \anchor UnifiedTest_TransferElement_B_10_1_2 \ref transferElement_B_10_1_2 "B.10.1.2"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_10_1_2() {
  std::cout << "--- test_B_10_1_2" << std::endl;
  ctk::Device d(cdd);
  d.open();

  // obtain accessors and read initial value
  std::list<ctk::TransferElementAbstractor> accessors;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      accessors.push_back(reg);

      // read initial value
      reg.read();
    }
  });

  // enter exception state
  d.setException();

  // each accessor has now an exception in the queue -> remove from queue
  for(auto& accessor : accessors) {
    BOOST_CHECK_THROW(accessor.read(), ctk::runtime_error); // (no test intended, just catch)
  }

  // call setException repeatedly
  d.setException();
  d.setException();

  // give potential race conditions a chance...
  usleep(10000);

  // each accessor still must not have any more exceptions in the queue
  for(auto& accessor : accessors) {
    BOOST_CHECK(accessor.readNonBlocking() == false);
  }

  // close device again
  d.close();
}

/********************************************************************************************************************/

/**
 *  Test version number bigger for newer values
 *  * \anchor UnifiedTest_TransferElement_B_11_2_1 \ref transferElement_B_11_2_1 "B.11.2.1",
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_11_2_1() {
  std::cout << "--- test_B_11_2_1" << std::endl;
  ctk::Device d(cdd);

  // open the device
  d.open();

  // synchronous read
  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      for(size_t i = 0; i < 2; ++i) {
        // Set remote value to be read.
        _setRemoteValueCallable(registerName);

        // Read value
        reg.read();

        // Check application buffer
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    }
  });

  // asynchronous read
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << " (async)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      for(size_t i = 0; i < 2; ++i) {
        // Set remote value to be read.
        _setRemoteValueCallable(registerName);

        // Read value
        reg.read();

        // Check application buffer
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();
      }
    }
  });

  // close device
  d.close();
}
/********************************************************************************************************************/

/**
 *  Test consistent data gets same VersionNumber
 *  * \anchor UnifiedTest_TransferElement_B_11_2_2 \ref transferElement_B_11_2_2 "B.11.2.2"
 * 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!  TODO FIXME This test is not complete yet  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_11_2_2() {
  std::cout << "--- test_B_11_2_2" << std::endl;
  ctk::Device d(cdd);

  // open the device
  d.open();

  // CASE 1: consistency with the same register in async read
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);

      // Obtain accessor
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Read the initial value
      reg.read();

      // Read through second accessor
      auto reg2 = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      reg2.read();

      // Version must be identical to the version of the first accessor
      BOOST_CHECK(reg2.getVersionNumber() == reg.getVersionNumber());

      // Change value, must be seen by both accessors, again same version expected
      _setRemoteValueCallable(registerName);

      reg.read();
      reg2.read();
      BOOST_CHECK(reg.getVersionNumber() == reg2.getVersionNumber());

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test the value after construction for the version number in the application buffer
 *  * \anchor UnifiedTest_TransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_B_11_6() {
  std::cout << "--- B.11.6" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(allRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction" for VersionNumber
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test logic_error on operation while backend closed
 *  * \anchor UnifiedTest_TransferElement_C_5_2_5 \ref transferElement_C_5_2_5 "C.5.2.5"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_C_5_2_5() {
  std::cout << "--- test_C_5_2_5" << std::endl;
  ctk::Device d(cdd);

  std::cout << "... synchronous read" << std::endl;
  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);
    }
  });

  std::cout << "... asynchronous read" << std::endl;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);
      BOOST_CHECK_THROW(reg.readNonBlocking(), ctk::logic_error);
    }
  });

  std::cout << "... write" << std::endl;
  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      BOOST_CHECK_THROW(reg.write(), ctk::logic_error);
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test the content of the application data buffer after construction.
 *  * MISSING SPEC
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_NOSPEC_valueAfterConstruction() {
  std::cout << "--- test_NOSPEC_valueAfterConstruction" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(allRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction" for value buffer
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test small getter functions of accessors
 * 
 *  This tests if small getter functions of accessors work as expected, and it verifies that the implementation complies
 *  for to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_15_2 \ref transferElement_B_15_2 "B.15.2"
 */

/********************************************************************************************************************/

/**
 *  Test catalogue-accessor consistency
 * 
 *  This tests if the catalogue information matches the actual accessor.
 */

/********************************************************************************************************************/

/**
 *  Test logic_errors
 * 
 *  This tests if logic_errors are thrown in the right place, and it verifies that the implementation complies
 *  for to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_C_5_2 \ref transferElement_C_5_2 "C.5.2"
 */

/********************************************************************************************************************/
