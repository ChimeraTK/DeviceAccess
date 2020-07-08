#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BufferingRegisterAccessor.h"
#include "Device.h"
#include "TransferGroup.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "DummyRegisterAccessor.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static boost::shared_ptr<ExceptionDummy> exceptionDummy, exceptionDummy2;
static boost::shared_ptr<LogicalNameMappingBackend> lmapBackend;

template<typename Derived>
struct RegisterDescriptorBase {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  bool testAsyncReadInconsistency() { return false; }
  size_t nRuntimeErrorCases() { return 1; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
  }

  Derived* derived{static_cast<Derived*>(this)};

  [[noreturn]] void setForceDataLossWrite(bool) { assert(false); }

  [[noreturn]] void forceAsyncReadInconsistency() { assert(false); }
};

template<typename Derived>
struct ChannelRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

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
  }
};

template<typename Derived>
struct OneDRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

  size_t nChannels() { return 1; }

  size_t myOffset() { return 0; }

  // T is always minimumUserType, but C++ doesn't allow to use Derived::minimumUserType here (circular dependency)
  template<typename T>
  T convertRawToCooked(T value) {
    return value;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool getRaw = false) {
    std::vector<UserType> v;
    typedef typename Derived::rawUserType Traw;
    typedef typename Derived::minimumUserType T;
    auto cv = derived->template getRemoteValue<Traw>(true)[0];
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      Traw e = cv[i + derived->myOffset()] + derived->increment * (static_cast<T>(i) + 1);
      if(!getRaw) {
        v.push_back(derived->template convertRawToCooked<T>(e));
      }
      else {
        v.push_back(static_cast<T>(e));
      }
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool getRaw = false) {
    std::vector<UserType> v;
    typedef typename Derived::rawUserType Traw;
    typedef typename Derived::minimumUserType T;
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      Traw e = derived->acc[i + derived->myOffset()];
      if(!getRaw) {
        v.push_back(derived->template convertRawToCooked<T>(e));
      }
      else {
        v.push_back(static_cast<T>(e));
      }
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::rawUserType>(true)[0];
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      derived->acc[i + derived->myOffset()] = v[i];
    }
  }
};

template<typename Derived>
struct ScalarRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  size_t nElementsPerChannel() { return 1; }
};

template<typename Derived>
struct ConstantRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

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

template<typename Derived>
struct VariableRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

  size_t nChannels() { return 1; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }

  size_t nRuntimeErrorCases() { return 0; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool = false) {
    auto acc = lmapBackend->getRegisterAccessor<typename Derived::minimumUserType>(derived->path(), 0, 0, {});
    acc->read();
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
    acc->write();
  }

  void setForceRuntimeError(bool, size_t) { assert(false); }
};

template<typename Derived>
struct BitRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }

  typedef uint8_t minimumUserType;
  typedef minimumUserType rawUserType;

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

  void setRemoteValue() { derived->target.setRemoteValue(); }

  void setForceRuntimeError(bool enable, size_t caseIndex) { derived->target.setForceRuntimeError(enable, caseIndex); }
};

/********************************************************************************************************************/

struct RegSingleWord : ScalarRegisterDescriptorBase<RegSingleWord> {
  std::string path() { return "/SingleWord"; }

  const uint32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegFullArea : OneDRegisterDescriptorBase<RegFullArea> {
  std::string path() { return "/FullArea"; }

  const int32_t increment = 7;
  size_t nElementsPerChannel() { return 0x400; }

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/ADC.AREA_DMAABLE"};
};

struct RegPartOfArea : OneDRegisterDescriptorBase<RegPartOfArea> {
  std::string path() { return "/PartOfArea"; }

  const int32_t increment = 11;
  size_t nElementsPerChannel() { return 20; }
  size_t myOffset() { return 10; }

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/ADC.AREA_DMAABLE"};
};

struct RegChannel3 : ChannelRegisterDescriptorBase<RegChannel3> {
  std::string path() { return "/Channel3"; }

  const int32_t increment = 17;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{3};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummy2.get(), "TEST", "NODMA"};
};

struct RegChannel4 : ChannelRegisterDescriptorBase<RegChannel4> {
  std::string path() { return "/Channel4"; }

