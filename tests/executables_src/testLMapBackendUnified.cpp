// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "LogicalNameMappingBackend.h"
#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static boost::shared_ptr<ExceptionDummy> exceptionDummyLikeMtcadummy, exceptionDummyMuxed, exceptionDummyPush;
static boost::shared_ptr<LogicalNameMappingBackend> lmapBackend;

/**********************************************************************************************************************/
/* First a number of base descriptors is defined to simplify the descriptors for the individual registers. */

/// Base descriptor with defaults, used for all registers
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
  // Note: I set enableTestRawTransfer to enabled here and disable it where necessary, so new registers will be tested
  // by default.

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

/// Base descriptor for channel accessors
template<typename Derived>
struct ChannelRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();

  size_t nChannels() { return 1; }
  bool isWriteable() { return false; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      v.push_back(derived->acc[derived->channel][k] + derived->increment * (k + 1));
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      v.push_back(derived->acc[derived->channel][k]);
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::minimumUserType>()[0];
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      derived->acc[derived->channel][k] = v[k];
    }
    if(derived->isPush()) {
      // At the moment only interrupt 6 is used, so we hard code it here. Can be made more flexible if needed.
      dynamic_cast<DummyBackend&>(derived->acc.getBackend()).triggerInterrupt(6);
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
        cookedValues[i] = static_cast<UserType>(rawValues[i]); // you can only use raw if user type and raw type are
                                                               // the same, so the static cast is a no-op
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

/// Base descriptor for constant accessors
template<typename Derived>
struct ConstantRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities =
      RegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer().disableSetRemoteValueIncrementsVersion();

  size_t nChannels() { return 1; }
  bool isWriteable() { return false; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nRuntimeErrorCases() { return 0; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return this->getRemoteValue<UserType>();
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      v.push_back(derived->value[k]);
    }
    return {v};
  }

  void setRemoteValue() {}

  void setForceRuntimeError(bool, size_t) { assert(false); }
};

/// Base descriptor for variable accessors
template<typename Derived>
struct VariableRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();

  size_t nChannels() { return 1; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }

  size_t nRuntimeErrorCases() { return 0; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool = false) {
    // For Variables we don't have a backdoor. We have to use the normal read and write
    // functions which are good enough. It seems like a self consistency test, but all
    // functionality the variable has to provide is that I can write something, and
    // read it back, which is tested with it.

    // We might have to open/recover the backend to perform the operation. We have to remember
    // that we did so and close/set-exception it again it we did. Some tests require the backend to be closed.
    bool backendWasOpened = lmapBackend->isOpen();
    bool backendWasFunctional = lmapBackend->isFunctional();
    if(!backendWasOpened || !backendWasFunctional) {
      lmapBackend->open();
    }
    auto acc = lmapBackend->getRegisterAccessor<typename Derived::minimumUserType>(derived->path(), 0, 0, {});
    acc->read();
    if(!backendWasOpened) {
      lmapBackend->close();
    }
    else if(!backendWasFunctional) {
      lmapBackend->setException("Some message");
    }
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      v.push_back(acc->accessData(k));
    }
    return {v};
  }

  void setRemoteValue() {
    auto acc = lmapBackend->getRegisterAccessor<typename Derived::minimumUserType>(derived->path(), 0, 0, {});
    auto v = derived->template generateValue<typename Derived::minimumUserType>()[0];
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      acc->accessData(k) = v[k];
    }
    bool backendWasOpened = lmapBackend->isOpen();
    if(!backendWasOpened) {
      lmapBackend->open();
    }
    try {
      acc->write();
    }
    catch(...) {
      // ignore any exceptions: if the device is in an exception state, this write must not take place but the exception
      // must not get through.
    }
    if(!backendWasOpened) {
      lmapBackend->close();
    }
  }

  void setForceRuntimeError(bool, size_t) { assert(false); }
};

// Base descriptor for bit accessors
template<typename Derived>
struct BitRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }

  typedef ChimeraTK::Boolean minimumUserType;
  typedef int32_t rawUserType;

  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nRuntimeErrorCases() { return derived->target.nRuntimeErrorCases(); }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return {{!this->template getRemoteValue<uint64_t>()[0][0]}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    uint64_t v = derived->target.template getRemoteValue<uint64_t>()[0][0];
    uint64_t mask = 1 << derived->bit;
    bool result = v & mask;
    return {{result}};
  }

  void setRemoteValue() {
    derived->target.setRemoteValue();
    if(derived->isPush()) {
      // At the moment only interrupt 6 is used, so we hard code it here. Can be made more flexible if needed.
      exceptionDummyPush->triggerInterrupt(6);
    }
  }

  void setForceRuntimeError(bool enable, size_t caseIndex) { derived->target.setForceRuntimeError(enable, caseIndex); }
};

