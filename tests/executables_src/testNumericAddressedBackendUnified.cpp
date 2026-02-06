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

  DummyMultiplexedRegisterAccessor<int32_t> rawAcc{exceptionDummyMuxed.get(), "TEST", "FLOAT"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<std::vector<UserType>> v;
    v.resize(nChannels());
    for(uint32_t c = 0; c < nChannels(); ++c) {
      for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
        int32_t rawValue = rawAcc[c][e];
        float* cookedValue = reinterpret_cast<float*>(&rawValue);
        v[c].push_back(*cookedValue + 0.7f * c + 3 * e);
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
        int32_t rawValue = rawAcc[c][e];
        float* cookedValue = reinterpret_cast<float*>(&rawValue);
        v[c].push_back(*cookedValue);
      }
    }
    return v;
  }

  void setRemoteValue() {
    auto v = generateValue<minimumUserType>();
    for(uint32_t c = 0; c < nChannels(); ++c) {
      for(uint32_t e = 0; e < nElementsPerChannel(); ++e) {
        int32_t rawValue;
        float* cookedValue = reinterpret_cast<float*>(&rawValue);
        *cookedValue = v[c][e];
        std::cout << "raw value is " << rawValue << "; cookedValue is " << *cookedValue << std::endl;
        rawAcc[c][e] = rawValue;
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

BOOST_AUTO_TEST_SUITE_END()
