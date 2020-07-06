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

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static boost::shared_ptr<ExceptionDummy> exceptionDummy, exceptionDummy2;

template<typename Derived>
struct RegisterDescriptorBase {
  const bool isWriteable{true};
  const bool isReadable{true};
  const ChimeraTK::AccessModeFlags supportedFlags{ChimeraTK::AccessMode::raw};
  const size_t writeQueueLength{std::numeric_limits<size_t>::max()};
  const bool testAsyncReadInconsistency{false};

  void setForceRuntimeError(bool enable) {
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

  const size_t nChannels{1};
  const bool isWriteable{false};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel; ++k) {
      v.push_back(derived->acc[derived->channel][k] + derived->increment * (k + 1));
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel; ++k) {
      v.push_back(derived->acc[derived->channel][k]);
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::minimumUserType>()[0];
    for(size_t k = 0; k < derived->nElementsPerChannel; ++k) {
      derived->acc[derived->channel][k] = v[k];
    }
  }
};

template<typename Derived>
struct OneDRegisterDescriptorBase : RegisterDescriptorBase<Derived> {
  using RegisterDescriptorBase<Derived>::derived;

  const size_t nChannels{1};

  size_t myOffset() { return 0; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < derived->nElementsPerChannel; ++i) {
      v.push_back(derived->acc[i + derived->myOffset()] + derived->increment * (i + 1));
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < derived->nElementsPerChannel; ++i) {
      v.push_back(derived->acc[i + derived->myOffset()]);
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Derived::minimumUserType>()[0];
    for(size_t i = 0; i < derived->nElementsPerChannel; ++i) {
      derived->acc[i + derived->myOffset()] = v[i];
    }
  }
};

template<typename Derived>
struct ScalarRegisterDescriptorBase : OneDRegisterDescriptorBase<Derived> {
  const size_t nElementsPerChannel{1};
};

/********************************************************************************************************************/

struct RegSingleWord : ScalarRegisterDescriptorBase<RegSingleWord> {
  const std::string path{"/SingleWord"};

  const int32_t increment = 3;

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/BOARD.WORD_USER"};
};

struct RegFullArea : OneDRegisterDescriptorBase<RegFullArea> {
  const std::string path{"/FullArea"};

  const int32_t increment = 7;
  const size_t nElementsPerChannel{0x400};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/ADC.AREA_DMAABLE"};
};

struct RegPartOfArea : OneDRegisterDescriptorBase<RegPartOfArea> {
  const std::string path{"/PartOfArea"};

  const int32_t increment = 11;
  const size_t nElementsPerChannel{20};
  size_t myOffset() { return 10; }

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyRegisterAccessor<minimumUserType> acc{exceptionDummy.get(), "", "/ADC.AREA_DMAABLE"};
};

struct RegChannel3 : ChannelRegisterDescriptorBase<RegChannel3> {
  const std::string path{"/Channel3"};

  const int32_t increment = 17;
  const size_t nElementsPerChannel{4};
  const size_t channel{3};

  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;
  DummyMultiplexedRegisterAccessor<minimumUserType> acc{exceptionDummy2.get(), "TEST", "NODMA"};
};

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string dummyCdd = "(ExceptionDummy?map=mtcadummy.map)";
  std::string dummy2Cdd = "(ExceptionDummy?map=muxedDataAcessor.map)";
  std::string lmapCdd = "(logicalNameMap?map=unifiedTest.xlmap&target=" + dummyCdd + "&target2=" + dummy2Cdd + ")";
  exceptionDummy = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummyCdd));
  exceptionDummy2 = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummy2Cdd));
  auto ubt = ChimeraTK::UnifiedBackendTest<>()
                 .addRegister<RegSingleWord>()
                 .addRegister<RegFullArea>()
                 .addRegister<RegPartOfArea>()
                 .addRegister<RegChannel3>();
  ubt.runTests(lmapCdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