/********************************************************************************************************************/
/* Now for each register in unifiedTest.xlmap we define a descriptor */

/// Test passing through scalar accessors
struct RegSingleWord : ScalarRegisterDescriptorBase<RegSingleWord> {
  std::string path() { return "/SingleWord"; }

  const uint32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test passing through scalar accessors - use another target. We use the one with the push accessors (target 3 in lmap
/// file)
struct RegSingleWordB : ScalarRegisterDescriptorBase<RegSingleWordB> {
  std::string path() { return "/SingleWord"; }

  const uint32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyPush.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test passing through push-type scalar accessors
struct RegSingleWord_push : ScalarRegisterDescriptorBase<RegSingleWord_push> {
  std::string path() { return "/SingleWord_push"; }
  bool isPush() { return true; }
  bool isWriteable() {
    std::cout << "Warning: Writing test for /SingleWord_push has been disabled due to missing support in the dummy."
              << std::endl;
    return false;
  }

  const uint32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyPush.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test passing through 1D array accessors
struct RegFullArea : OneDRegisterDescriptorBase<RegFullArea> {
  std::string path() { return "/FullArea"; }

  const int32_t increment = 7;
  size_t nElementsPerChannel() { return 0x400; }

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/ADC.AREA_DMAABLE"};
};

/// Test passing through partial array accessors
struct RegPartOfArea : OneDRegisterDescriptorBase<RegPartOfArea> {
  std::string path() { return "/PartOfArea"; }

  const int32_t increment = 11;
  size_t nElementsPerChannel() { return 20; }
  size_t myOffset() { return 10; }

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/ADC.AREA_DMAABLE"};
};

/// Test channel accessor
struct RegChannel3 : ChannelRegisterDescriptorBase<RegChannel3> {
  std::string path() { return "/Channel3"; }

  const int32_t increment = 17;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{3};

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;

  // Multiplexed 2d accessors don't have access mode raw
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummyMuxed.get(), "TEST", "NODMA"};
};

/// Test channel accessors
struct RegChannel4_push : ChannelRegisterDescriptorBase<RegChannel4_push> {
  std::string path() { return "/Channel4_push"; }
  bool isPush() { return true; }

  const int32_t increment = 23;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{4};

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;

  // Multiplexed 2d accessors don't have access mode raw
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummyMuxed.get(), "TEST", "NODMA"};
};

/// Test channel accessors
struct RegChannelLast : ChannelRegisterDescriptorBase<RegChannelLast> {
  std::string path() { return "/LastChannelInRegister"; }

  const int32_t increment = 27;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{15};

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;

  // Multiplexed 2d accessors don't have access mode raw
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummyMuxed.get(), "TEST", "NODMA"};
};

/// Test constant accessor
struct RegConstant : ConstantRegisterDescriptorBase<RegConstant> {
  std::string path() { return "/Constant"; }

  size_t nElementsPerChannel() { return 1; }
  const std::vector<int32_t> value{42};

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;
};

/// Test constant accessor
struct RegConstant2 : ConstantRegisterDescriptorBase<RegConstant2> {
  std::string path() { return "/Constant2"; }

  size_t nElementsPerChannel() { return 1; }
  const std::vector<int32_t> value{666};

  typedef int32_t minimumUserType;
  typedef int32_t rawUserType;
};

/// Test variable accessor
struct RegVariable : VariableRegisterDescriptorBase<RegVariable> {
  std::string path() { return "/MyModule/SomeSubmodule/Variable"; }

  const int increment = 43;
  size_t nElementsPerChannel() { return 1; }

  typedef float minimumUserType;
  typedef int32_t rawUserType;
};

/// Test constant accessor with arrays
struct RegArrayConstant : ConstantRegisterDescriptorBase<RegArrayConstant> {
  std::string path() { return "/ArrayConstant"; }

  const std::vector<int32_t> value{1111, 2222, 3333, 4444, 5555};
  size_t nElementsPerChannel() { return 5; }

  typedef float minimumUserType;
  typedef int32_t rawUserType;
};

/// Test variable accessor with arrays
struct RegArrayVariable : VariableRegisterDescriptorBase<RegArrayVariable> {
  std::string path() { return "/ArrayVariable"; }

  const int increment = 121;
  size_t nElementsPerChannel() { return 6; }

