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
 *  Note: This is work in progress. Tests are by far not yet complete. Interface changes of the test class are also
 *  likely.
 */
class UnifiedBackendTest {
 public:
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
   *  Set functor to set values in remote registers. The test will call this functor e.g. before reading the register
   *  through the backend, then verify the read value through checkRemoteValue().
   * 
   *  The first argument of the functor is the register name. The second argument is an index used to identify the
   *  value. The functor shall generate a quasi-unique value which is a member of the set of possible values the
   *  register can have, and assign the remote register to that value. If the functor is called again with the same
   *  index, the same value is supposed to be set. If it is called with a different index, a different value shall be
   *  used.
   */
  void setRemoteValue(std::function<void(const std::string&, size_t)> functor) { _setRemoteValue = functor; }

  /**
   *  Set functor to check values in remote registers. The test will call this functor e.g. after reading the register
   *  through the backend, to compare the result with the value previously set via setRemoteValue().
   * 
   *  The first argument (const std::string&) of the functor is the register name. The second argument (size_t) is an
   *  index used to identify the value. The same value has been passed previously to setRemoteValue() (note that this
   *  was not necessarily the last call to setRemoteValue(), hence the index is required to find the expected value).
   *  The third argument (auto -> will be std::vector<std::vector<UserType>>&) is the value which has been read
   *  through the accessor.
   * 
   *  The functor needs to convert the expected value into the given type of the 3rd argument. Then the comparison shall
   *  be performed with BOOST_CHECK_EQUAL for non-float, resp. BOOST_CHECK_CLOSE for float types.
   * 
   *  For the conversion, ChimeraTK::numericToUserType() should be used for convenience. The conversion should be done
   *  in the direction as described here (from original type into given UserType), so the information loss which might
   *  take place during conversion is the same as in the read operation.
   * 
   *  Note: If the original data type is a string there will never be any conversion expected, the 3rd argument will
   *  always be of the type std::string.
   */
  template<typename LAMBDA>
  void checkRemoteValue(LAMBDA lambda) {
    // store function pointer for each type
    ChimeraTK::for_each(_checkRemoteValue.table, [&](auto pair) {
      typedef typename decltype(pair)::first_type UserType;
      pair.second = _checkRemoteValue_functor<UserType>(lambda);
    });
  }

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
  void valueAfterConstruction();
  void exceptionHandlingSyncRead();
  void exceptionHandlingAsyncRead();
  void exceptionHandlingWrite();

  /// Utility functions for recurring tasks
  void recoverDevice(ChimeraTK::Device& d);

  /// Actions for enable exception throwing
  EnableDisableActionList forceExceptionsRead, forceExceptionsWrite;

  /// Functor to set and remote values
  std::function<void(const std::string&, size_t)> _setRemoteValue;

  /// "Typedef" of a templated functor type for use in _checkRemoteValue
  template<typename UserType>
  class _checkRemoteValue_functor
  : public std::function<void(const std::string&, size_t, std::vector<std::vector<UserType>>&)> {
    using std::function<void(const std::string&, size_t, std::vector<std::vector<UserType>>&)>::function;
  };

  /// Functors to check remote values
  ctk::TemplateUserTypeMap<_checkRemoteValue_functor> _checkRemoteValue;

  /// Quirk hook: called right after each call to activateAsyncRead()
  std::function<void(void)> quirk_activateAsyncRead{[] {}};

  /// CDD for backend to test
  std::string cdd;

  /// Name of integer register used for tests
  ctk::FixedUserTypeMap<std::list<std::string>> syncReadRegisters, asyncReadRegisters, readRegisters, writeRegisters,
      allRegisters;
};

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

void UnifiedBackendTest::runTests(const std::string& backend) {
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
  valueAfterConstruction();
  exceptionHandlingSyncRead();
  exceptionHandlingAsyncRead();
  exceptionHandlingWrite();
}

/********************************************************************************************************************/

void UnifiedBackendTest::recoverDevice(ChimeraTK::Device& d) {
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
void UnifiedBackendTest::valueAfterConstruction() {
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
 *  Test exception handling for synchronous read operations.
 * 
 *  This tests that the implementation throws exceptions when it is supposed to do so, and it verifies that the
 *  implementation complies for synchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_6_4_syncRead \ref transferElement_B_6_4 "B.6.4", and
 *  * \anchor UnifiedTest_TransferElement_B_9_3 \ref transferElement_B_9_3 "B.9.3", and
 *  * \anchor UnifiedTest_TransferElement_C_5_2_5_syncRead \ref transferElement_C_5_2_5 "C.5.2.5".
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
void UnifiedBackendTest::exceptionHandlingSyncRead() {
  std::cout << "--- exceptionHandlingSyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // cannot be changed

      // attempt read while device closed, logic_error is expected. (C.5.2.5)
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);

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
        BOOST_CHECK_THROW(reg.read(), ctk::runtime_error);

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);

        // disable exceptions on read
        testCondition.second();

        // check that exception is still thrown (device not yet recovered) (B.9.3)
        usleep(100000); // give potential race conditions a chance to pop up more easily...
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

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
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

        // check that the application buffer has not changed after exception (B.6.4)
        BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
        BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::faulty);
        BOOST_CHECK(reg.getVersionNumber() == someVersion);

        // disable exceptions on read
        testCondition.second();
      }

      // close device again
      d.close();
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
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
void UnifiedBackendTest::exceptionHandlingAsyncRead() {
  std::cout << "--- exceptionHandlingAsyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // alter the application buffer to make sure it is not changed under exception
      reg[0][0] = ctk::numericToUserType<UserType>(someNumber);
      reg.setDataValidity(ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr)); // cannot be changed

      // attempt read while device closed, logic_error is expected. (C.5.2.5)
      BOOST_CHECK_THROW(reg.read(), ctk::logic_error);

      // check that the application buffer has not changed after exception (B.6.4)
      BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
      BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, let it throw runtime_error exceptions
      d.open();
      d.activateAsyncRead();
      quirk_activateAsyncRead();

      // read initial value
      reg.read();

      // check that the application buffer is now changed (without implying assumptions about the value)
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
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);
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

        // reactivate async read transfers
        d.activateAsyncRead();
        quirk_activateAsyncRead();

        // make a successful read (initial value) to make sure the exception state is gone
        reg.read();
        auto t1 = std::chrono::steady_clock::now();
        BOOST_CHECK(reg.readNonBlocking() == false);

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
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

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

        // await initial value (to be in same state again as at the beginning of the loop)
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
 * 
 *  Note: it is probbaly better to move the logic_error related tests into a separate function and test here for
 *  runtime_error handling only.
 */
