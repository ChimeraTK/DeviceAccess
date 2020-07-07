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
    auto cv = derived->template getRemoteValue<typename Derived::minimumUserType>()[0];
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      typename Derived::minimumUserType e = cv[i + derived->myOffset()] + derived->increment * (i + 1);
      if(!getRaw) e = derived->convertRawToCooked(e);
      v.push_back(e);
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < derived->nElementsPerChannel(); ++i) {
      v.push_back(derived->template convertRawToCooked<typename Derived::minimumUserType>(
          derived->acc[i + derived->myOffset()]));
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::minimumUserType>(true)[0];
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
  std::vector<std::vector<UserType>> getRemoteValue() {
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
/********************************************************************************************************************/

struct RegSingleWord : ScalarRegisterDescriptorBase<RegSingleWord> {
  std::string path() { return "/SingleWord"; }

  const int32_t increment = 3;

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_USER"};
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

struct RegSingleWordScaled : ScalarRegisterDescriptorBase<RegSingleWordScaled> {
  std::string path() { return "/SingleWord_Scaled"; }

  const float increment = std::exp(29.F);

  template<typename T>
  T convertRawToCooked(T value) {
    return 4.2 * value;
  }

  typedef float minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_USER"};
};

struct RegSingleWordScaledTwice : ScalarRegisterDescriptorBase<RegSingleWordScaledTwice> {
  std::string path() { return "/SingleWord_Scaled_Twice"; }

  const float increment = std::exp(31.F);

  template<typename T>
  T convertRawToCooked(T value) {
    return 6 * value;
  }

  typedef float minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_USER"};
};

struct RegFullAreaScaled : OneDRegisterDescriptorBase<RegFullAreaScaled> {
  std::string path() { return "/FullArea_Scaled"; }

  const float increment = std::exp(37.F);
  size_t nElementsPerChannel() { return 0x400; }

  template<typename T>
  T convertRawToCooked(T value) {
    return 0.5 * value;
  }

  typedef float minimumUserType;
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
/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string dummyCdd = "(ExceptionDummy?map=mtcadummy.map)";
  std::string dummy2Cdd = "(ExceptionDummy?map=muxedDataAcessor.map)";
  std::string lmapCdd = "(logicalNameMap?map=unifiedTest.xlmap&target=" + dummyCdd + "&target2=" + dummy2Cdd + ")";
  exceptionDummy = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummyCdd));
  exceptionDummy2 = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummy2Cdd));
  lmapBackend =
      boost::dynamic_pointer_cast<LogicalNameMappingBackend>(BackendFactory::getInstance().createBackend(lmapCdd));
  auto ubt = ChimeraTK::UnifiedBackendTest<>()
                 .addRegister<RegSingleWord>()
                 .addRegister<RegFullArea>()
                 .addRegister<RegPartOfArea>()
                 //.addRegister<RegChannel3>()
                 //.addRegister<RegChannel4>()
                 //.addRegister<RegChannelLast>()
                 //.addRegister<RegChannelLast>()
                 .addRegister<RegConstant>()
                 .addRegister<RegConstant2>()
                 .addRegister<RegSingleWordScaled>()
                 .addRegister<RegSingleWordScaledTwice>()
                 .addRegister<RegFullAreaScaled>()
                 .addRegister<RegVariable>();
  ubt.runTests(lmapCdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