  typedef float minimumUserType;
  typedef int32_t rawUserType;
};

/// Test bit accessor with a variable accessor as target
struct RegBit0OfVar : BitRegisterDescriptorBase<RegBit0OfVar> {
  std::string path() { return "/Bit0ofVar"; }

  RegVariable target;
  size_t bit = 0;
};

/// Test bit accessor with a variable accessor as target
struct RegBit3OfVar : BitRegisterDescriptorBase<RegBit3OfVar> {
  std::string path() { return "/Bit3ofVar"; }

  RegVariable target;
  size_t bit = 3;
};

/// Test bit accessor with a real dummy accessor as target
struct RegBit2OfWordFirmware : BitRegisterDescriptorBase<RegBit2OfWordFirmware> {
  std::string path() { return "/Bit2ofWordFirmwareA"; }

  RegSingleWord target;
  size_t bit = 2;
};

/// Test bit accessor with another instance of a real dummy accessor as target
struct RegBit2OfWordFirmwareB : BitRegisterDescriptorBase<RegBit2OfWordFirmwareB> {
  std::string path() { return "/Bit2ofWordFirmwareB"; }

  RegSingleWordB target;
  size_t bit = 2;
  // in order to make our test sensitive to incorrect (bit accessor->device) associations, we need an instance
  // of a bit accessor to device A, same register path, as a fixture
  boost::shared_ptr<NDRegisterAccessor<minimumUserType>> fixAccessorOnA{
      lmapBackend->getRegisterAccessor<minimumUserType>("/Bit2ofWordFirmwareA", 1, 0, AccessModeFlags{})};
};

/// Test bit accessor with a real dummy accessor as target
struct RegBit2OfWordFirmware_push : BitRegisterDescriptorBase<RegBit2OfWordFirmware_push> {
  std::string path() { return "/Bit2ofWordFirmware_push"; }
  bool isPush() { return true; }
  bool isWriteable() {
    std::cout
        << "Warning: Writing test for /Bit2ofWordFirmware_push has been disabled due to missing support in the dummy."
        << std::endl;
    return false;
  }

  RegSingleWordB target;
  size_t bit = 2;
};

/// Test multiply plugin - needs to be done separately for reading and writing (see below)
template<typename Derived>
struct RegSingleWordScaled : ScalarRegisterDescriptorBase<Derived> {
  std::string path() { return "/SingleWord_Scaled"; }

  const double increment = std::exp(1.);

  typedef double minimumUserType;
  typedef uint32_t rawUserType;
  // Mutliply plugin does not support access mode raw
  static constexpr auto capabilities = ScalarRegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyRegisterAccessor<rawUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegSingleWordScaled_R : RegSingleWordScaled<RegSingleWordScaled_R> {
  bool isWriteable() { return false; }
  // turn off the catalogue check. It reports that the register is writeable, which is correct. Writing is just turned
  // off for the test.
  static constexpr auto capabilities = RegSingleWordScaled<RegSingleWordScaled_R>::capabilities.disableTestCatalogue();

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value * 4.2;
  }
};

struct RegSingleWordScaled_W : RegSingleWordScaled<RegSingleWordScaled_W> {
  bool isReadable() { return false; }
  // turn off the catalogue check. It reports that the register is readable, which is correct. Reading is just turned
  // off for the test.
  static constexpr auto capabilities = RegSingleWordScaled<RegSingleWordScaled_W>::capabilities.disableTestCatalogue();

  // the scale plugin applies the same factor in both directions, so we have to inverse it for write tests
  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value / 4.2;
  }
};

struct RegSingleWordScaled_RW : RegSingleWordScaled<RegSingleWordScaled_RW> {
  std::string path() { return "/SingleWord_NotScaled"; }

  // The scale plugin applies the same factor in both directions, so it has to be 1 to make the test pass
  // for both reading and writing.
  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value;
  }
};

/// Test multiply plugin applied twice (just one direction for sake of simplicity)
struct RegSingleWordScaledTwice_push : ScalarRegisterDescriptorBase<RegSingleWordScaledTwice_push> {
  std::string path() { return "/SingleWord_Scaled_Twice_push"; }
  bool isWriteable() { return false; }
  bool isPush() { return true; }

