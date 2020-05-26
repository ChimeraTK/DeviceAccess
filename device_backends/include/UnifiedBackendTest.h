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
   *  Set the names of integer registers to be used for the tests
   */
  void integerRegister(const std::list<std::string>& names) { setTestRegisters<int>(names); }

  template<typename UserType>
  void setTestRegisters(const std::list<std::string>& names) {
    boost::fusion::at_key<int>(registers.table) = names;
  }

 private:
  /**
   *  Test basic exception handling behavior
   */
  void basicExceptionHandling();

  /// Actions for enable exception throwing
  ActionList forceExceptionsRead, forceExceptionsWrite;

  /// CDD for backend to test
  std::string cdd;

  /// Name of integer register used for tests
  ctk::FixedUserTypeMap<std::list<std::string>> registers;
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

  // check inputs
  if(forceExceptionsRead.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnRead() not called with a non-empty list." << std::endl;
    std::exit(1);
  }
  if(forceExceptionsWrite.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnWrite() not called with a non-empty list." << std::endl;
    std::exit(1);
  }

  size_t nRegisters = 0;
  auto lambda = [&nRegisters](auto pair) { nRegisters += pair.second.size(); };
  ctk::for_each(registers.table, lambda);
  if(nRegisters == 0) {
    std::cout << "UnifiedBackendTest: No test registers specified." << std::endl;
    std::exit(1);
  }
  std::cout << "UnifiedBackendTest: Using  " << nRegisters << " test registers." << std::endl;

  // run the tests
  basicExceptionHandling();
}

/********************************************************************************************************************/

void UnifiedBackendTest::basicExceptionHandling() {
  std::cout << "--- basicExceptionHandling" << std::endl;
  ctk::Device d(cdd);

  ctk::for_each(registers.table, [&](auto pair) {
    for(auto& registerName : pair.second) {
      std::cout << "... registerName = " << registerName << std::endl;
      auto reg = d.getScalarRegisterAccessor<int32_t>(registerName);

      // check "value after construction"
      BOOST_CHECK_EQUAL(reg, 0);
      BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

      // repeat the following check for a list of actions
      std::list<std::pair<std::string, std::function<void(void)>>> actionListRead, actionListWrite;
      actionListRead.push_back({"read()", [&] { reg.read(); }});
      actionListRead.push_back({"readNonBlocking()", [&] { reg.readNonBlocking(); }});
      actionListRead.push_back({"readLatest()", [&] { reg.readLatest(); }});
      actionListRead.push_back({"readAsync()", [&] {
                                  auto future = reg.readAsync();
                                  future.wait();
                                }});
      actionListWrite.push_back({"write()", [&] { reg.write(); }});
      actionListWrite.push_back({"writeDestructively()", [&] { reg.writeDestructively(); }});

      // define lambda for the test to execute repetedly (avoid code duplication)
      auto theTest = [&](auto& theAction, auto expectedExceptionType) {
        // attempt action
        BOOST_CHECK_THROW(theAction(), decltype(expectedExceptionType));

        // check "value after construction" still there
        BOOST_CHECK_EQUAL(reg, 0);
        BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
      };

      // run test for both read and write operations, logic_error is expected
      for(auto action : actionListRead) {
        std::cout << "    " << action.first << std::endl;
        theTest(action.second, ctk::logic_error("only type matters"));
      }
      for(auto action : actionListWrite) {
        std::cout << "    " << action.first << std::endl;
        theTest(action.second, ctk::logic_error("only type matters"));
      }

      // open the device, let it throw an exception on every read and write operation
      d.open();

      for(auto& testCondition : forceExceptionsRead) {
        // enable exceptions on read
        testCondition.first();

        // repeat the above test, this time a runtime_error is expected
        for(auto action : actionListRead) {
          std::cout << "    " << action.first << std::endl;
          theTest(action.second, ctk::runtime_error("only type matters"));
        }

        // disable exceptions on read
        testCondition.second();
      }

      for(auto& testCondition : forceExceptionsWrite) {
        // enable exceptions on write
        testCondition.first();

        // repeat the above test, this time a runtime_error is expected
        for(auto action : actionListWrite) {
          std::cout << "    " << action.first << std::endl;
          theTest(action.second, ctk::runtime_error("only type matters"));
        }

        // disable exceptions on write
        testCondition.second();
      }
    }
  });
}

/********************************************************************************************************************/
