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
  class ActionList : public std::list<std::pair<std::function<void(void)>, std::function<void(void)>>> {
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
  void forceRuntimeErrorOnRead(const ActionList& list) { forceExceptionsRead = list; }

  /**
   *  Set functor, which will do whatever necessary that the backend will throw a ChimeraTK::runtime_error for any write
   *  operation.
   */
  void forceRuntimeErrorOnWrite(const ActionList& list) { forceExceptionsWrite = list; }

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

 private:
  void valueAfterConstruction();
  void exceptionHandlingSyncRead();
  void exceptionHandlingAsyncRead();
  void exceptionHandlingWrite();

  /// Actions for enable exception throwing
  ActionList forceExceptionsRead, forceExceptionsWrite;

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
  exceptionHandlingSyncRead();
  exceptionHandlingAsyncRead();
  exceptionHandlingWrite();
  valueAfterConstruction();
}

/********************************************************************************************************************/

/**
 * This tests the TransferElement specifications:
 * * B.11.6 and
 * * MISSING SPEC for value after construction of data buffer
 */
void UnifiedBackendTest::valueAfterConstruction() {
  std::cout << "--- valueAfterConstruction" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(allRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // check "value after construction"
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
    }
  });
}

/********************************************************************************************************************/

void UnifiedBackendTest::exceptionHandlingSyncRead() {
  std::cout << "--- exceptionHandlingSyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(readRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // attempt read while device closed, logic_error is expected.
      BOOST_CHECK_THROW(reg.read(), ChimeraTK::logic_error);

      // check "value after construction" still there
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // repeat the above test, this time a runtime_error is expected
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

        // disable exceptions on read
        testCondition.second();
      }

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/

void UnifiedBackendTest::exceptionHandlingAsyncRead() {
  std::cout << "--- exceptionHandlingAsyncRead" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(asyncReadRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName, 0, 0, {ctk::AccessMode::wait_for_new_data});

      // attempt read while device closed, logic_error is expected.
      BOOST_CHECK_THROW(reg.read(), ChimeraTK::logic_error);

      // check "value after construction" still there
      std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
      for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // open the device, let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // repeat the above test, this time a runtime_error is expected
        BOOST_CHECK_THROW(reg.read(), ChimeraTK::runtime_error);

        // disable exceptions on read
        testCondition.second();
      }

      // close device again
      d.close();
    }
  });
}
/********************************************************************************************************************/

void UnifiedBackendTest::exceptionHandlingWrite() {
  std::cout << "--- exceptionHandlingWrite" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(writeRegisters.table, [&](auto pair) {
    typedef typename decltype(pair)::first_type UserType;
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getTwoDRegisterAccessor<UserType>(registerName);

      // repeat the following check for a list of actions
      std::list<std::pair<std::string, std::function<void(void)>>> actionList;
      actionList.push_back({"write()", [&] { reg.write(); }});
      actionList.push_back({"writeDestructively()", [&] { reg.writeDestructively(); }});

      // define lambda for the test to execute repetedly (avoid code duplication)
      auto theTest = [&](auto& theAction, auto expectedExceptionType) {
        // attempt action
        BOOST_CHECK_THROW(theAction(), decltype(expectedExceptionType));

        // check "value after construction" still there
        std::vector<UserType> v(reg.getNElementsPerChannel(), UserType());
        for(size_t i = 0; i < reg.getNChannels(); ++i) BOOST_CHECK(reg[i] == v);
        BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
      };

      // attempt read while device closed, logic_error is expected.
      for(auto action : actionList) {
        std::cout << "    " << action.first << std::endl;
        theTest(action.second, ctk::logic_error("only type matters"));
      }

      // open the device, let it throw runtime_error exceptions
      d.open();

      for(auto& testCondition : forceExceptionsWrite) {
        // enable exceptions on write
        testCondition.first();

        // repeat the above test, this time a runtime_error is expected
        for(auto action : actionList) {
          std::cout << "    " << action.first << std::endl;
          theTest(action.second, ctk::runtime_error("only type matters"));
        }

        // disable exceptions on write
        testCondition.second();
      }

      // close device again
      d.close();
    }
  });
}

/********************************************************************************************************************/