  const double increment = std::exp(3.);

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return 6 * value;
  }

  typedef double minimumUserType;
  typedef minimumUserType rawUserType;
  // Mutliply plugin does not support access mode raw
  static constexpr auto capabilities =
      ScalarRegisterDescriptorBase<RegSingleWordScaledTwice_push>::capabilities.disableTestRawTransfer();
  ChimeraTK::AccessModeFlags supportedFlags() { return {AccessMode::wait_for_new_data}; }
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyPush.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test multiply plugin applied to array (just one direction for sake of simplicity)
struct RegFullAreaScaled : OneDRegisterDescriptorBase<RegFullAreaScaled> {
  std::string path() { return "/FullArea_Scaled"; }
  bool isWriteable() { return false; }
  // Mutliply plugin does not support access mode raw
  //  turn off the catalogue check. It reports that the register is readable, which is correct. Reading is just turned
  //  off for the test.
  static constexpr auto capabilities =
      OneDRegisterDescriptorBase<RegFullAreaScaled>::capabilities.disableTestRawTransfer().disableTestCatalogue();

  const double increment = std::exp(4.);
  size_t nElementsPerChannel() { return 0x400; }

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return 0.5 * value;
  }

  typedef double minimumUserType;
  typedef int32_t rawUserType;
  // Mutliply plugin does not support access mode raw. Capabilities already turned off above.
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/ADC.AREA_DMAABLE"};
};

/// Test force readonly plugin
struct RegWordFirmwareForcedReadOnly : ScalarRegisterDescriptorBase<RegWordFirmwareForcedReadOnly> {
  std::string path() { return "/WordFirmwareForcedReadOnly"; }

  const uint32_t increment = -47;
  bool isWriteable() { return false; }

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test force readonly plugin with wait_for_new_data
struct RegWordFirmwareForcedReadOnly_push : ScalarRegisterDescriptorBase<RegWordFirmwareForcedReadOnly_push> {
  std::string path() { return "/WordFirmwareForcedReadOnly_push"; }
  bool isPush() { return true; }

  const uint32_t increment = -47;
  bool isWriteable() { return false; }

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyPush.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test math plugin - needs to be done separately for reading and writing (see below)
template<typename Derived>
struct RegWordFirmwareWithMath : ScalarRegisterDescriptorBase<Derived> {
  const double increment = 7;

  typedef double minimumUserType;
  typedef uint32_t rawUserType;
  // Math plugin does not support access mode raw
  static constexpr auto capabilities = ScalarRegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyRegisterAccessor<rawUserType> acc{exceptionDummyPush.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegWordFirmwareWithMath_R : RegWordFirmwareWithMath<RegWordFirmwareWithMath_R> {
  std::string path() { return "/WordFirmwareWithMath_r"; }
  bool isWriteable() { return false; }

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value + 2.345;
  }
};

struct RegWordFirmwareWithMath_R_push : RegWordFirmwareWithMath<RegWordFirmwareWithMath_R_push> {
  bool isWriteable() { return false; }
  bool isPush() { return true; }
  std::string path() { return "/WordFirmwareWithMath_push"; }

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value + 2.345;
  }
  // Math plugin does not support access mode raw
  ChimeraTK::AccessModeFlags supportedFlags() { return {AccessMode::wait_for_new_data}; }
};

struct RegWordFirmwareWithMath_W : RegWordFirmwareWithMath<RegWordFirmwareWithMath_W> {
  std::string path() { return "/WordFirmwareWithMath_w"; }
  bool isReadable() { return false; }

  // the math plugin applies the same formula in both directions, so we have to reverse the formula for write tests
  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value - 2.345;
  }
};

/// Test math plugin with real dummy register as parameter (exception handling...)
struct RegWordFirmwareAsParameterInMath : ScalarRegisterDescriptorBase<RegWordFirmwareAsParameterInMath> {
  std::string path() { return "/WordFirmwareAsParameterInMath"; }

  // no write test, since we cannot write into a parameter...
  bool isWriteable() { return false; }

  const double increment = 91;

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return value - 42;
  }

