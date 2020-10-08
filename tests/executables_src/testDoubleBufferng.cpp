#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendUnifiedTestSuite)

/**********************************************************************************************************************/

//static std::string dbdevice("(ExceptionDummy:1?map=DoubleBuffer.map)");
static std::string db("(logicalNameMap?map=doubleBuffer.xlmap)");
static auto target = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(db));
//static auto device = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dbdevice));
static boost::shared_ptr<ExceptionDummy> exceptionDummy;
static boost::shared_ptr<LogicalNameMappingBackend> lmapBackend;
/**********************************************************************************************************************/

template<typename Register>
struct AreaType : Register {
  Register* derived{static_cast<Register*>(this)};

  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 0; }

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

  DummyRegisterAccessor<uint32_t> acc{target.get(), "/doubleBuffer", "doubleBuffer"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      assert(i + this->address() / 4 < 10);
      typename Register::minimumUserType e = acc[i + this->address() / 4] + this->increment * (i + 1);
      v.push_back(this->limitGenerated(e));
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool = false) {
    // For Variables we don't have a backdoor. We have to use the normal read and write
    // functions which are good enough. It seems like a self consistency test, but all
    // functionality the variable has to provide is that I can write something, and
    // read it back, which is tested with it.

    // We might have to open the backend to perform the operation. We have to remember
    // that we did so and close it again it we did. Some tests require the backend to be closed.
    bool backendWasOpened = lmapBackend->isOpen();
    if(!backendWasOpened) {
      lmapBackend->open();
    }
    auto acc = lmapBackend->getRegisterAccessor<typename Register::minimumUserType>(derived->path(), 0, 0, {});
    acc->read();
    if(!backendWasOpened) {
      lmapBackend->close();
    }
    std::vector<UserType> v;
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      v.push_back(acc->accessData(k));
    }
    return {v};
  }

  void setRemoteValue() {
    auto acc = lmapBackend->getRegisterAccessor<typename Register::minimumUserType>(derived->path(), 0, 0, {});
    auto v = getRemoteValue<typename Register::minimumUserType>()[0];
//    auto v = derived->template generateValue<typename Register::minimumUserType>()[0];
    for(size_t k = 0; k < derived->nElementsPerChannel(); ++k) {
      acc->accessData(k) = v[k];
    }
    bool backendWasOpened = lmapBackend->isOpen();
    if(!backendWasOpened) {
      lmapBackend->open();
    }
    acc->write();
    if(!backendWasOpened) {
      lmapBackend->close();
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
//    target->throwExceptionRead = enable;
//    target->throwExceptionWrite = enable;
    assert(false);
  }
};

/*********************************************************************************************************************/

struct MyArea1 {
  std::string path() { return "/doubleBuffer"; }
  size_t nElementsPerChannel() { return 10; }
  size_t address() { return 20; }
  uint32_t toRaw(float v) { return v * 65536.F; }
  float fromRaw(uint32_t v) { return v / 65536.F; }
  float limitGenerated(float e) {
    while(e > 32768.F) e -= 65535.F;
    while(e < -32767.F) e += 65535.F;
    return e;
  }
  float increment = 666. / 65536.;
  typedef float minimumUserType;
  typedef int32_t rawUserType;
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUnified) {
  // test area type
  std::string dummy = "(ExceptionDummy?map=DoubleBuffer.map)";
  std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + dummy + ")";
  exceptionDummy = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(dummy));


  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyArea1>>().runTests(lmap);


}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
