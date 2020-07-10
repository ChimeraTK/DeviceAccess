#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "TransferGroup.h"
#include "BackendFactory.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(NumericAddressedBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=test3.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

static std::string cddMuxed("(ExceptionDummy:1?map=muxedDataAcessor.map)");
static auto exceptionDummyMuxed =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cddMuxed));

/**********************************************************************************************************************/

struct Integers_signed32 {
  std::string path() { return "/Integers/signed32"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  bool testAsyncReadInconsistency() { return false; }
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", path()};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return {{acc + 3}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return {{acc}};
  }

  void setRemoteValue() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
  }

  void setForceDataLossWrite(bool) { assert(false); }

  void forceAsyncReadInconsistency() { assert(false); }
};

/**********************************************************************************************************************/

struct MuxedNodma {
  std::string path() { return "/TEST/NODMA"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nChannels() { return 16; }
  size_t nElementsPerChannel() { return 4; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  bool testAsyncReadInconsistency() { return false; }
  typedef uint16_t minimumUserType;
  typedef minimumUserType rawUserType;

  DummyMultiplexedRegisterAccessor<uint16_t> acc{exceptionDummyMuxed.get(), "TEST", "NODMA"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<std::vector<UserType>> v;
    v.resize(nChannels());
    for(size_t c = 0; c < nChannels(); ++c) {
      for(size_t e = 0; e < nElementsPerChannel(); ++e) {
        v[c].push_back(uint16_t(acc[c][e] + 7 * c + 3 * e));
      }
    }
    return v;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<std::vector<UserType>> v;
    v.resize(nChannels());
    for(size_t c = 0; c < nChannels(); ++c) {
      for(size_t e = 0; e < nElementsPerChannel(); ++e) {
        v[c].push_back(acc[c][e]);
      }
    }
    return v;
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(size_t c = 0; c < nChannels(); ++c) {
      for(size_t e = 0; e < nElementsPerChannel(); ++e) {
        acc[c][e] = v[c][e];
      }
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummyMuxed->throwExceptionRead = enable;
    exceptionDummyMuxed->throwExceptionWrite = enable;
  }

  void setForceDataLossWrite(bool) { assert(false); }

  void forceAsyncReadInconsistency() { assert(false); }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Integers_signed32>().runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMultiplexedRegisterAccessor) {
  std::cout << "*** testMultiplexedRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>().addRegister<MuxedNodma>().runTests(cddMuxed);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
