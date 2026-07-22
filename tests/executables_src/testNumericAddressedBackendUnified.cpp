// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendUnified
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
 * Despite its name, this test also qualifies as an unified test for the (Exception)DummyBackend.
 * They are thinish implementations of the NumericAddressedBackend's internal interface.
 * ==============================================================================================*/

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(NumericAddressedBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=test3.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

static std::string cddMuxed("(ExceptionDummy:1?map=muxedDataAcessor.map)");
static auto exceptionDummyMuxed =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cddMuxed));

static std::string cddBitRange("(ExceptionDummy:1?map=unifiedTest.jmap)");
static auto exceptionDummyBitRange =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cddBitRange));

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

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", path()};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{acc + 3}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{acc}};
  }

  void setRemoteValue() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

struct Integers_signed32_async {
  std::string path() { return "/Integers/signed32_async"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }
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
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", path()};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{acc + 3}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{acc}};
  }

  void setRemoteValue() {
    acc = generateValue<minimumUserType>()[0][0];
    if(exceptionDummy->isOpen()) exceptionDummy->triggerInterrupt(6);
  }

  void forceAsyncReadInconsistency() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
    if(exceptionDummy->isOpen()) exceptionDummy->triggerInterrupt(6);
  }
};
struct Integers_signed32_async_rw {
  // Using the DUMMY_WRITEABLE register here since usually an async register is r/o implicitly
  std::string path() { return "/Integers/signed32_async/DUMMY_WRITEABLE"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }
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
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", "/Integers/signed32_async"};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{acc + 3}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{acc}};
  }

  void setRemoteValue() {
    acc = generateValue<minimumUserType>()[0][0];
    if(exceptionDummy->isOpen()) exceptionDummy->triggerInterrupt(6);
  }

  void forceAsyncReadInconsistency() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
    if(exceptionDummy->isOpen()) exceptionDummy->triggerInterrupt(6);
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

  // The padding (i.e. the bits of the 32 bit word which are not part of the register under test but of the neighbouring
  // registers) needs to be checked to be unchanged in write operations. This test assumes that in between the calls to
  // generateValue() and getRemoteValue() the padding bits are not *intentionally* changed (e.g. by testing another
  // register in between). This is true for the UnifiedBackendTest right now.
  // TODO: In future it may be better to foresee a "no-side-effect" test in the UnifiedBackendTest, which tests that
  // other registers stay unchanged (caveat: need to specify which registers to look at! We have intentionally
  // overlapping registers in this test which of course will change!)
  int32_t lastPadding{0};
  bool printPaddingChangedMessage{true};

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  // This register shares the address space with all our test registers. It gives us direct access to the 4 byte
  // address range, so we can test the correct placement of the unaligned values.
  DummyRegisterRawAccessor acc{exceptionDummy, "", "/Integers/unsigned32"};

  rawUserType get() { return (acc & derived->bitmask) >> derived->bitshift; }

  void set(rawUserType val) {
    acc &= ~derived->bitmask;
    lastPadding = acc;
    printPaddingChangedMessage = true;
    acc |= (val << derived->bitshift) & derived->bitmask;
  }

  // Type can be user type or raw type, depeding of the raw flag
  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool raw = false) {
    rawUserType newRawValue = static_cast<rawUserType>(get() + derived->rawIncrement);
    Type v = ChimeraTK::numeric::convert<Type>(raw ? newRawValue : (newRawValue * derived->rawToCooked));
    /* std::cout << "generateValue " << derived->path() << " " << float(rawUserType(get() + derived->rawIncrement))
              << " -> " << float(v) << std::endl; */
    lastPadding = acc & ~derived->bitmask;
    printPaddingChangedMessage = true;
    return {{v}};
  }

  // We use the same function to implement raw and cooked reading.
  // Do the calculation on the target type, then decide on the padding error.
  // rawToCooked might undo the ++ effect in case
  // of rounding. When getting cooked, the ++ must happen on the UserType.
  // Type can be UserType or RawType.
  template<typename Type>
  std::vector<std::vector<Type>> getRemoteValue(bool raw = false) {
    Type v = get() * (raw ? 1 : derived->rawToCooked);
    if((acc & ~derived->bitmask) != lastPadding) {
      if(printPaddingChangedMessage) {
        std::cerr << "getRemoteValue(): Padding data has changed. Test will be failed by returing a false remote value "
                  << "(off by one)." << std::endl;
        printPaddingChangedMessage = false;
      }
      v++;
      return {{v}};
    }
    /* std::cout << "getRemoteValue " << derived->path() << " " << float(get()) << " -> " << float(v) << std::endl; */
    return {{v}};
  }

  void setRemoteValue() {
    set(get() + derived->rawIncrement);
    /* std::cout << "setRemoteValue " << derived->path() << " " << float(get()) << " -> " <<
       getRemoteValue<float>()[0][0]
              << std::endl; */
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

struct ShortRaw_signed16 : ShortRaw_base<ShortRaw_signed16, int16_t> {
  std::string path() { return "/ShortRaw/signed16"; }
  typedef int16_t minimumUserType;
  typedef minimumUserType rawUserType;

  int16_t rawToCooked = 1;
  // The rawIncrements are chosen such that we generate different values for different variables, and to cover the
  // specific value range with very few values (to check signed vs. unsigned). Uncomment the above couts in the base
  // class to check the generated values.
  int16_t rawIncrement = 17117;
  int bitshift = 0;
  int32_t bitmask = 0x0000FFFF;
};

/**********************************************************************************************************************/

struct ShortRaw_unsigned16 : ShortRaw_base<ShortRaw_unsigned16, uint16_t> {
  std::string path() { return "/ShortRaw/unsigned16"; }
  typedef uint16_t minimumUserType;
  typedef int16_t rawUserType;

  int16_t rawToCooked = 1;
  int16_t rawIncrement = 17119;
  int bitshift = 16;
  int32_t bitmask = 0xFFFF0000;
};

/**********************************************************************************************************************/

struct ShortRaw_fixedPoint16_8u : ShortRaw_base<ShortRaw_fixedPoint16_8u, uint16_t> {
  std::string path() { return "/ShortRaw/fixedPoint16_8u"; }
  typedef float minimumUserType;
  typedef int16_t rawUserType;

  float rawToCooked = 1. / 256.;
  int16_t rawIncrement = 17121;
  int bitshift = 0;
  int32_t bitmask = 0x0000FFFF;
};

/**********************************************************************************************************************/

struct ShortRaw_fixedPoint16_8s : ShortRaw_base<ShortRaw_fixedPoint16_8s, int16_t> {
  std::string path() { return "/ShortRaw/fixedPoint16_8s"; }
  typedef float minimumUserType;
  typedef int16_t rawUserType;

  float rawToCooked = 1. / 256.;
  int16_t rawIncrement = 17123;
  int bitshift = 16;
  int32_t bitmask = 0xFFFF0000;
};

/**********************************************************************************************************************/

struct ByteRaw_signed8 : ShortRaw_base<ByteRaw_signed8, int8_t> {
  std::string path() { return "/ByteRaw/signed8"; }
  typedef int8_t minimumUserType;
  typedef int8_t rawUserType;

  int8_t rawToCooked = 1;
  int8_t rawIncrement = 119;
  int bitshift = 0;
  int32_t bitmask = 0x000000FF;
};

/**********************************************************************************************************************/

struct ByteRaw_unsigned8 : ShortRaw_base<ByteRaw_unsigned8, uint8_t> {
  std::string path() { return "/ByteRaw/unsigned8"; }
  typedef uint8_t minimumUserType;
  typedef int8_t rawUserType;

  uint8_t rawToCooked = 1;
  int8_t rawIncrement = 121;
  int bitshift = 8;
  int32_t bitmask = 0x0000FF00;
};

/**********************************************************************************************************************/

struct ByteRaw_fixedPoint8_4u : ShortRaw_base<ByteRaw_fixedPoint8_4u, uint8_t> {
  std::string path() { return "/ByteRaw/fixedPoint8_4u"; }
  typedef float minimumUserType;
  typedef int8_t rawUserType;

  float rawToCooked = 1. / 16.;
  int8_t rawIncrement = 123;
  int bitshift = 16;
  int32_t bitmask = 0x00FF0000;
};

/**********************************************************************************************************************/

struct ByteRaw_fixedPoint8_4s : ShortRaw_base<ByteRaw_fixedPoint8_4s, int8_t> {
  std::string path() { return "/ByteRaw/fixedPoint8_4s"; }
  typedef float minimumUserType;
  typedef int8_t rawUserType;

  float rawToCooked = 1. / 16.;
  int8_t rawIncrement = 125;
  int bitshift = 24;
  int32_t bitmask = 0xFF000000;
};

/**********************************************************************************************************************/

struct AsciiData {
  static std::string path() { return "/Text/someAsciiData"; }
  static bool isWriteable() { return true; }
  static bool isReadable() { return true; }
  using minimumUserType = std::string;
  static ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  static size_t nChannels() { return 1; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  static size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  // Note: The DummyRegisterAccessor does not yet work properly with none 32-bit word sizes.
  DummyRegisterAccessor<uint32_t> acc{exceptionDummy.get(), "Text", "someAsciiData_as_i32"};

  std::string textBase{"Some $%#@! characters "};
  size_t counter{0};
  const size_t asciiLength{35};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return {{textBase + std::to_string(counter++)}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<std::vector<UserType>> v{{""}};
    for(size_t i = 0; i < asciiLength; ++i) {
      size_t word = i / 4;
      size_t byteInWord = i % 4;
      char ch = char((acc[word] >> (byteInWord * 8U)) & 0xFF);
      if(ch == 0) break;
      v[0][0] += ch;
    }
    std::cout << "getRemoteValue: " << v[0][0] << std::endl;
    return v;
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(size_t i = 0; i < acc.getNumberOfElements(); ++i) {
      acc[i] = 0;
    }
    for(size_t i = 0; i < std::min(asciiLength, v[0][0].length()); ++i) {
      char ch = v[0][0][i];
      size_t word = i / 4;
      size_t byteInWord = i % 4;
      acc[word] = acc[word] | uint32_t(ch) << (byteInWord * 8U);
    }
    std::cout << "setRemoteValue: " << v[0][0] << std::endl;
  }

  static void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
  }
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

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

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
    exceptionDummyMuxed->throwExceptionOpen = enable;
  }
};

