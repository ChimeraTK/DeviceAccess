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
   *  Set the names of synchronous read registers to be used for the tests. These registers must *not* support
   *  AccessMode::wait_for_new_data. The registers must be readable. Registers may not appear in the list set via
   *  setAsyncReadTestRegisters() as well, but they may appear in the list set through setWriteTestRegisters().
   */
  template<typename UserType>
  void setSyncReadTestRegisters(std::list<std::string> names) {
    names.sort();
    boost::fusion::at_key<UserType>(syncReadRegisters.table).merge(std::list<std::string>(names));
    boost::fusion::at_key<UserType>(readRegisters.table).merge(std::list<std::string>(names));
    boost::fusion::at_key<UserType>(allRegisters.table).merge(std::list<std::string>(names));
  }

  /**
   *  Set the names of asynchronous read registers to be used for the tests. These registers must support
   *  AccessMode::wait_for_new_data. The registers must be readable. Registers may not appear in the list set via
   *  setSyncReadTestRegisters() as well, but they may appear in the list set through setWriteTestRegisters().
   */
  template<typename UserType>
  void setAsyncReadTestRegisters(std::list<std::string> names) {
    names.sort();
    boost::fusion::at_key<UserType>(asyncReadRegisters.table).merge(std::list<std::string>(names));
    boost::fusion::at_key<UserType>(readRegisters.table).merge(std::list<std::string>(names));
    boost::fusion::at_key<UserType>(allRegisters.table).merge(std::list<std::string>(names));
  }

  /**
   *  Set the names of asynchronous read registers to be used for the tests. These registers must support
   *  AccessMode::wait_for_new_data. The registers must be readable.
   */
  template<typename UserType>
  void setWriteTestRegisters(std::list<std::string> names) {
    names.sort();
    boost::fusion::at_key<UserType>(writeRegisters.table).merge(std::list<std::string>(names));
    boost::fusion::at_key<UserType>(allRegisters.table).merge(std::list<std::string>(names));
  }

  [[deprecated]] void integerRegister(const std::list<std::string>& names) { setSyncReadTestRegisters<int>(names); }

 protected:
  void test_valueAfterConstruction();
  void test_syncRead();
  void test_asyncRead();
  void test_write();
  void test_exceptionHandlingSyncRead();
  void test_exceptionHandlingAsyncRead();
  void test_exceptionHandlingWrite();
  void test_writeDataLoss();
  void test_asyncReadConsistencyHeartbeat();
  void test_setException();
  void test_interrupt();

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

  /// Name of integer register used for tests
  ctk::FixedUserTypeMap<std::list<std::string>> syncReadRegisters, asyncReadRegisters, readRegisters, writeRegisters,
      allRegisters;

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
    std::string fail = "";                                                                                             \
    BOOST_REQUIRE(accessor.getNChannels() == expectedValue.size());                                                    \
    BOOST_REQUIRE(accessor.getNElementsPerChannel() == expectedValue[0].size());                                       \
    for(size_t i = 0; i < expectedValue.size(); ++i) {                                                                 \
      for(size_t k = 0; k < expectedValue.size(); ++k) {                                                               \
        if(!comparHelper(accessor[i][k], expectedValue[i][k])) {                                                       \
          if(fail.size() == 0) {                                                                                       \
            fail = "Accessor content differs from expected value. First difference at index [" + std::to_string(i) +   \
                "][" + std::to_string(k) + "]: " + std::to_string(accessor[i][k]) +                                    \
                " != " + std::to_string(expectedValue[i][k]);                                                          \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if(fail != "") {                                                                                                   \
      BOOST_ERROR(fail);                                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
  (void)(0)

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
  test_valueAfterConstruction();
  test_syncRead();
  test_asyncRead();
  test_write();
  test_exceptionHandlingSyncRead();
  test_exceptionHandlingAsyncRead();
  test_exceptionHandlingWrite();
  test_writeDataLoss();
  test_asyncReadConsistencyHeartbeat();
  test_setException();
  test_interrupt();
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
 *  Test the content of the application buffer after construction.
 * 
 *  This tests verifies that the implementation complies to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6", and
 *  * MISSING SPEC for value after construction of value buffer
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_valueAfterConstruction() {
  std::cout << "--- valueAfterConstruction" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(allRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction" for value buffer
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);

      // check "value after construction" for VersionNumber
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test synchronous read.
 * 
 *  This tests that the correct values are read in synchronous read operations, and it verifies that the
 *  implementation complies for synchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_1_2_1 \ref transferElement_B_3_1_2_1 "B.3.1.2.1",
 *  * \anchor UnifiedTest_TransferElement_B_11_2_1_syncRead \ref transferElement_B_11_2_1 "B.11.2.1",
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_syncRead() {
  std::cout << "--- syncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

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
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

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
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test asynchronous read.
 * 
 *  This tests that the correct values are read in asynchronous read operations, and it verifies that the
 *  implementation complies for asynchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_1_3 \ref transferElement_B_3_1_3 "B.3.1.3" (with sub-points),
 *  * \anchor UnifiedTest_TransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2",
 *  * \anchor UnifiedTest_TransferElement_B_8_2_1 \ref transferElement_B_8_2_1 "B.8.2.1",
 *  * \anchor UnifiedTest_TransferElement_B_8_5 \ref transferElement_B_8_5_1 "B.8.5" (without sub-points),
 *  * \anchor UnifiedTest_TransferElement_B_8_5_1 \ref transferElement_B_8_5_1 "B.8.5.1",
 *  * \anchor UnifiedTest_TransferElement_B_8_5_2 \ref transferElement_B_8_5_2 "B.8.5.2",
 *  * \anchor UnifiedTest_TransferElement_B_8_5_3 \ref transferElement_B_8_5_3 "B.8.5.3",
 *  * \anchor UnifiedTest_TransferElement_B_11_2_2_sameRegister \ref transferElement_B_11_2_2 "B.11.2.2" (same register
 *    only),
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_asyncRead() {
  std::cout << "--- asyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // Set remote value to be read.
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // open the device
      d.open();

      // Check no value arrives before activateAsyncRead(). This is more precisely tested again in
      // exceptionHandlingAsyncRead() below. (B.8.5)
      usleep(100000); // give potential race conditions a chance to pop up more easily...
      BOOST_CHECK(reg.readNonBlocking() == false);

      // Activate async read (B.8.5.1)
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // Read initial value (B.8.5.2)
      reg.read();

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

      // Read and check third value, this time with non blocking read (B.3.1.3.2 with data available)
      BOOST_CHECK(reg.readNonBlocking() == true);
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

      // No more data available, tested with non blocking read (B.3.1.3.2 without data available)
      BOOST_CHECK(reg.readNonBlocking() == false);
      CHECK_EQUALITY(reg, v4); // application buffer is unchanged
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == someVersion);

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

      // Obtain a second accessor, which should receive data right away since the device is open already (B.8.5.3)
      auto reg2 = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      _setRemoteValueCallable(registerName);
      auto v6 = _getRemoteValueCallable(registerName, UserType());
      reg2.read();
      CHECK_EQUALITY(reg2, v6);
      BOOST_CHECK(reg2.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg2.getVersionNumber() != ctk::VersionNumber(nullptr));

      // The value must be seen by the first accessor as well
      reg.read();
      CHECK_EQUALITY(reg, v6);
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);

      // Since the data is the same, it is consistent and hence must have the same VersionNumber (B.11.2.2)
      BOOST_CHECK(reg.getVersionNumber() == reg2.getVersionNumber());

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

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
 *  Test write.
 * 
 *  This tests that the correct values are written in write operations (non-destructive and destructive), and it
 *  verifies that the implementation complies for write operations to the following TransferElement
 *  specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_2_1_2 \ref transferElement_B_3_2_1_2 "B.3.2.1.2",
 *  * \anchor UnifiedTest_TransferElement_B_3_2_2 \ref transferElement_B_3_2_2 "B.3.2.2",
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_write() {
  std::cout << "--- write" << std::endl;
  ctk::Device d(cdd);
  double someValue = 42;

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
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
 *  Test exception handling for synchronous read operations.
 * 
 *  This tests that the implementation throws exceptions when it is supposed to do so, and it verifies that the
 *  implementation complies for synchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_6_4_syncRead \ref transferElement_B_6_4 "B.6.4", and
 *  * \anchor UnifiedTest_TransferElement_B_9_3 \ref transferElement_B_9_3 "B.9.3", and
 *  * \anchor UnifiedTest_TransferElement_C_5_2_5_syncRead \ref transferElement_C_5_2_5 "C.5.2.5".
 *  * \anchor UnifiedTest_TransferElement_B_9_1_syncRead \ref transferElement_B_9_1 "B.9.1"
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_exceptionHandlingSyncRead() {
  std::cout << "--- exceptionHandlingSyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // cannot be changed

      // attempt read while device closed, logic_error is expected. (C.5.2.5)
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);
      BOOST_CHECK(!erb->hasSeenException());

      // check that the application buffer has not changed after exception (B.6.4)
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, then let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // alter application buffer
        reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
        reg.setDataValidity(ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

        // Check for runtime_error where it is now expected
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);

        // disable exceptions on read
        testCondition.second();

        // check that exception is still thrown (device not yet recovered) (B.9.3)
        usleep(100000); // give potential race conditions a chance to pop up more easily...
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // recover
        this->recoverDevice(d);

        // make a successful read to make sure the exception state is gone
        BOOST_CHECK_NO_THROW(reg.read());

        // check that the application buffer is now changed (without implying assumptions about the value)
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();

        // re-enable exceptions on read
        testCondition.first();

        // alter application buffer
        reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
        reg.setDataValidity(ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

        // repeat the above test, a runtime_error should be expected again
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);

        // disable exceptions on read
        testCondition.second();
      }

      // close device again
      d.close();

      BOOST_CHECK(!erb->hasSeenException());
    }
    
  });
}

/********************************************************************************************************************/

/**
 *  Test exception handling for asynchronous read operations.
 * 
 *  This tests that the implementation throws exceptions when it is supposed to do so, and it verifies that the
 *  implementation complies for asynchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_6_4_asyncRead \ref transferElement_B_6_4 "B.6.4",
 *  * \anchor UnifiedTest_TransferElement_C_5_2_5_asyncRead \ref transferElement_C_5_2_5 "C.5.2.5",
 *  * \anchor UnifiedTest_TransferElement_B_8_3 \ref transferElement_B_8_3 "B.8.3" (only first sentence),
 *  * \anchor UnifiedTest_TransferElement_B_9_2_1_single \ref transferElement_B_9_2_1 "B.9.2.1" (only single accessor), and
 *  * \anchor UnifiedTest_TransferElement_B_9_2_2_single \ref transferElement_B_9_2_2 "B.9.2.2" (only single accessor).
 *  * \anchor UnifiedTest_TransferElement_B_9_1_asyncRead \ref transferElement_B_9_1 "B.9.1"
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_exceptionHandlingAsyncRead() {
  std::cout << "--- exceptionHandlingAsyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;

      // Obtain accessor for the test

      // Test obtaining an accessor while device is closed and never using it. Bad implementations could fail in the
      // desctructor in such case.
      {
        auto unused = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
        (void)unused;
      }

      // obtain accessor for the test
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      // Set remove value (so we know it)
      _setRemoteValueCallable(registerName);
      auto v1 = _getRemoteValueCallable(registerName, UserType());

      // Generate a number which is for sure different that the current value and fits into every data type
      int someNumber = 42;
      if(ctk::userTypeToNumeric<int>(v1[0][0]) == 42) someNumber = 43;

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // cannot be changed

      // attempt read while device closed, logic_error is expected. (C.5.2.5)
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);
      BOOST_CHECK(!erb->hasSeenException());

      // check that the application buffer has not changed after exception (B.6.4)
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // Change remove value, will be seen as initial value after recovery
      _setRemoteValueCallable(registerName);
      auto v2 = _getRemoteValueCallable(registerName, UserType());

      // open the device, let it throw runtime_error exceptions
      d.open();
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // read initial value
      reg.read();

      // check that the application buffer is now changed
      CHECK_EQUALITY(reg, v2);
      BOOST_CHECK(reg.getVersionNumber() > someVersion);
      someVersion = reg.getVersionNumber();

      // make sure no additional value arrives
      usleep(10000);
      BOOST_CHECK(reg.readNonBlocking() == false);
      BOOST_CHECK(reg.getVersionNumber() == someVersion);

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // alter application buffer
        reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
        reg.setDataValidity(ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

        // Check for runtime_error where it is now expected (B.9.2.1/B.9.2.2)
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
        BOOST_CHECK(erb->hasSeenException());
        usleep(10000);
        BOOST_CHECK(reg.readNonBlocking() == false);

        // disable exceptions on read
        testCondition.second();

        // No data received before device is recovered and async read transfers are re-activated (B.9.2.1)
        usleep(100000); // give potential race conditions a chance to pop up more easily...
        BOOST_CHECK(reg.readNonBlocking() == false);

        // recover
        this->recoverDevice(d);

        // measure time until first data arrives, required for testing B.9.2.1 later
        auto t0 = std::chrono::steady_clock::now();

        // change value (will be initial value)
        _setRemoteValueCallable(registerName);
        auto v3 = _getRemoteValueCallable(registerName, UserType());

        // reactivate async read transfers
        d.activateAsyncRead();
        quirk_activateAsyncRead();

        // make a successful read (initial value) to make sure the exception state is gone
        reg.read();
        auto t1 = std::chrono::steady_clock::now();
        BOOST_CHECK(reg.readNonBlocking() == false);

        // check that the application buffer is now changed
        CHECK_EQUALITY(reg, v3);
        BOOST_CHECK(reg.getVersionNumber() > someVersion);
        someVersion = reg.getVersionNumber();

        // re-enable exceptions on read
        testCondition.first();

        // alter application buffer
        reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
        reg.setDataValidity(ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

        // repeat the above test, a runtime_error should be expected again
        BOOST_CHECK(!erb->hasSeenException());
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
        BOOST_CHECK(erb->hasSeenException());

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);

        // disable exceptions on read
        testCondition.second();

        // recover
        this->recoverDevice(d);

        // wait twice as long as it took above until first data arrived after recovery+reactivation
        // note: if this sleep is too short, the following BOOST_CHECK is insensitive against bugs.
        std::this_thread::sleep_for((t1 - t0) * 2);

        // No data received before device async read transfers are re-activated (B.9.2.1)
        BOOST_CHECK(reg.readNonBlocking() == false);

        // reactivate async read transfers
        d.activateAsyncRead();
        quirk_activateAsyncRead();

        // await initial value (no checks, just to be in same state again as at the beginning of the loop)
        reg.read();
      }

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test exception handling for write operations.
 * 
 *  This tests that the implementation throws exceptions when it is supposed to do so, and it verifies that the
 *  implementation complies for write operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_6_4_write \ref transferElement_B_6_4 "B.6.4",
 *  * \anchor UnifiedTest_TransferElement_B_9_4_single \ref transferElement_B_9_4 "B.9.4" (only single accessor),
 *  * \anchor UnifiedTest_TransferElement_C_5_2_5_write \ref transferElement_C_5_2_5 "C.5.2.5",
 *  * \anchor UnifiedTest_TransferElement_B_9_1_write\ref transferElement_B_9_1 "B.9.1"
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_exceptionHandlingWrite() {
  std::cout << "--- exceptionHandlingWrite" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // set exception reporting backend
      auto erb = boost::make_shared<ExceptionReportingBackend>(d.getBackend());
      reg.getHighLevelImplElement()->setExceptionBackend(erb);

      // repeat the following check for a list of actions
      std::list<std::pair<std::string, std::function<void(void)>>> actionList;
      actionList.push_back({"write()", [&] { reg.write(); }});
      actionList.push_back({"writeDestructively()", [&] { reg.writeDestructively(); }});

      // attempt write while device closed, logic_error is expected (C.5.2.5).
      for(auto action : actionList) {
        std::cout << "    " << action.first << std::endl;
        auto theAction = action.second;

        // alter the application buffer to make sure it is not changed under exception
        reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
        reg.setDataValidity(ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

        // check for runtime_error where it is now expected
        BOOST_CHECK_THROW(theAction(), ChimeraTK::logic_error);
        BOOST_CHECK(!erb->hasSeenException());

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);
      }

      // open the device, let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsWrite) {
        for(auto action : actionList) {
          std::cout << "    " << action.first << std::endl;
          auto theAction = action.second;

          // enable exceptions on write
          testCondition.first();

          // alter the application buffer to make sure it is not changed under exception
          reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
          reg.setDataValidity(ctk::DataValidity::ok);
          BOOST_CHECK(reg.getVersionNumber() == someVersion); // cannot be changed

          // check for runtime_error where it is now expected
          BOOST_CHECK(!erb->hasSeenException());
          BOOST_CHECK_THROW(theAction(), ChimeraTK::runtime_error);
          BOOST_CHECK(erb->hasSeenException());

          // check that the application buffer has not changed after exception (B.6.4)
          BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
          BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
          BOOST_CHECK(reg.getVersionNumber() == someVersion);

          // disable exceptions on write
          testCondition.second();

          // check that exception is still thrown (device not yet recovered) (B.9.4)
          usleep(100000); // give potential race conditions a chance to pop up more easily...
          BOOST_CHECK(!erb->hasSeenException());
          BOOST_CHECK_THROW(theAction(), ChimeraTK::runtime_error);
          BOOST_CHECK(erb->hasSeenException());

          // recover the device
          this->recoverDevice(d);

          // alter the application buffer in preparation of a write
          reg[0][0] = ctk::numericToUserType<UserType>(++someNumber);
          reg.setDataValidity(ctk::DataValidity::faulty);

          // execute a successful write
          BOOST_CHECK_NO_THROW(theAction());

          // new version number must have been generated (B.11.3 - guaranteed by the base class)
          BOOST_CHECK(reg.getVersionNumber() > someVersion);
          someVersion = reg.getVersionNumber();
        }
      }

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

/**
 *  Test data loss in write.
 * 
 *  This tests if data loss in writes is correctly reported, and it verifies that the implementation complies to the
 *  following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_7_2 \ref transferElement_B_7_2 "B.7.2"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_writeDataLoss() {
  std::cout << "--- writeDataLoss" << std::endl;
  ctk::Device d(cdd);
  double someValue = 42;

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
    for(auto& registerName : pair.second) {
      ctk::VersionNumber someVersion{nullptr};
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
 *  Test async read consistency heartbeat
 * 
 *  This tests if data consistency is checked and corrected periodically for async reads (if necessary), and it verifies
 *  that the implementation complies to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_8_4 \ref transferElement_B_8_4 "B.8.4"
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_asyncReadConsistencyHeartbeat() {
  std::cout << "--- asyncReadConsistencyHeartbeat" << std::endl;
  if(!_forceAsyncReadInconsistency) {
    std::cout << "    (skipped)" << std::endl;
    return;
  }

  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
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
 *  Test setException()
 * 
 *  This tests if exceptions reported to the backend via setException() are treated correctly, and it verifies that the
 *  implementation complies to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_9 \ref transferElement_B_9 "B.9" (without sub-points)
 *  * \anchor UnifiedTest_TransferElement_B_10_1 \ref transferElement_B_10_1 "B.10.1" (with all sub-points)
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_setException() {
  std::cout << "--- setException" << std::endl;

  ctk::Device d(cdd);
  auto backend = d.getBackend();
  d.open();

  // obtain all read accessors, check that they are not throwing initially
  std::list<ctk::TransferElementAbstractor> readAccessors;
  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << " (sync read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      readAccessors.push_back(reg);
      BOOST_CHECK_NO_THROW(reg.read());
    }
  });

  // obtain all async read accessors, check that they are not throwing initially
  std::list<ctk::TransferElementAbstractor> asyncReadAccessors;
  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << " (async read)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});
      asyncReadAccessors.push_back(reg);
      BOOST_CHECK_NO_THROW(reg.readNonBlocking());
    }
  });

  // obtain all write accessors, check that they are not throwing initially
  std::list<ctk::TransferElementAbstractor> writeAccessors;
  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "    registerName = " << registerName << " (write)" << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);
      writeAccessors.push_back(reg);
      BOOST_CHECK_NO_THROW(reg.write());
      BOOST_CHECK_NO_THROW(reg.writeDestructively());
    }
  });

  // set backend into exception state
  backend->setException();

  // check that all read accessors are now throwing
  for(auto& reg : readAccessors) {
    BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);
  }

  // check that all async read accessors are now throwing
  for(auto& reg : asyncReadAccessors) {
    BOOST_CHECK_THROW(reg.readNonBlocking(), ctk::runtime_error);
  }

  // check that all write accessors are now throwing
  for(auto& reg : writeAccessors) {
    BOOST_CHECK_THROW(reg.write(), ctk::runtime_error);
    BOOST_CHECK_THROW(reg.writeDestructively(), ctk::runtime_error);
  }

  // recover the device
  recoverDevice(d);

  // check that all read accessors are no longer throwing
  for(auto& reg : readAccessors) {
    BOOST_CHECK_NO_THROW(reg.read());
  }

  // check that all async read accessors are no longer throwing
  for(auto& reg : asyncReadAccessors) {
    BOOST_CHECK_NO_THROW(reg.readNonBlocking());
  }

  // check that all write accessors are no longer throwing
  for(auto& reg : writeAccessors) {
    BOOST_CHECK_NO_THROW(reg.write());
    BOOST_CHECK_NO_THROW(reg.writeDestructively());
  }

  d.close();
}

/********************************************************************************************************************/

/**
 *  Test interrupt()
 * 
 *  This tests that implementations supporting wait_for_new_data override correctly interrupt() as described in the
 *  description of the function, and it verifies that the implementation complies for to the following TransferElement
 *  specifications:
 *  * \anchor UnifiedTest_TransferElement_B_8_6 \ref transferElement_B_8_6 "B.8.6" (with sub-points)
 */
template<typename GET_REMOTE_VALUE_CALLABLE_T>
void UnifiedBackendTest<GET_REMOTE_VALUE_CALLABLE_T>::test_interrupt() {
  std::cout << "--- interrupt" << std::endl;

  ctk::Device d(cdd);
  auto backend = d.getBackend();
  d.open();

  // Activate async read
  d.activateAsyncRead();
  quirk_activateAsyncRead();

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    size_t valueId = 0;
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