void UnifiedBackendTest::exceptionHandlingWrite() {
  std::cout << "--- exceptionHandlingWrite" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      int someNumber = 42;
      ctk::VersionNumber someVersion{nullptr};

      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

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
          BOOST_CHECK_THROW(theAction(), ChimeraTK::runtime_error);

          // check that the application buffer has not changed after exception (B.6.4)
          BOOST_CHECK(reg[0][0] == ctk::numericToUserType<UserType>(someNumber));
          BOOST_CHECK(reg.dataValidity() == ctk::DataValidity::ok);
          BOOST_CHECK(reg.getVersionNumber() == someVersion);

          // disable exceptions on write
          testCondition.second();

          // check that exception is still thrown (device not yet recovered) (B.9.4)
          usleep(100000); // give potential race conditions a chance to pop up more easily...
          BOOST_CHECK_THROW(theAction(), ChimeraTK::runtime_error);

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
 *  Test synchronous read.
 * 
 *  This tests that the correct values are read in synchronous read operations, and it verifies that the
 *  implementation complies for synchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_1_2_1 \ref transferElement_B_3_1_2_1 "B.3.1.2.1",
 */

/********************************************************************************************************************/

/**
 *  Test asynchronous read.
 * 
 *  This tests that the correct values are read in asynchronous read operations, and it verifies that the
 *  implementation complies for asynchronous read operations to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_1_3 \ref transferElement_B_3_1_3 "B.3.1.3",
 *  * \anchor UnifiedTest_TransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2",
 *  * \anchor UnifiedTest_TransferElement_B_8_2_1 \ref transferElement_B_8_2_1 "B.8.2.1",
 *  * \anchor UnifiedTest_TransferElement_B_8_5_1 \ref transferElement_B_8_5_1 "B.8.5.1",
 *  * \anchor UnifiedTest_TransferElement_B_8_5_2 \ref transferElement_B_8_5_2 "B.8.5.2",
 *  * \anchor UnifiedTest_TransferElement_B_8_5_3 \ref transferElement_B_8_5_3 "B.8.5.3",
 */

/********************************************************************************************************************/

/**
 *  Test write.
 * 
 *  This tests that the correct values are written in write operations (non-destructive and destructive), and it
 *  verifies that the implementation complies for write operations to the following TransferElement
 *  specifications:
 *  * \anchor UnifiedTest_TransferElement_B_3_2_1_2 \ref transferElement_B_3_2_1_2 "B.3.2.1.2",
 *  * \anchor UnifiedTest_TransferElement_B_3_2_2 \ref transferElement_B_3_2_1_2 "B.3.2.2",
 */

/********************************************************************************************************************/

/**
 *  Test data loss in write.
 * 
 *  This tests if data loss in writes is correctly reported, and it verifies that the implementation complies to the
 *  following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_7_2 \ref transferElement_B_7_2 "B.7.2"
 */

/********************************************************************************************************************/

/**
 *  Test async read consistency heartbeat
 * 
 *  This tests if data consistency is checked and corrected periodically for async reads (if necessary), and it verifies
 *  that the implementation complies to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_8_4 \ref transferElement_B_8_4 "B.8.4"
 */

/********************************************************************************************************************/

/**
 *  Test exception reporting to exceptionBackend
 * 
 *  This tests if exceptions are reported to the exceptionBachend, and it verifies that the implementation complies to
 *  the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_9 \ref transferElement_B_9 "B.9" (with all sub-points)
 *  * \anchor UnifiedTest_TransferElement_B_10 \ref transferElement_B_10 "B.10" (with all sub-points)
 */

/********************************************************************************************************************/

/**
 *  Test small getter functions of accessors
 * 
 *  This tests if small getter functions of accessors work as expected, and it verifies that the implementation complies
 *  for to the following TransferElement specifications:
 *  * \anchor UnifiedTest_TransferElement_B_15_2 \ref transferElement_B_15_2 "B.15.2"
 */

/********************************************************************************************************************/
