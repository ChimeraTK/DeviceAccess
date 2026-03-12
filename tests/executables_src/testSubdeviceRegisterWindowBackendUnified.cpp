// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(SubdeviceBackendUnifiedTestSuite)

/**********************************************************************************************************************/

static std::string cdd("(ExceptionDummy:1?map=SubdeviceTarget.map)");

static auto target = boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

struct StaticCore {
  StaticCore() {
    assert(target != nullptr);
    data.setWriteCallback([this] { this->writeCallback(); });
    area.setWriteCallback([this] { this->writeCallback(); });
    readRequest.setWriteCallback([this] { this->readCallback(); });
  }

  DummyRegisterAccessor<uint32_t> transferAddress{target.get(), "APP.1", "ADDRESS"};
  DummyRegisterAccessor<uint32_t> data{target.get(), "APP.1", "DATA"};
  DummyRegisterAccessor<uint32_t> area{target.get(), "APP.0", "THE_AREA"};
  DummyRegisterAccessor<uint32_t> status{target.get(), "APP.1", "STATUS"};
  DummyRegisterAccessor<uint32_t> readData{target.get(), "APP.REG_WIN", "READOUT_SINGLE"};
  DummyRegisterAccessor<uint32_t> readArea{target.get(), "APP.REG_WIN", "READOUT_AREA"};
  DummyRegisterAccessor<uint32_t> readRequest{target.get(), "APP.REG_WIN", "READ_REQUEST"};
  DummyRegisterAccessor<uint32_t> chipSelect{target.get(), "APP.REG_WIN", "CHIP_SELECT"};

  // last address must be able to hold the full transfer width of the last transfer, even of the described register end earlier.
  static constexpr uint32_t lastAddress{140}; // in bytes. End of APP.0.SIXTYFOUR_BIT is a complete transfer.
  std::vector<std::byte> currentValue{std::vector<std::byte>(lastAddress)};
  static constexpr size_t areaSize{10};
  bool useStatus{true};
  bool useArea{false};

  size_t chipId{0};
  size_t transferSize{4}; // in bytes

  void writeCallback() {
    if(chipSelect != chipId) {
      std::cout << "Error in writeCallback: chipSelect is " << chipSelect << ", expected " << chipId << std::endl;
      return;
    }

    if(useStatus) {
      status = 1;
    }
    if(transferAddress * transferSize >=
        lastAddress) { // the transfer address is in size of the transfer (up to 4 bytes)
      std::cout << "Error: address (" << transferAddress << "*" << transferSize << ") >= lastAddress (" << lastAddress
                << ")!" << std::endl;
    }

    BOOST_REQUIRE(transferAddress * transferSize < lastAddress);
    if(!useArea) {
      uint32_t theData = data;
      memcpy(&(currentValue[transferAddress * transferSize]), &theData, transferSize);
    }
    else {
      assert(area.getNumberOfElements() == areaSize);
      BOOST_REQUIRE((transferAddress + 1) * transferSize * areaSize <= lastAddress);
      for(size_t i = 0; i < areaSize; ++i) {
        uint32_t theData = area[i];
        memcpy(&(currentValue[(transferAddress * areaSize + i) * transferSize]), &theData, transferSize);
      }
    }
    usleep(432);
    if(useStatus) {
      status = 0;
    }
  }

  void readCallback() {
    if(chipSelect != chipId) {
      std::cout << "Error in readCallback: chipSelect is " << chipSelect << ", expected " << chipId << std::endl;
      return;
    }

    status = 1;
    if(transferAddress * transferSize >= lastAddress) {
      std::cout << "Error: address (" << transferAddress << "*" << transferSize << ") >= lastAddress (" << lastAddress
                << ")!" << std::endl;
    }

    BOOST_REQUIRE(transferAddress * transferSize < lastAddress);
    if(!useArea) {
      uint32_t theData{0};
      memcpy(&theData, &(currentValue[transferAddress * transferSize]), transferSize);
      readData = theData;
    }
    else {
      assert(area.getNumberOfElements() == areaSize);
      BOOST_REQUIRE((transferAddress + 1) * transferSize * areaSize <= lastAddress);
      for(size_t i = 0; i < areaSize; ++i) {
        uint32_t theData{0};
        memcpy(&theData, &(currentValue[(transferAddress * areaSize + i) * transferSize]), transferSize);
        readArea[i] = theData;
      }
    }
    usleep(432);
    status = 0;
  }
};
static StaticCore core;