struct MuxedNodmaAsync {
  std::string path() { return "/TEST/NODMAASYNC"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }

  size_t nChannels() { return 16; }
  size_t nElementsPerChannel() { return 4; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef uint16_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  DummyMultiplexedRegisterAccessor<uint16_t> acc{exceptionDummyMuxed.get(), "TEST", "NODMAASYNC"};

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
    if(exceptionDummyMuxed->isOpen()) exceptionDummyMuxed->triggerInterrupt(6);
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummyMuxed->throwExceptionRead = enable;
    exceptionDummyMuxed->throwExceptionWrite = enable;
    exceptionDummyMuxed->throwExceptionOpen = enable;
    if(exceptionDummyMuxed->isOpen()) exceptionDummyMuxed->triggerInterrupt(6);
  }
};

/**********************************************************************************************************************/

struct MuxedFloat {
  std::string path() { return "/TEST/FLOAT"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nChannels() { return 4; }
  size_t nElementsPerChannel() { return 8; }
  size_t nRuntimeErrorCases() { return 1; }
  typedef float minimumUserType;
  // typedef int32_t rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  DummyMultiplexedRegisterAccessor<float> acc{exceptionDummyMuxed.get(), "TEST", "FLOAT"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<std::vector<UserType>> v;
    v.resize(nChannels());
    for(uint32_t c = 0; c < nChannels(); ++c) {
      for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
        v[c].push_back(acc[c][e] + 0.7f * c + 3 * e);
      }
    }
    return v;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<std::vector<UserType>> v;
    v.resize(nChannels());
    for(uint32_t c = 0; c < nChannels(); ++c) {
      for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
        v[c].push_back(acc[c][e]);
      }
    }
    return v;
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(uint32_t c = 0; c < nChannels(); ++c) {
      for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
        std::cout << "cookedValue is " << v[c][e] << std::endl;
        acc[c][e] = v[c][e];
      }
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummyMuxed->throwExceptionRead = enable;
    exceptionDummyMuxed->throwExceptionWrite = enable;
    exceptionDummyMuxed->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

