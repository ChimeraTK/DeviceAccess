#include "unifiedBackendTest.h"

#include <boost/test/unit_test.hpp>

#include "Device.h"

namespace ctk = ChimeraTK;

/********************************************************************************************************************/
/********************************************************************************************************************/

void UnifiedBackendTest::basicExceptionHandling(
    std::string cdd, std::string registerName, std::function<void(void)> forceExceptionsReadWrite) {
  ctk::Device d(cdd);

  auto reg = d.getScalarRegisterAccessor<int32_t>(registerName);

  // check "value after construction"
  BOOST_CHECK_EQUAL(reg, 0);
  BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));

  // repeat the following check for a list of actions
  std::list<std::function<void(void)>> actionList;
  actionList.push_back([&] { reg.read(); });
  actionList.push_back([&] { reg.readNonBlocking(); });
  actionList.push_back([&] { reg.readLatest(); });
  actionList.push_back([&] { reg.readAsync().wait(); });
  actionList.push_back([&] { reg.write(); });
  actionList.push_back([&] { reg.writeDestructively(); });
  for(auto action : actionList) {
    // attempt action
    BOOST_CHECK_THROW(action(), ctk::logic_error);

    // check "value after construction" still there
    BOOST_CHECK_EQUAL(reg, 0);
    BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
  }

  // open the device, let it throw an exception on every read and write operation
  d.open();
  forceExceptionsReadWrite();

  // repeat the above test, this time a runtime_error is expected
  for(auto action : actionList) {
    // attempt action
    BOOST_CHECK_THROW(action(), ctk::runtime_error);

    // check "value after construction" still there
    BOOST_CHECK_EQUAL(reg, 0);
    BOOST_CHECK(reg.getVersionNumber() == ctk::VersionNumber(nullptr));
  }
}

/********************************************************************************************************************/
