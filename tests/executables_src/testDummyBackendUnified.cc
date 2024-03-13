// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE DummyBackendUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

/* ===============================================================================================
 * This test covers accessors and features that are specific to the DummyBackend
 * ==============================================================================================*/

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(DummyBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=test3.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

struct Interrupt_dummy {
  std::string path() { return "/DUMMY_INTERRUPT_6"; }
  bool isWriteable() { return true; }
  bool isReadable() { return false; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return {{1}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return {{1}};
  }

  void setRemoteValue() {}

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Interrupt_dummy>().runTests(cdd);
}
BOOST_AUTO_TEST_SUITE_END()