// Base descriptor with defaults, used for all register descriptor which use the base classes from this file
template<typename Derived>
struct RegisterDescriptorBase {
  Derived* derived{static_cast<Derived*>(this)};

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  bool isPush() { return false; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    ChimeraTK::AccessModeFlags flags{ChimeraTK::AccessMode::raw};
    if(derived->isPush()) flags.add(ChimeraTK::AccessMode::wait_for_new_data);
    return flags;
  }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }

  void setForceRuntimeError(bool enable, size_t) {
    auto& dummy = dynamic_cast<ExceptionDummy&>(derived->acc.getBackend());
    dummy.throwExceptionRead = enable;
    dummy.throwExceptionWrite = enable;
    dummy.throwExceptionOpen = enable;
    if(derived->isPush() && enable) {
      dummy.triggerInterrupt(6);
    }
  }
};

/// Base descriptor for 1D accessors (and scalars)
template<typename Derived>
struct OneDRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

  size_t nChannels() { return 1; }

  size_t myOffset() { return 0; }

  // T is always minimumUserType, but C++ doesn't allow to use Derived::minimumUserType here (circular dependency)
  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return static_cast<T>(value);
  }

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {} // override in derived if needed

  // type can be user type or raw type
  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool getRaw = false) {
    std::vector<Type> v;
    typedef typename Derived::rawUserType Traw;
    typedef typename Derived::minimumUserType T;
    auto cv = derived->template getRemoteValue<Traw>(true)[0];
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      Traw e = cv[i] + derived->increment * (static_cast<Traw>(i) + 1);
      if(!getRaw) {
        v.push_back(derived->template convertRawToCooked<T, Traw>(e));
      }
      else {
        v.push_back(static_cast<T>(e));
      }
    }
    derived->generateValueHook(v);
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool getRaw = false) {
    typedef typename Derived::rawUserType Traw;

    std::vector<UserType> cookedValues(derived->nElementsPerChannel());
    std::vector<Traw> rawValues(derived->nElementsPerChannel());

    // Keep the scope of the dummy buffer lock as limited as possible (see #12332).
    // The rawToCooked conversion will acquire a lock via the math pluging decorator,
    // which will cause lock order inversion if you hold the dummy buffer lock at that point.
    {
      auto bufferLock = derived->acc.getBufferLock();

      for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
        rawValues[i] = derived->acc[i + derived->myOffset()];
      }
    } // end of bufferLock scope

    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      if(!getRaw) {
        cookedValues[i] = derived->template convertRawToCooked<UserType, Traw>(rawValues[i]);
      }
      else {
        cookedValues[i] = static_cast<UserType>(rawValues[i]);
      }
    }
    return {cookedValues};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::rawUserType>(true)[0];
    { // scope for the buffer lock
      auto bufferLock = derived->acc.getBufferLock();
      for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
        derived->acc[i + derived->myOffset()] = v[i];
      }
    } // release the buffer lock before triggering another thread
    if(derived->isPush()) {
      dynamic_cast<ExceptionDummy&>(derived->acc.getBackend()).triggerInterrupt(6);
    }
  }
};

