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
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

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
};

/**********************************************************************************************************************/

template<typename Derived, typename rawUserType>
struct ShortRaw_base {
  Derived* derived{static_cast<Derived*>(this)};

  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

  DummyRegisterRawAccessor acc{exceptionDummy, "", derived->path()};

  rawUserType get() { return (acc & derived->bitmask) >> derived->bitshift; }
  void set(rawUserType val) {
    acc &= ~derived->bitmask;
    acc |= (val << derived->bitshift) & derived->bitmask;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    UserType v = (get() + derived->rawIncrement) * derived->rawToCooked;
    return {{v}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    UserType v = get() * derived->rawToCooked;
    return {{v}};
  }

  void setRemoteValue() {
    set(get() + derived->rawIncrement);
    std::cout << "setRemoteValue " << derived->path() << " " << float(get()) << " -> " << getRemoteValue<float>()[0][0]
              << std::endl;
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
  }
};

/**********************************************************************************************************************/

struct ShortRaw_signed16 : ShortRaw_base<ShortRaw_signed16, int16_t> {
  std::string path() { return "/ShortRaw/signed16"; }
  typedef int16_t minimumUserType;

  int16_t rawToCooked = 1;
  int16_t rawIncrement = 5;
  int bitshift = 0;
  int32_t bitmask = 0x0000FFFF;
};

/**********************************************************************************************************************/

struct ShortRaw_unsigned16 : ShortRaw_base<ShortRaw_unsigned16, uint16_t> {
  std::string path() { return "/ShortRaw/unsigned16"; }
  typedef uint16_t minimumUserType;

  int16_t rawToCooked = 1;
  int16_t rawIncrement = 11;
  int bitshift = 16;
  int32_t bitmask = 0xFFFF0000;
};

/**********************************************************************************************************************/

struct ShortRaw_fixedPoint16_8u : ShortRaw_base<ShortRaw_fixedPoint16_8u, uint16_t> {
  std::string path() { return "/ShortRaw/fixedPoint16_8u"; }
  typedef float minimumUserType;

  float rawToCooked = 1. / 256.;
  int16_t rawIncrement = 17;
  int bitshift = 0;
  int32_t bitmask = 0x0000FFFF;
};

/**********************************************************************************************************************/

struct ShortRaw_fixedPoint16_8s : ShortRaw_base<ShortRaw_fixedPoint16_8s, int16_t> {
  std::string path() { return "/ShortRaw/fixedPoint16_8s"; }
  typedef float minimumUserType;

  float rawToCooked = 1. / 256.;
  int16_t rawIncrement = 19;
  int bitshift = 16;
  int32_t bitmask = 0xFFFF0000;
};

/**********************************************************************************************************************/

struct ByteRaw_signed8 : ShortRaw_base<ByteRaw_signed8, int8_t> {
  std::string path() { return "/ByteRaw/signed8"; }
  typedef int8_t minimumUserType;

  int8_t rawToCooked = 1;
  int8_t rawIncrement = 23;
  int bitshift = 0;
  int32_t bitmask = 0x000000FF;
};

/**********************************************************************************************************************/

struct ByteRaw_unsigned8 : ShortRaw_base<ByteRaw_unsigned8, uint8_t> {
  std::string path() { return "/ByteRaw/unsigned8"; }
  typedef uint8_t minimumUserType;

  uint8_t rawToCooked = 1;
  int8_t rawIncrement = 29;
  int bitshift = 8;
  int32_t bitmask = 0x0000FF00;
};

/**********************************************************************************************************************/

struct ByteRaw_fixedPoint8_4u : ShortRaw_base<ByteRaw_fixedPoint8_4u, uint8_t> {
  std::string path() { return "/ByteRaw/fixedPoint8_4u"; }
  typedef float minimumUserType;

  float rawToCooked = 1. / 16.;
  int8_t rawIncrement = 37;
  int bitshift = 16;
  int32_t bitmask = 0x00FF0000;
};

/**********************************************************************************************************************/

struct ByteRaw_fixedPoint8_4s : ShortRaw_base<ByteRaw_fixedPoint8_4s, int8_t> {
  std::string path() { return "/ByteRaw/fixedPoint8_4s"; }
  typedef float minimumUserType;

  float rawToCooked = 1. / 16.;
  int8_t rawIncrement = 41;
  int bitshift = 24;
  int32_t bitmask = 0xFF000000;
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
  typedef uint16_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

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
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<Integers_signed32>()
      .addRegister<ShortRaw_signed16>()
      .addRegister<ShortRaw_unsigned16>()
      .addRegister<ShortRaw_fixedPoint16_8u>()
      .addRegister<ShortRaw_fixedPoint16_8s>()
      .addRegister<ByteRaw_signed8>()
      .addRegister<ByteRaw_unsigned8>()
      .addRegister<ByteRaw_fixedPoint8_4s>()
      .addRegister<ByteRaw_fixedPoint8_4u>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMultiplexedRegisterAccessor) {
  std::cout << "*** testMultiplexedRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>().addRegister<MuxedNodma>().runTests(cddMuxed);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