  typedef double minimumUserType;
  typedef minimumUserType rawUserType;
  // Math plugin does not support access mode raw
  static constexpr auto capabilities =
      ScalarRegisterDescriptorBase<RegWordFirmwareAsParameterInMath>::capabilities.disableTestRawTransfer();
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyRegisterAccessor<rawUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/// Test math plugin with push-type parameter. In this test we write to one of the variables which is
/// a parameter to the Math plugin in /VariableAsPushParameterInMath. The result is then observed in the WORD_STATUS
/// register of the target. VariableAsPushParameterInMath is only directly written in the test with the
/// RegVariableAsPushParameterInMath_x definition.
static double RegVariableAsPushParameterInMathBase_lastX;

/* We use a has-a-pattern mixin-like pattern to break the "diamond of death".
 * - There is a convertRawToCooked() per variable (var1, var2, x)
 * - There is a base implementation and a "not written" implementation of generateValue() and getRemoteValue()
 * All combinations of them have to be tested, and each of them has a unique generateValueHook().
 * If we try multiple inveritance this ends up in a diamond of death.
 *
 * Solution: The convertRawToCooked() is not done by inheritance but coming from a RawToCookedProvider, which is a
 * template parameter. Like this RegVariableAsPushParameterInMathBase and RegVariableAsPushParameterInMath_not_written
 * can be used with all conversion functions and there is no code duplication.
 */
template<typename Derived, typename RawToCookedProvider>
struct RegVariableAsPushParameterInMathBase : ScalarRegisterDescriptorBase<Derived> {
  // Test only write direction, as we are writing to the variable parameter in this test
  // Also turn off the catalogue test which would fail because the register actually is readable.
  static constexpr auto capabilities = ScalarRegisterDescriptorBase<Derived>::capabilities.enableTestWriteOnly()
                                           .disableTestRawTransfer()
                                           .disableTestCatalogue();

  // no runtime error test cases, as writes happen to the variable only!
  size_t nRuntimeErrorCases() { return 0; }
  void setForceRuntimeError(bool, size_t) { assert(false); }

  // the test "sees" the variable which supports wait_for_new_data
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }

  template<typename T, typename Traw>
  T convertRawToCooked(Traw value) {
    return RawToCookedProvider::convertRawToCooked_impl(value, lmapBackend);
  }

  typedef double minimumUserType;
  typedef minimumUserType rawUserType; // for this context raw means before the math conversion
  DummyRegisterAccessor<rawUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_STATUS"};
};

// template type UserType and RawType have to be double for the math plugin, so we make that explicit in this helper
// function
struct RawToCookedProvider_Var1 {
  static double convertRawToCooked_impl(double value, boost::shared_ptr<LogicalNameMappingBackend>& lmapBackend) {
    auto variable2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    variable2->read();
    return (value - variable2->accessData(0) * 121 - RegVariableAsPushParameterInMathBase_lastX) / 120;
  }
};

struct RegVariableAsPushParameterInMath_var1
: RegVariableAsPushParameterInMathBase<RegVariableAsPushParameterInMath_var1, RawToCookedProvider_Var1> {
  std::string path() { return "/VariableForMathTest1"; }

  const double increment = 17;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->activateAsyncRead();

    // In addition we have to write the accessor which has the math plugin and the second parameter.
    // Otherwise writing of the parameters will have no effect.
    auto x = lmapBackend->getRegisterAccessor<double>("/RegisterWithVariableAsPushParameterInMath", 0, 0, {});
    x->accessData(0) = RegVariableAsPushParameterInMathBase_lastX;
    x->write();
    auto p2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    p2->read();
    p2->write();
  }
};

template<typename Derived, typename RawToCookedProvider>
struct RegVariableAsPushParameterInMath_not_written
: RegVariableAsPushParameterInMathBase<Derived, RawToCookedProvider> {
  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool /*getRaw */ = false) {
    // raw has been set to true, so we get the value as it is on the device
    registerValueBeforeWrite = getRemoteValue<double>(true)[0][0]; // remember for comparison later

    auto generatedValue =
        RegVariableAsPushParameterInMathBase<Derived, RawToCookedProvider>::template generateValue<UserType>();
    lastGeneratedValue = generatedValue[0][0]; // remember for comparison later
    return generatedValue;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool getRaw = false) {
    // We have to trick the unified test into passing. It expects to see the data it has written.
    // However, the test here is that the data actually has not been written.
    // So we do the real test here, and return what we gave out in generateValue, so the unified test can succeed.

    auto remoteRawValue =
        RegVariableAsPushParameterInMathBase<Derived, RawToCookedProvider>::template getRemoteValue<UserType>(true);

    // Hack:
    // getRemoteValue is used by the generateValue implementation in raw mode to access the device. We still need that.
    // As the register with math pluin does not have a raw value, the unified test will always see the tricked version
    // with the data it expects.
    if(getRaw) {
      return remoteRawValue;
    }

    auto convertedValue = this->derived->template convertRawToCooked<double, double>(registerValueBeforeWrite);
    assert(convertedValue != lastGeneratedValue); // test that the unified test does not accidentally pass because
                                                  // something in generateValue went wrong.
    if(remoteRawValue[0][0] == registerValueBeforeWrite) {
      // test successful. Return what is expected
      return {{lastGeneratedValue}};
    }
    else {
      // print limiter because this function is called many times in a timeout loop due to the multi-threading
      if((lastReportedRemoteValue != remoteRawValue[0][0]) ||
          (lastReportedValueBeforeWrite != registerValueBeforeWrite)) {
        std::cout << "FAILED TEST: Register content altered when it should not have been. (" << remoteRawValue[0][0]
                  << " !=  " << registerValueBeforeWrite << ")" << std::endl;
        lastReportedRemoteValue = remoteRawValue[0][0];
        lastReportedValueBeforeWrite = registerValueBeforeWrite;
      }
      return {{convertedValue}};
    }
  }

  double registerValueBeforeWrite;
  double lastGeneratedValue;
  double lastReportedRemoteValue{0};      // for the print limiter
  double lastReportedValueBeforeWrite{0}; // for the print limiter
};