/// Base descriptor for scalars
template<typename Derived>
struct ScalarRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  size_t nElementsPerChannel() { return 1; }
};

  // Base descriptor for bit accessors
template<typename Derived>
struct RegBitRangeDescriptor : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities;

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }

  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nValuesToTest() { return 1; }

  size_t nRuntimeErrorCases() { return derived->target.nRuntimeErrorCases(); }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool getRaw = false) {
    if(getRaw) {
      // For raw mode, generate a full word value (raw accessor returns the full parent word)
      return derived->target.template generateValue<UserType>(true);
    }
    // For cooked mode, delegate to target (generates a word value, which when written
    // to the bit range and read back via getRemoteValue will round-trip correctly)
    return derived->target.template generateValue<UserType>();
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool getRaw = false) {
    if(getRaw) {
      // For raw mode, return the full word value (raw accessor returns full parent word)
      return derived->target.template getRemoteValue<UserType>(true);
    }
    // For cooked mode, extract the bit range
    uint64_t v = derived->target.template getRemoteValue<uint64_t>()[0][0];
    uint64_t mask = ((1ull << derived->width) - 1) << derived->shift;
    UserType result = static_cast<UserType>((v & mask) >> derived->shift);
    return {{result}};
  }

  void setRemoteValue() { 
    derived->target.setRemoteValue(); 
    std::cout << "DEBUG setRemoteValue: target.acc = 0x" << std::hex 
              << (uint32_t)derived->target.acc << std::dec 
              << " (full word 0x" << std::hex << (uint32_t)derived->target.acc << std::dec
              << ") path=" << derived->path() << std::endl;
  }

  /// Read the raw full word value from _barContents directly via the DummyRegisterAccessor
  uint64_t debugReadRawFullWord() {
    return static_cast<uint64_t>(static_cast<uint32_t>(derived->target.acc));
  }

  void setForceRuntimeError(bool enable, size_t caseIndex) { derived->target.setForceRuntimeError(enable, caseIndex); }
};

