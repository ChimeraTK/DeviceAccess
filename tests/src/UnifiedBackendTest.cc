#include "UnifiedBackendTest.h"

#include <boost/test/unit_test.hpp>

#include "Device.h"

namespace ctk = ChimeraTK;

/********************************************************************************************************************/

void UnifiedBackendTest::runTests(const std::string& backend) {
  cdd = backend;
  // check inputs
  if(forceExceptionsRead.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnRead() not called with a non-empty list." << std::endl;
    std::exit(1);
  }
  if(forceExceptionsWrite.size() == 0) {
    std::cout << "UnifiedBackendTest::forceRuntimeErrorOnWrite() not called with a non-empty list." << std::endl;
    std::exit(1);
  }
  if(regInteger.size() == 0) {
    std::cout << "UnifiedBackendTest::integerRegister() not called with a non-empty list." << std::endl;
    std::exit(1);
  }
}

/********************************************************************************************************************/

void UnifiedBackendTest::basicExceptionHandling() {
  ctk::Device d(cdd);

  for(auto& registerName : regInteger) {
    auto reg = d.getScalarRegisterAccessor<int32_t>(registerName);

    // check "value after construction"
    BOOST_CHECK_EQUAL(reg, 0);
    BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

    // repeat the following check for a list of actions
    std::list<std::function<void(void)>> actionListRead, actionListWrite;
    actionListRead.push_back([&] { reg.read(); });
    actionListRead.push_back([&] { reg.readNonBlocking(); });
    actionListRead.push_back([&] { reg.readLatest(); });
    actionListRead.push_back([&] { reg.readAsync().wait(); });
    actionListWrite.push_back([&] { reg.write(); });
    actionListWrite.push_back([&] { reg.writeDestructively(); });

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
      theTest(action, ctk::logic_error("only type matters"));
    }
    for(auto action : actionListWrite) {
      theTest(action, ctk::logic_error("only type matters"));
    }

    // open the device, let it throw an exception on every read and write operation
    d.open();

    for(auto& testCondition : forceExceptionsRead) {
      // enable exceptions on read
      testCondition.first();

      // repeat the above test, this time a runtime_error is expected
      for(auto action : actionListRead) {
        theTest(action, ctk::runtime_error("only type matters"));
      }

      // disable exceptions on read
      testCondition.second();
    }

    for(auto& testCondition : forceExceptionsWrite) {
      // enable exceptions on write
      testCondition.first();

      // repeat the above test, this time a runtime_error is expected
      for(auto action : actionListWrite) {
        theTest(action, ctk::runtime_error("only type matters"));
      }

      // disable exceptions on write
      testCondition.second();
    }
  }
}

/********************************************************************************************************************/