struct RegVariableAsPushParameterInMath_var1_not_written1
: RegVariableAsPushParameterInMath_not_written<RegVariableAsPushParameterInMath_var1_not_written1,
      RawToCookedProvider_Var1> {
  std::string path() { return "/VariableForMathTest1"; }

  const double increment = 18;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->close();
    lmapBackend->open(); // this test is explicitly for writing after open
    lmapBackend->activateAsyncRead();
    // Only write the accessor, not the second parameter.
    auto x = lmapBackend->getRegisterAccessor<double>("/RegisterWithVariableAsPushParameterInMath", 0, 0, {});
    x->accessData(0) = RegVariableAsPushParameterInMathBase_lastX;
    x->write();
  }
};

struct RegVariableAsPushParameterInMath_var1_not_written2
: RegVariableAsPushParameterInMath_not_written<RegVariableAsPushParameterInMath_var1_not_written2,
      RawToCookedProvider_Var1> {
  std::string path() { return "/VariableForMathTest1"; }

  const double increment = 19;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->close();
    lmapBackend->open(); // this test is explicitly for writing after open
    lmapBackend->activateAsyncRead();
    // Only write the second parameter, not the accessor.
    auto p2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    p2->read();
    p2->write();
  }
};

struct RawToCookedProvider_Var2 {
  static double convertRawToCooked_impl(double value, boost::shared_ptr<LogicalNameMappingBackend>& lmapBackend) {
    auto variable1 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest1", 0, 0, {});
    variable1->read();
    return (value - variable1->accessData(0) * 120 - RegVariableAsPushParameterInMathBase_lastX) / 121;
  }
};

struct RegVariableAsPushParameterInMath_var2
: RegVariableAsPushParameterInMathBase<RegVariableAsPushParameterInMath_var2, RawToCookedProvider_Var2> {
  std::string path() { return "/VariableForMathTest2"; }

  const double increment = 23;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->activateAsyncRead();

    // In addition we have to write the accessor which has the math plugin and the first parameter.
    // Otherwise writing of the parameters will have no effect.
    auto x = lmapBackend->getRegisterAccessor<double>("/RegisterWithVariableAsPushParameterInMath", 0, 0, {});
    x->accessData(0) = RegVariableAsPushParameterInMathBase_lastX;
    x->write();
    auto p1 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest1", 0, 0, {});
    p1->read();
    p1->write();
  }
};

struct RawToCookedProvider_x {
  static double convertRawToCooked_impl(double value, boost::shared_ptr<LogicalNameMappingBackend>& lmapBackend) {
    auto variable1 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest1", 0, 0, {});
    variable1->read();
    auto variable2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    variable2->read();
    return value - variable1->accessData(0) * 120 - variable2->accessData(0) * 121;
  }
};

// This is the actual register that is "decoreated" with the math plugin (the x in the formula)
struct RegVariableAsPushParameterInMath_x
: RegVariableAsPushParameterInMathBase<RegVariableAsPushParameterInMath_x, RawToCookedProvider_x> {
  std::string path() { return "/RegisterWithVariableAsPushParameterInMath"; }

  const double increment = 42;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>& v) {
    // Note: This in particular is a hack, since we have no guarantee that this gets actually written!
    // FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    RegVariableAsPushParameterInMathBase_lastX = v[0];
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->activateAsyncRead();

    // In addition we have to write the two parameters. Otherwise writing must have no effect.
    auto p1 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest1", 0, 0, {});
    p1->read();
    p1->write();
    auto p2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    p2->read();
    p2->write();
  }
};