struct BitRangeAccessorTarget : ScalarRegisterDescriptorBase<BitRangeAccessorTarget> {
  std::string path() { return "/WORD_FIRMWARE"; }

  const uint32_t increment = 0x1313'2131;

  using minimumUserType = uint32_t;
  using rawUserType = int32_t;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyBitRange.get(), "", "/WORD_FIRMWARE"};
};

struct RegLowerHalfOfFirmware : RegBitRangeDescriptor<RegLowerHalfOfFirmware> {
  std::string path() { return "/WORD_FIRMWARE/BitRangeLower"; }

  using minimumUserType = int8_t;
  using rawUserType = int32_t;

  uint16_t width = 8;
  uint16_t shift = 8;

  BitRangeAccessorTarget target;
};

struct RegUpperHalfOfFirmware : RegBitRangeDescriptor<RegUpperHalfOfFirmware> {
  std::string path() { return "/WORD_FIRMWARE/BitRangeUpper"; }

  using minimumUserType = int16_t;
  using rawUserType = int32_t;

  uint16_t width = 16;
  uint16_t shift = 16;

  BitRangeAccessorTarget target;
};

struct Reg9BitsInChar : RegBitRangeDescriptor<Reg9BitsInChar> {
  std::string path() { return "/WORD_FIRMWARE/BitRangeMiddle"; }

  using minimumUserType = int8_t;
  using rawUserType = int32_t;

  uint16_t width = 9;
  uint16_t shift = 4;