  const int32_t increment = 23;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{4};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummy2.get(), "TEST", "NODMA"};
};

struct RegChannelLast : ChannelRegisterDescriptorBase<RegChannelLast> {
  std::string path() { return "/LastChannelInRegister"; }

  const int32_t increment = 27;
  size_t nElementsPerChannel() { return 4; }
  const size_t channel{15};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummy2.get(), "TEST", "NODMA"};
};

struct RegConstant : ConstantRegisterDescriptorBase<RegConstant> {
  std::string path() { return "/Constant"; }

  size_t nElementsPerChannel() { return 1; }
  const std::vector<int32_t> value{42};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
};

struct RegConstant2 : ConstantRegisterDescriptorBase<RegConstant2> {
  std::string path() { return "/Constant2"; }

  size_t nElementsPerChannel() { return 1; }
  const std::vector<int32_t> value{666};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
};

template<typename Derived>
struct RegSingleWordScaled : ScalarRegisterDescriptorBase<Derived> {
  std::string path() { return "/SingleWord_Scaled"; }

  const double increment = std::exp(1.);

  typedef double minimumUserType;
  typedef uint32_t rawUserType;
  DummyRegisterAccessor<rawUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegSingleWordScaled_R : RegSingleWordScaled<RegSingleWordScaled_R> {
  bool isWriteable() { return false; }

  template<typename T>
  T convertRawToCooked(T value) {
    return value * 4.2;
  }
};

struct RegSingleWordScaled_W : RegSingleWordScaled<RegSingleWordScaled_W> {
  bool isReadable() { return false; }

  // the scale plugin applies the same factor in both directions, so we have to inverse it for write tests
  template<typename T>
  T convertRawToCooked(T value) {
    return value / 4.2;
  }
};

struct RegSingleWordScaledTwice : ScalarRegisterDescriptorBase<RegSingleWordScaledTwice> {
  std::string path() { return "/SingleWord_Scaled_Twice"; }
  bool isWriteable() { return false; }

  const double increment = std::exp(3.);

  template<typename T>
  T convertRawToCooked(T value) {
    return 6 * value;
  }

  typedef double minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegFullAreaScaled : OneDRegisterDescriptorBase<RegFullAreaScaled> {
  std::string path() { return "/FullArea_Scaled"; }
  bool isWriteable() { return false; }

  const double increment = std::exp(4.);
  size_t nElementsPerChannel() { return 0x400; }

  template<typename T>
  T convertRawToCooked(T value) {
    return 0.5 * value;
  }

  typedef double minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/ADC.AREA_DMAABLE"};
};

struct RegVariable : VariableRegisterDescriptorBase<RegVariable> {
  std::string path() { return "/MyModule/SomeSubmodule/Variable"; }

  const int increment = 43;
  size_t nElementsPerChannel() { return 1; }

  typedef float minimumUserType;
  typedef minimumUserType rawUserType;
};

struct RegArrayConstant : ConstantRegisterDescriptorBase<RegArrayConstant> {
  std::string path() { return "/ArrayConstant"; }

  const std::vector<int32_t> value{1111, 5555, 2222, 4444, 3333};
  size_t nElementsPerChannel() { return 5; }

  typedef float minimumUserType;
  typedef minimumUserType rawUserType;
};

struct RegArrayVariable : ConstantRegisterDescriptorBase<RegArrayVariable> {
  std::string path() { return "/ArrayVariable"; }

  const std::vector<int32_t> value{11, 22, 33, 44, 55, 66};
  size_t nElementsPerChannel() { return 6; }

  typedef float minimumUserType;
  typedef minimumUserType rawUserType;
};

struct RegBit0OfVar : BitRegisterDescriptorBase<RegBit0OfVar> {
  std::string path() { return "/Bit0ofVar"; }

  RegVariable target;
  size_t bit = 0;
};

struct RegBit3OfVar : BitRegisterDescriptorBase<RegBit3OfVar> {
  std::string path() { return "/Bit3ofVar"; }

  RegVariable target;
  size_t bit = 3;
};

struct RegBit2OfWordFirmware : BitRegisterDescriptorBase<RegBit2OfWordFirmware> {
  std::string path() { return "/Bit2ofWordFirmware"; }