struct RegVariableAsPushParameterInMath_x_not_written1
: RegVariableAsPushParameterInMath_not_written<RegVariableAsPushParameterInMath_x_not_written1, RawToCookedProvider_x> {
  std::string path() { return "/RegisterWithVariableAsPushParameterInMath"; }

  const double increment = 43;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>& v) {
    // Note: This in particular is a hack, since we have no guarantee that this gets actually written!
    // FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    RegVariableAsPushParameterInMathBase_lastX = v[0];
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->close();
    lmapBackend->open(); // this test is explicitly for writing after open
    lmapBackend->activateAsyncRead();

    auto p1 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest1", 0, 0, {});
    p1->read();
    p1->write();
    // don't write p2
  }
};

struct RegVariableAsPushParameterInMath_x_not_written2
: RegVariableAsPushParameterInMath_not_written<RegVariableAsPushParameterInMath_x_not_written2, RawToCookedProvider_x> {
  std::string path() { return "/RegisterWithVariableAsPushParameterInMath"; }

  const double increment = 44;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>& v) {
    // Note: This in particular is a hack, since we have no guarantee that this gets actually written!
    // FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    RegVariableAsPushParameterInMathBase_lastX = v[0];
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->close();
    lmapBackend->open(); // this test is explicitly for writing after open
    lmapBackend->activateAsyncRead();

    auto p2 = lmapBackend->getRegisterAccessor<double>("/VariableForMathTest2", 0, 0, {});
    p2->read();
    p2->write();
  }
};

// template type UserType and RawType have to be double for the math plugin, so we make that explicit in this helper
// function
struct RawToCookedProvider_BitWithMath {
  static constexpr double theOffset = 10;

  static double convertRawToCooked_impl(double value, boost::shared_ptr<LogicalNameMappingBackend>&) {
    return ((uint32_t(value) >> 3) & 1) + theOffset;
  }
};

struct RegRedirectedBitWithMath
: RegVariableAsPushParameterInMathBase<RegRedirectedBitWithMath, RawToCookedProvider_BitWithMath> {
  std::string path() { return "/RedirectedBitWithMath"; }

  const double increment = 8;
  typedef int32_t rawUserType;

  template<typename UserType>
  void generateValueHook(std::vector<UserType>&) {
    // this is a bit a hack: we know that the test has to generate a value before writing, so we can activate
    // async read here which is required for the test to be successful. The assumption is that generateValue is not
    // called before the device is open... FIXME: Better introduce a proper pre-write hook in the UnifiedBackendTest!
    lmapBackend->activateAsyncRead();

    // In addition we have to write the accessor which has the math plugin.
    // Otherwise writing of the parameters will have no effect.
    auto x = lmapBackend->getRegisterAccessor<double>("/RedirectedBitWithMath_helper", 0, 0, {});
    x->accessData(0) = RawToCookedProvider_BitWithMath::theOffset;
    x->write();
  }
};

/// Test monostable trigger plugin (rather minimal test, needs extension!)
struct RegMonostableTrigger : ScalarRegisterDescriptorBase<RegMonostableTrigger> {
  std::string path() { return "/MonostableTrigger"; }

  // Note: the test is rather trivial and does not cover much apart from exception handling, since it requires a special
  // dummy to test the intermediate value.

  bool isReadable() { return false; }

  uint32_t increment = 0; // unused but required to be present

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return {{0}};
  }

  // Conceptually the monostable trigger is of data type void. The input value
  // is not written anywhere. To fulfill the requirements of the test, just return what
  // was generated so the comparison succeeds.
  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool /*getRaw*/ = false) {
    return generateValue<UserType>();
  }

  // FIXME: This is Boolean until the UnifiedTest is modified to support Void correctly
  typedef ChimeraTK::Boolean minimumUserType;
  typedef minimumUserType rawUserType;

  // Mutliply plugin does not support access mode raw
  static constexpr auto capabilities =
      ScalarRegisterDescriptorBase<RegMonostableTrigger>::capabilities.disableTestRawTransfer();
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_STATUS"};
};

// Base descriptor for bit accessors
template<typename Derived>
struct RegBitRangeDescriptor : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;
  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities.disableTestRawTransfer();

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }

  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nRuntimeErrorCases() { return derived->target.nRuntimeErrorCases(); }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    return derived->target.template generateValue<UserType>();
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    uint64_t v = derived->target.template getRemoteValue<uint64_t>()[0][0];
    uint64_t mask = ((1 << derived->width) - 1) << derived->shift;
    UserType result = (v & mask) >> derived->shift;
    return {{result}};
  }

  void setRemoteValue() { derived->target.setRemoteValue(); }

  void setForceRuntimeError(bool enable, size_t caseIndex) { derived->target.setForceRuntimeError(enable, caseIndex); }
};

