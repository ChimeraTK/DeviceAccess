#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendUnifiedTestSuite)

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

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

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
};

/**********************************************************************************************************************/

struct StaticCore {
  StaticCore() {
    assert(target != nullptr);
    data.setWriteCallback([this] { this->writeCallback(); });
    data.setReadCallback([this] { this->readCallback(); });
  }

  DummyRegisterAccessor<uint32_t> address{target.get(), "APP.1", "ADDRESS"};
  DummyRegisterAccessor<uint32_t> data{target.get(), "APP.1", "DATA"};
  DummyRegisterAccessor<uint32_t> status{target.get(), "APP.1", "STATUS"};
  size_t lastAddress{32};
  std::vector<uint32_t> currentValue{std::vector<uint32_t>(lastAddress)};

  bool useStatus{true};

  void readCallback() {
    std::cout << "******   Hallo!" << std::endl;
  }

  void writeCallback() {
    if(useStatus) status = 1;
    BOOST_REQUIRE(address <= lastAddress);
    currentValue[address] = data;
    usleep(1234);
    if(useStatus) status = 0;
  }
};
static StaticCore core;

/**********************************************************************************************************************/

template<typename Register>
struct Regs3Type : Register {
  bool isWriteable() { return true; }
  bool isReadable() { return false; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      typename Register::minimumUserType e =
          this->fromRaw(core.currentValue[i * 4 + this->address()]) + this->increment * (i + 1);
      v.push_back(this->limitGenerated(e));
    }
    return {v};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    std::vector<UserType> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      v.push_back(this->fromRaw(core.currentValue[i * 4 + this->address()]));
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Register::minimumUserType>()[0];
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      core.currentValue[i * 4 + this->address()] = this->toRaw(v[i]);
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    target->throwExceptionRead = enable;
    target->throwExceptionWrite = enable;
  }
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
  // test area type
  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyRegister1>>().addRegister<AreaType<MyArea1>>().runTests(
      "(subdevice?type=area&device=" + cdd + "&area=APP.0.THE_AREA&map=Subdevice.map)");

  // test 3regs type
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MyRegister1>>().addRegister<Regs3Type<MyArea1>>().runTests(
      "(subdevice?type=3regs&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.1.DATA&status=APP.1.STATUS&map=Subdevice.map)");

  // test 2regs type
  core.useStatus = false;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MyRegister1>>().addRegister<Regs3Type<MyArea1>>().runTests(
      "(subdevice?type=2regs&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.1.DATA&sleep=1000000&map=Subdevice.map)");
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