  RegSingleWord target;
  size_t bit = 2;
};

struct RegWirdFirmwareForcedReadOnly : ScalarRegisterDescriptorBase<RegWirdFirmwareForcedReadOnly> {
  std::string path() { return "/WordFirmwareForcedReadOnly"; }

  const uint32_t increment = -47;
  bool isWriteable() { return false; }

  typedef uint32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

template<typename Derived>
struct RegWordFirmwareWithMath : ScalarRegisterDescriptorBase<Derived> {
  std::string path() { return "/WordFirmwareWithMath"; }

  const double increment = 7;

  typedef double minimumUserType;
  typedef uint32_t rawUserType;
  DummyRegisterAccessor<rawUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

struct RegWordFirmwareWithMath_R : RegWordFirmwareWithMath<RegWordFirmwareWithMath_R> {
  bool isWriteable() { return false; }

  template<typename T>
  T convertRawToCooked(T value) {
    return value + 2.345;
  }
};

struct RegWordFirmwareWithMath_W : RegWordFirmwareWithMath<RegWordFirmwareWithMath_W> {
  bool isReadable() { return false; }

  // the math plugin applies the same formula in both directions, so we have to reverse the formula for write tests
  template<typename T>
  T convertRawToCooked(T value) {
    return value - 2.345;
  }
};

struct RegWordFirmwareAsParameterInMath : ScalarRegisterDescriptorBase<RegWordFirmwareAsParameterInMath> {
  std::string path() { return "/WordFirmwareAsParameterInMath"; }

  // no write test, since we cannot write into a parameter...
  bool isWriteable() { return false; }

  const double increment = 91;

  template<typename T>
  T convertRawToCooked(T value) {
    return value - 42;
  }

  typedef double minimumUserType;
  typedef uint32_t rawUserType;
  DummyRegisterAccessor<rawUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

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

  typedef uint32_t minimumUserType;
  typedef uint32_t rawUserType;
  DummyRegisterAccessor<rawUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_FIRMWARE"};
};

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string dummyCdd = "(ExceptionDummy?map=mtcadummy.map)";
  std::string dummy2Cdd = "(ExceptionDummy?map=muxedDataAcessor.map)";
  std::string lmapCdd = "(logicalNameMap?map=unifiedTest.xlmap&target=" + dummyCdd + "&target2=" + dummy2Cdd + ")";
  exceptionDummy = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummyCdd));
  exceptionDummy2 = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummy2Cdd));
  lmapBackend =
      boost::dynamic_pointer_cast<LogicalNameMappingBackend>(BackendFactory::getInstance().createBackend(lmapCdd));

  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegSingleWord>()
      .addRegister<RegFullArea>()
      .addRegister<RegPartOfArea>()
      //.addRegister<RegChannel3>() // triggers "BUG: Wrong exception type thrown in transfer function!"
      //.addRegister<RegChannel4>() // triggers "BUG: Wrong exception type thrown in transfer function!"
      //.addRegister<RegChannelLast>() // triggers "BUG: Wrong exception type thrown in transfer function!"
      //.addRegister<RegChannelLast>() // triggers "BUG: Wrong exception type thrown in transfer function!"
      .addRegister<RegConstant>()
      .addRegister<RegConstant2>()
      .addRegister<RegSingleWordScaled_R>()
      .addRegister<RegSingleWordScaled_W>()
      .addRegister<RegSingleWordScaledTwice>()
      .addRegister<RegFullAreaScaled>()
      .addRegister<RegVariable>()
      .addRegister<RegArrayConstant>()
      .addRegister<RegArrayVariable>()
      .addRegister<RegBit0OfVar>()
      .addRegister<RegBit3OfVar>()
      //.addRegister<RegBit2OfWordFirmware>() // throws wrong exception type, needs investigation...
      .addRegister<RegWirdFirmwareForcedReadOnly>()
      .addRegister<RegWordFirmwareWithMath_R>()
      .addRegister<RegWordFirmwareWithMath_W>()
      .addRegister<RegWordFirmwareAsParameterInMath>()
      .addRegister<RegMonostableTrigger>()
      .runTests(lmapCdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()