struct BitRangeAccessorTarget : ScalarRegisterDescriptorBase<BitRangeAccessorTarget> {
  std::string path() { return "/BOARD.WORD_FIRMWARE"; }

  const uint32_t increment = 0x1313'2131;

  using minimumUserType = uint32_t;
  using rawUserType = int32_t;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummyLikeMtcadummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegLowerHalfOfFirmware : RegBitRangeDescriptor<RegLowerHalfOfFirmware> {
  std::string path() { return "/BitRangeLower"; }

  using minimumUserType = int8_t;

  uint16_t width = 8;
  uint16_t shift = 8;

  BitRangeAccessorTarget target;
};

struct RegUpperHalfOfFirmware : RegBitRangeDescriptor<RegUpperHalfOfFirmware> {
  std::string path() { return "/BitRangeUpper"; }

  using minimumUserType = int16_t;

  uint16_t width = 16;
  uint16_t shift = 16;

  BitRangeAccessorTarget target;
};

struct Reg9BitsInChar : RegBitRangeDescriptor<Reg9BitsInChar> {
  std::string path() { return "/BitRangeMiddle"; }

  using minimumUserType = int8_t;

  uint16_t width = 9;
  uint16_t shift = 4;

  BitRangeAccessorTarget target;
};

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string dummyCdd = "(ExceptionDummy?map=mtcadummy.map)";
  std::string muxedDummyCdd = "(ExceptionDummy?map=muxedDataAcessor.map)";
  std::string pushDummyCdd = "(ExceptionDummy?map=mtcadummyB.map)";
  std::string lmapCdd = "(logicalNameMap?map=unifiedTest.xlmap&target=" + dummyCdd + "&target2=" + muxedDummyCdd +
      "&target3=" + pushDummyCdd + ")";
  exceptionDummyLikeMtcadummy =
      boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummyCdd));
  exceptionDummyMuxed =
      boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(muxedDummyCdd));
  // needed for a test that redirected bit goes to right target device
  exceptionDummyPush =
      boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(pushDummyCdd));
  lmapBackend =
      boost::dynamic_pointer_cast<LogicalNameMappingBackend>(BackendFactory::getInstance().createBackend(lmapCdd));

  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegSingleWord>()
      .addRegister<RegSingleWord_push>()
      .addRegister<RegFullArea>()
      .addRegister<RegPartOfArea>()
      .addRegister<RegChannel3>()
      .addRegister<RegChannel4_push>()
      .addRegister<RegChannelLast>()
      .addRegister<RegConstant>()
      .addRegister<RegConstant2>()
      .addRegister<RegVariable>()
      .addRegister<RegArrayConstant>()
      .addRegister<RegArrayVariable>()
      .addRegister<RegBit0OfVar>()
      .addRegister<RegBit3OfVar>()
      .addRegister<RegBit2OfWordFirmware>()
      .addRegister<RegBit2OfWordFirmwareB>()
      .addRegister<RegBit2OfWordFirmware_push>()
      .addRegister<RegSingleWordScaled_R>()
      .addRegister<RegSingleWordScaled_W>()
      .addRegister<RegSingleWordScaled_RW>()
      .addRegister<RegSingleWordScaledTwice_push>()
      .addRegister<RegFullAreaScaled>()
      .addRegister<RegWordFirmwareForcedReadOnly>()
      .addRegister<RegWordFirmwareForcedReadOnly_push>()
      .addRegister<RegWordFirmwareWithMath_R>()
      .addRegister<RegWordFirmwareWithMath_R_push>()
      .addRegister<RegWordFirmwareWithMath_W>()
      .addRegister<RegWordFirmwareAsParameterInMath>()
      .addRegister<RegVariableAsPushParameterInMath_var1>()
      .addRegister<RegVariableAsPushParameterInMath_var1_not_written1>()
      .addRegister<RegVariableAsPushParameterInMath_var1_not_written2>()
      .addRegister<RegVariableAsPushParameterInMath_var2>()
      .addRegister<RegVariableAsPushParameterInMath_x>()
      .addRegister<RegVariableAsPushParameterInMath_x_not_written1>()
      .addRegister<RegVariableAsPushParameterInMath_x_not_written2>()
      .addRegister<RegRedirectedBitWithMath>()
      .addRegister<RegMonostableTrigger>()
      .addRegister<RegLowerHalfOfFirmware>()
      .addRegister<RegUpperHalfOfFirmware>()
      .addRegister<Reg9BitsInChar>()
      .runTests(lmapCdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
