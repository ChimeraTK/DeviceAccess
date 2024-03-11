// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(SubdeviceBackendUnifiedTestSuite)

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

  static constexpr auto capabilities =
      TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency().enableTestRawTransfer();

  DummyRegisterAccessor<uint32_t> acc{target.get(), "APP.0", "THE_AREA"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool raw = false) {
    std::vector<UserType> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      assert(i + this->address() / 4 < 10);
      typename Register::minimumUserType e = acc[i + this->address() / 4] + this->increment * (i + 1);
      auto limited = this->limitGenerated(e);
      v.push_back(raw ? this->toRaw(limited) : limited);
    }
    return {v};
  }

  // Use the same implementation for raw and cooked.
  // Type can be UserType or RawType
  template<typename Type>
  std::vector<std::vector<Type>> getRemoteValue(bool raw = false) {
    std::vector<Type> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      assert(i + this->address() / 4 < 10);
      auto rawVal = acc[i + this->address() / 4];
      v.push_back((raw ? rawVal : this->fromRaw(rawVal)));
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
    target->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

struct StaticCore {
  StaticCore() {
    assert(target != nullptr);
    data.setWriteCallback([this] { this->writeCallback(); });
    area.setWriteCallback([this] { this->writeCallback(); });
  }

  DummyRegisterAccessor<uint32_t> address{target.get(), "APP.1", "ADDRESS"};
  DummyRegisterAccessor<uint32_t> data{target.get(), "APP.1", "DATA"};
  DummyRegisterAccessor<uint32_t> area{target.get(), "APP.0", "THE_AREA"};
  DummyRegisterAccessor<uint32_t> status{target.get(), "APP.1", "STATUS"};
  size_t lastAddress{32};
  std::vector<uint32_t> currentValue{std::vector<uint32_t>(lastAddress)};
  size_t areaSize{10};
  std::vector<std::vector<uint32_t>> currentAreaValue{lastAddress, std::vector<uint32_t>(areaSize)};

  bool useStatus{true};
  bool useArea{false};

  void writeCallback() {
    if(useStatus) status = 1;
    if(address >= lastAddress) {
      std::cout << "Error: address (" << address << ") >= lastAddress (" << lastAddress << ")!" << std::endl;
    }
    BOOST_REQUIRE(address < lastAddress);
    if(!useArea) {
      currentValue[address] = data;
    }
    else {
      assert(area.getNumberOfElements() == areaSize);
      for(size_t i = 0; i < areaSize; ++i) {
        currentAreaValue[address][i] = area[i];
      }
    }
    usleep(432);
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

  static constexpr auto capabilities =
      TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency().enableTestRawTransfer();

  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool raw = false) {
    std::vector<Type> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      uint32_t cv;
      if(!core.useArea) {
        cv = core.currentValue[i + this->address()];
      }
      else {
        cv = core.currentAreaValue[this->address() + i / core.areaSize][i % core.areaSize];
      }
      // Do the calculation in cooked, and convert back to raw if necessary
      typename Register::minimumUserType e = this->fromRaw(cv) + this->increment * (i + 1);
      auto limited = this->limitGenerated(e);
      v.push_back(raw ? this->toRaw(limited) : limited);
    }
    return {v};
  }

  template<typename Type>
  std::vector<std::vector<Type>> getRemoteValue(bool raw = false) {
    std::vector<Type> v;
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      Type rawValue;
      if(!core.useArea) {
        rawValue = core.currentValue[i + this->address()];
      }
      else {
        rawValue = core.currentAreaValue[this->address() + i / core.areaSize][i % core.areaSize];
      }
      v.push_back(raw ? rawValue : this->fromRaw(rawValue));
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Register::minimumUserType>()[0];
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      if(!core.useArea) {
        core.currentValue[i + this->address()] = this->toRaw(v[i]);
      }
      else {
        core.currentAreaValue[this->address() + i / core.areaSize][i % core.areaSize] = this->toRaw(v[i]);
      }
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    target->throwExceptionRead = enable;
    target->throwExceptionWrite = enable;
    target->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

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

/**********************************************************************************************************************/

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

/**********************************************************************************************************************/

struct MuxedArea1 {
  std::string path() { return "/APP.0/THE_AREA_1"; }
  size_t nElementsPerChannel() { return 10; }
  size_t address() { return 0; }
  uint32_t toRaw(uint32_t v) { return v; }
  uint32_t fromRaw(uint32_t v) { return v; }
  uint32_t limitGenerated(uint32_t e) { return e; }
  uint32_t increment = 17;
  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
};

/**********************************************************************************************************************/

struct MuxedArea2 {
  std::string path() { return "/APP.0/THE_AREA_2"; }
  size_t nElementsPerChannel() { return 25; }
  size_t address() { return 7; }
  uint32_t toRaw(float v) { return v * 65536.F; }
  float fromRaw(uint32_t v) { return v / 65536.F; }
  float limitGenerated(float e) {
    while(e > 32768.F) e -= 65535.F;
    while(e < -32767.F) e += 65535.F;
    return e;
  }
  float increment = 42. / 65536.;
  typedef float minimumUserType;
  typedef int32_t rawUserType;
};

/**********************************************************************************************************************/

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
      "(subdevice?type=2regs&device=" + cdd + "&address=APP.1.ADDRESS&data=APP.1.DATA&sleep=1000&map=Subdevice.map)");

  // test different use case of 3regs mode: multiplexing of an area
  core.useStatus = true;
  core.useArea = true;
  core.lastAddress = 10;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MuxedArea1>>().addRegister<Regs3Type<MuxedArea2>>().runTests(
      "(subdevice?type=3regs&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.0.THE_AREA&status=APP.1.STATUS&map=SubdeviceMuxedArea.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