/**********************************************************************************************************************/

template<typename Register>
struct RegWindowType : Register {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .enableTestRawTransfer()
                                           .disableTestPartialAccessor();

  using RegisterRawType = typename Register::rawUserType;

  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool raw = false) {
    std::vector<Type> v;

    assert(this->address() + this->nElementsPerChannel() * sizeof(RegisterRawType) <= core.lastAddress);
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      RegisterRawType cv;
      auto* bufferStart = core.currentValue.data();
      memcpy(&cv, bufferStart + i * sizeof(RegisterRawType) + this->address(), sizeof(RegisterRawType));

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

    assert(this->address() + this->nElementsPerChannel() * sizeof(RegisterRawType) <= core.lastAddress);
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      RegisterRawType rawValue;
      auto* bufferStart = std::bit_cast<std::byte*>(core.currentValue.data());
      memcpy(&rawValue, bufferStart + i * sizeof(RegisterRawType) + this->address(), sizeof(RegisterRawType));
      v.push_back(raw ? rawValue : this->fromRaw(rawValue));
    }
    return {v};
  }

  void setRemoteValue() {
    auto v = generateValue<typename Register::minimumUserType>()[0];
    auto* bufferStart = std::bit_cast<std::byte*>(core.currentValue.data());

    assert(this->address() + this->nElementsPerChannel() * sizeof(RegisterRawType) <= core.lastAddress);
    for(size_t i = 0; i < this->nElementsPerChannel(); ++i) {
      auto rawVal = this->toRaw(v[i]);
      static_assert(std::is_same_v<decltype(rawVal), RegisterRawType>);
      memcpy(bufferStart + i * sizeof(RegisterRawType) + this->address(), &rawVal, sizeof(RegisterRawType));
    }
  }

  void setForceRuntimeError(bool enable, size_t) {
    target->throwExceptionRead = enable;
    target->throwExceptionWrite = enable;
    target->throwExceptionOpen = enable;
  }
};

/**********************************************************************************************************************/

template<typename Register>
struct Regs3Type : RegWindowType<Register> {
  bool isReadable() { return false; }
};

/**********************************************************************************************************************/

template<typename Register>
struct ReadOnlyType : RegWindowType<Register> {
  bool isWriteable() { return false; }
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
  typedef uint32_t rawUserType;
};

/**********************************************************************************************************************/

struct MyArea1 {
  std::string path() { return "/APP.0/MY_AREA1"; }
  size_t nElementsPerChannel() { return 6; }
  size_t address() { return 8; }
  uint32_t toRaw(double v) { return int32_t(v * 65536.); }
  double fromRaw(uint32_t v) { return std::bit_cast<int32_t>(v) / 65536.; }
  double limitGenerated(double e) {
    while(e > 32768.F) e -= 65535.;
    while(e < -32767.F) e += 65535.;
    return e;
  }
  float increment = 666. / 65536.;
  typedef double minimumUserType;
  typedef uint32_t rawUserType;
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
  typedef uint32_t rawUserType;
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
  typedef uint32_t rawUserType;
};

/**********************************************************************************************************************/

