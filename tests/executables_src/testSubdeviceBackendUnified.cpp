#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "DummyRegisterAccessor.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(SubdeviceBackendUnifiedTestSuite)

/*********************************************************************************************************************/

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=SubdeviceTarget.map)");
static auto target = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

template<typename Register>
struct AreaType : Register {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  bool testAsyncReadInconsistency() { return false; }

  DummyRegisterAccessor<uint32_t> acc{target.get(), "APP.0", "THE_AREA"};

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
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      assert(i + this->address() / 4 < 10);
      v.push_back(this->fromRaw(acc[i + this->address() / 4]));
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Register::minimumUserType>()[0];
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      assert(i + this->address() / 4 < 10);
      acc[i + this->address() / 4] = this->toRaw(v[i]);
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    target->throwExceptionRead = enable;
    target->throwExceptionWrite = enable;
  }

  void setForceDataLossWrite(bool) { assert(false); }

  void forceAsyncReadInconsistency() { assert(false); }
};

/*********************************************************************************************************************/

struct MyRegister1 {
  std::string path() { return "/APP.0/MY_REGISTER1"; }
  size_t nElementsPerChannel() { return 1; }
  size_t address() { return 0; }
  uint32_t toRaw(uint32_t v) { return v; }
  uint32_t fromRaw(uint32_t v) { return v; }
  uint32_t limitGenerated(uint32_t e) { return e; }
  uint32_t increment = 7;
  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
};

/*********************************************************************************************************************/

struct MyArea1 {
  std::string path() { return "/APP.0/MY_AREA1"; }
  size_t nElementsPerChannel() { return 6; }
  size_t address() { return 8; }
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
  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyRegister1>>().addRegister<AreaType<MyArea1>>().runTests(
      "(subdevice?type=area&device=" + cdd + "&area=APP.0.THE_AREA&map=Subdevice.map)");
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