  BitRangeAccessorTarget target;
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<Integers_signed32>()
      .addRegister<Integers_signed32_async>()
      /* .addRegister<Integers_signed32_async_rw>()  // disabled for now as .DUMMY_WRITABLE no longer supports
         wait_for_new_data */
      .addRegister<ShortRaw_signed16>()
      .addRegister<ShortRaw_unsigned16>()
      .addRegister<ShortRaw_fixedPoint16_8u>()
      .addRegister<ShortRaw_fixedPoint16_8s>()
      .addRegister<ByteRaw_signed8>()
      .addRegister<ByteRaw_unsigned8>()
      .addRegister<ByteRaw_fixedPoint8_4s>()
      .addRegister<ByteRaw_fixedPoint8_4u>()
      .addRegister<AsciiData>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMultiplexedRegisterAccessor) {
  std::cout << "*** testMultiplexedRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<MuxedNodma>()
      .addRegister<MuxedNodmaAsync>()
      .addRegister<MuxedFloat>()
      .runTests(cddMuxed);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBitRanges) {
  std::cout << "*** testBitRanges *** " << std::endl;

  // Quick debug: simulate exactly what test_B_3_1_2_1 does for RegLowerHalfOfFirmware
  {
    // Static target already exists via the static global exceptionDummyBitRange
    // Create the descriptors that will be used in the test
    RegLowerHalfOfFirmware desc;
    
    ChimeraTK::Device d(cddBitRange);
    auto reg = d.getTwoDRegisterAccessor<int8_t>("/WORD_FIRMWARE/BitRangeLower");
    
    std::cout << "DEBUG: BEFORE setRemoteValue: target.acc = 0x" << std::hex << (uint32_t)desc.target.acc << std::dec << std::endl;
    
    // Step 1: setRemoteValue
    desc.setRemoteValue();
    auto v1 = desc.getRemoteValue<int8_t>();
    std::cout << "DEBUG: AFTER setRemoteValue: target.acc = 0x" << std::hex << (uint32_t)desc.target.acc << std::dec << std::endl;
    std::cout << "DEBUG: AFTER setRemoteValue: getRemoteValue = " << (int)v1[0][0] << std::endl;
    
    // Step 2: open and read
    d.open();
    reg.read();
    std::cout << "DEBUG: AFTER open+read: Device read = " << (int)reg[0][0] << std::endl;
    
    d.close();
    std::cout << "DEBUG: AFTER close: target.acc = 0x" << std::hex << (uint32_t)desc.target.acc << std::dec << std::endl;
  }

  // Test after close cycle - same flow as test_B_3_1_2_1
  {
    RegLowerHalfOfFirmware desc;
    ChimeraTK::Device d(cddBitRange);
    auto reg = d.getTwoDRegisterAccessor<int8_t>("/WORD_FIRMWARE/BitRangeLower");
    std::cout << "DEBUG2: BEFORE setRemoteValue: target.acc = 0x" << std::hex << (uint32_t)desc.target.acc << std::dec << std::endl;
    desc.setRemoteValue();
    auto v1 = desc.getRemoteValue<int8_t>();
    std::cout << "DEBUG2: AFTER setRemoteValue: getRemoteValue = " << (int)v1[0][0] << std::endl;
    d.open();
    reg.read();
    std::cout << "DEBUG2: AFTER open+read: Device read = " << (int)reg[0][0] << std::endl;
    d.close();
  }

  // Debug: replicate exact test_B_3_1_2_1 flow for all three registers with close between
  // No rawReg accessors — testing if bug reproduces without them
  {
    ChimeraTK::Device d(cddBitRange);
    
    // --- BitRangeLower ---
    {
      RegLowerHalfOfFirmware desc;
      auto reg = d.getTwoDRegisterAccessor<int8_t>("/WORD_FIRMWARE/BitRangeLower");
      
      desc.setRemoteValue();
      auto v1 = desc.getRemoteValue<int8_t>();
      std::cout << "NO_RAW:Lower v1=" << (int)v1[0][0] << std::endl;
      
      d.open();
      reg.read();
      std::cout << "NO_RAW:Lower R1=" << (int)reg[0][0] << std::endl;
      
      desc.setRemoteValue();
      reg.read();
      std::cout << "NO_RAW:Lower R2=" << (int)reg[0][0] << std::endl;
      
      d.close();
    }
    
    // --- BitRangeUpper ---
    {
      RegUpperHalfOfFirmware desc;
      auto reg = d.getTwoDRegisterAccessor<int16_t>("/WORD_FIRMWARE/BitRangeUpper");
      
      desc.setRemoteValue();
      auto v1 = desc.getRemoteValue<int16_t>();
      std::cout << "NO_RAW:Upper v1=" << (int)v1[0][0] << std::endl;
      
      d.open();
      reg.read();
      std::cout << "NO_RAW:Upper R1=" << (int)reg[0][0] << std::endl;
      
      desc.setRemoteValue();
      reg.read();
      std::cout << "NO_RAW:Upper R2=" << (int)reg[0][0] << std::endl;
      
      d.close();
    }
    
    // --- BitRangeMiddle ---
    {
      Reg9BitsInChar desc;
      auto reg = d.getTwoDRegisterAccessor<int8_t>("/WORD_FIRMWARE/BitRangeMiddle");
      
      desc.setRemoteValue();
      auto v1 = desc.getRemoteValue<int8_t>();
      std::cout << "NO_RAW:Middle v1=" << (int)v1[0][0] << std::endl;
      
      d.open();
      reg.read();
      std::cout << "NO_RAW:Middle R1=" << (int)reg[0][0] << std::endl;
      
      d.close();
    }
    
    std::cout << "--- NO_RAW test complete ---" << std::endl;
  }

  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegLowerHalfOfFirmware>()
      .addRegister<RegUpperHalfOfFirmware>()
      .addRegister<Reg9BitsInChar>()
      .runTests(cddBitRange);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