struct EightBit {
  static std::string path() { return "/APP.0/EIGHT_BIT"; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t address() { return 100; }
  static uint8_t toRaw(int8_t v) { return std::bit_cast<uint8_t>(v); }
  static int8_t fromRaw(uint8_t v) { return std::bit_cast<int8_t>(v); }
  static int8_t limitGenerated(int8_t e) { return e; }
  int8_t increment{2};
  using minimumUserType = int8_t;
  using rawUserType = uint8_t;
};

/**********************************************************************************************************************/

struct EightBitArray {
  static std::string path() { return "/APP.0/EIGHT_BIT_ARRAY"; }
  static size_t nElementsPerChannel() { return 8; }
  static size_t address() { return 100; }
  static uint8_t toRaw(int8_t v) { return std::bit_cast<uint8_t>(v); }
  static int8_t fromRaw(uint8_t v) { return std::bit_cast<int8_t>(v); }
  static int8_t limitGenerated(int8_t e) { return e; }
  int8_t increment{3};
  using minimumUserType = int8_t;
  using rawUserType = uint8_t;
};

/**********************************************************************************************************************/

struct SixteenBit {
  static std::string path() { return "/APP.0/SIXTEEN_BIT"; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t address() { return 108; }
  static uint16_t toRaw(int16_t v) { return std::bit_cast<uint16_t>(v); }
  static int16_t fromRaw(uint16_t v) { return std::bit_cast<int16_t>(v); }
  static int16_t limitGenerated(int16_t e) { return e; }
  int16_t increment{0x202};
  using minimumUserType = int16_t;
  using rawUserType = uint16_t;
};

/**********************************************************************************************************************/

struct SixteenBitOff1 {
  static std::string path() { return "/APP.0/SIXTEEN_OFF1"; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t address() { return 109; }
  static uint16_t toRaw(int16_t v) { return std::bit_cast<uint16_t>(v); }
  static int16_t fromRaw(uint16_t v) { return std::bit_cast<int16_t>(v); }
  static int16_t limitGenerated(int16_t e) { return e; }
  int16_t increment{0x303};
  using minimumUserType = int16_t;
  using rawUserType = uint16_t;
};

/**********************************************************************************************************************/

struct SixteenBitOff2 {
  static std::string path() { return "/APP.0/SIXTEEN_OFF2"; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t address() { return 110; }
  static uint16_t toRaw(int16_t v) { return std::bit_cast<uint16_t>(v); }
  static int16_t fromRaw(uint16_t v) { return std::bit_cast<int16_t>(v); }
  static int16_t limitGenerated(int16_t e) { return e; }
  int16_t increment{0x404};
  using minimumUserType = int16_t;
  using rawUserType = uint16_t;
};

/**********************************************************************************************************************/

struct SixteenBitArray {
  static std::string path() { return "/APP.0/SIXTEEN_BIT_ARRAY"; }
  static size_t nElementsPerChannel() { return 8; }
  static size_t address() { return 112; }
  static uint16_t toRaw(int16_t v) { return std::bit_cast<uint16_t>(v); }
  static int16_t fromRaw(uint16_t v) { return std::bit_cast<int16_t>(v); }
  static int16_t limitGenerated(int16_t e) { return e; }
  int16_t increment{0x505};
  using minimumUserType = int16_t;
  using rawUserType = uint16_t;
};

/**********************************************************************************************************************/

struct SixteenBitArrayOff2 {
  static std::string path() { return "/APP.0/SIXTEEN_ARRAY_OFF2"; }
  static size_t nElementsPerChannel() { return 8; }
  static size_t address() { return 114; }
  static uint16_t toRaw(int16_t v) { return std::bit_cast<uint16_t>(v); }
  static int16_t fromRaw(uint16_t v) { return std::bit_cast<int16_t>(v); }
  static int16_t limitGenerated(int16_t e) { return e; }
  int16_t increment{0x606};
  using minimumUserType = int16_t;
  using rawUserType = uint16_t;
};

/**********************************************************************************************************************/

struct SixtyFourBit {
  static std::string path() { return "/APP.0/SIXTYFOUR_BIT"; }
  static size_t nElementsPerChannel() { return 1; }
  static size_t address() { return 132; }
  static uint64_t toRaw(int64_t v) { return std::bit_cast<uint64_t>(v); }
  static int64_t fromRaw(uint64_t v) { return std::bit_cast<int64_t>(v); }
  static int64_t limitGenerated(int64_t e) { return e; }
  int64_t increment{0x600000006};
  using minimumUserType = int64_t;
  using rawUserType = uint64_t;
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnified3Reg) {
  // test 3regs type
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MyRegister1>>().addRegister<Regs3Type<MyArea1>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.1.DATA&status=APP.1.STATUS&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnified2Reg) {
  // test 2regs type
  core.useStatus = false;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MyRegister1>>().addRegister<Regs3Type<MyArea1>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.1.DATA&sleep=1000&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnifiedMuxed3Reg) {
  // test different use case of 3regs mode: multiplexing of an area

  core.useStatus = true;
  core.useArea = true;
  ChimeraTK::UnifiedBackendTest<>().addRegister<Regs3Type<MuxedArea1>>().addRegister<Regs3Type<MuxedArea2>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&data=APP.0.THE_AREA&status=APP.1.STATUS&map=SubdeviceMuxedArea.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnifiedRegWindow) {
  core.chipId = 3;
  core.useArea = false;
  ChimeraTK::UnifiedBackendTest<>().addRegister<RegWindowType<MyRegister1>>().addRegister<RegWindowType<MyArea1>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&writeData=APP.1.DATA&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData=APP."
      "REG_WIN.READOUT_SINGLE&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&chipIndex=3&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnifiedMuxedRegwindow) {
  // multiplexing of an area (whatever that means? Why multiplexing?) with read and write
  core.chipId = 0; // test without the optional chipIndex parameter
  core.useArea = true;
  ChimeraTK::UnifiedBackendTest<>().addRegister<RegWindowType<MuxedArea1>>().addRegister<RegWindowType<MuxedArea2>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&writeData=APP.0.THE_AREA&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData="
      "APP.REG_WIN.READOUT_AREA&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&map=SubdeviceMuxedArea.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestUnifiedReadOnly) {
  core.chipId = 3;
  core.useArea = false;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<ReadOnlyType<MyRegister1>>()
      .addRegister<ReadOnlyType<MyArea1>>()
      .runTests("(subdevice?type=regWindow&device=" + cdd +
          "&address=APP.1.ADDRESS&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData=APP."
          "REG_WIN.READOUT_SINGLE&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&chipIndex=3&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(Test8Bit) {
  core.chipId = 0;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegWindowType<EightBit>>()
      .addRegister<RegWindowType<EightBitArray>>()
      .runTests("(subdevice?type=regWindow&device=" + cdd +
          "&address=APP.1.ADDRESS&writeData=APP.1.DATA&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData="
          "APP.REG_WIN.READOUT_SINGLE&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(Test16Bit) {
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegWindowType<SixteenBit>>()
      .addRegister<RegWindowType<SixteenBitOff1>>()
      .addRegister<RegWindowType<SixteenBitOff2>>()
      .addRegister<RegWindowType<SixteenBitArray>>()
      .addRegister<RegWindowType<SixteenBitArrayOff2>>()
      .runTests("(subdevice?type=regWindow&device=" + cdd +
          "&address=APP.1.ADDRESS&writeData=APP.1.DATA&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData="
          "APP.REG_WIN.READOUT_SINGLE&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(Test64Bit) {
  core.chipId = 0;
  ChimeraTK::UnifiedBackendTest<>().addRegister<RegWindowType<SixtyFourBit>>().runTests(
      "(subdevice?type=regWindow&device=" + cdd +
      "&address=APP.1.ADDRESS&writeData=APP.1.DATA&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_REQUEST&readData="
      "APP.REG_WIN.READOUT_SINGLE&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&map=Subdevice.map)");
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestTransfer8) {
  core.transferSize = 1;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<RegWindowType<EightBit>>()
      .addRegister<RegWindowType<EightBitArray>>()
      .addRegister<RegWindowType<SixteenBit>>()
      .addRegister<RegWindowType<SixteenBitOff1>>()
      .addRegister<RegWindowType<SixteenBitOff2>>()
      .addRegister<RegWindowType<SixteenBitArray>>()
      .addRegister<RegWindowType<SixteenBitArrayOff2>>()
      .addRegister<RegWindowType<MyRegister1>>()
      .addRegister<RegWindowType<MyArea1>>()
      .addRegister<RegWindowType<SixtyFourBit>>()
      .runTests("(subdevice?type=regWindow&device=" + cdd +
          "&address=APP.1.ADDRESS&writeData=APP.1.WRITE_DATA_8BIT&busy=APP.1.STATUS&readRequest=APP.REG_WIN.READ_"
          "REQUEST&readData=APP.REG_WIN.READOUT_8BIT&chipSelectRegister=APP.REG_WIN.CHIP_SELECT&map=Subdevice.map)");
}
/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
