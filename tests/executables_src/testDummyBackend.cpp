// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testDummyBackend

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "Exception.h"
#include "parserUtilities.h"

#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>

// FIXME Remove
#include <regex>

#define BOOST_NO_EXCEPTIONS
#include <boost/test/unit_test.hpp>
#undef BOOST_NO_EXCEPTIONS

using namespace boost::unit_test_framework;
namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

#define TEST_MAPPING_FILE "mtcadummy_withoutModules.map"
#define FIRMWARE_REGISTER_STRING "WORD_FIRMWARE"
#define STATUS_REGISTER_STRING "WORD_STATUS"
#define USER_REGISTER_STRING "WORD_USER"
#define CLOCK_MUX_REGISTER_STRING "WORD_CLK_MUX"
#define CLOCK_RESET_REGISTER_STRING "WORD_CLK_RST"
#define READ_ONLY_REGISTER_STRING "WORD_READ_ONLY"

#define EXISTING_DEVICE "(TestableDummy?map=" TEST_MAPPING_FILE ")"
#define NON_EXISTING_DEVICE "DUMMY9"

static BackendFactory& FactoryInstance = BackendFactory::getInstance();

/**
 *  The TestableDummybackend is derived from
 *  DummyBackend to get access to the protected members.
 *  This is done by declaring DummybackendTest as a friend.
 *
 *  FIXME get away from testing implementation details!
 */
class TestableDummyBackend : public DummyBackend {
 public:
  explicit TestableDummyBackend(std::string mapFileName) : DummyBackend(mapFileName) {}
  using DummyBackend::checkSizeIsMultipleOfWordSize;
  using DummyBackend::_barContents;
  using DummyBackend::setReadOnly;
  using DummyBackend::AddressRange;
  using DummyBackend::setWriteCallbackFunction;
  using DummyBackend::writeRegisterWithoutCallback;
  using DummyBackend::isWriteRangeOverlap;
  using DummyBackend::_readOnlyAddresses;
  using DummyBackend::_writeCallbackFunctions;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new TestableDummyBackend(parameters["map"]));
  }
  class BackendRegisterer {
   public:
    BackendRegisterer() {
      std::cout << "TestableDummyBackend::BackendRegisterer: registering backend type TestableDummy" << std::endl;
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "TestableDummy", &TestableDummyBackend::createInstance);
    }
  };
  static BackendRegisterer backendRegisterer;
};
TestableDummyBackend::BackendRegisterer TestableDummyBackend::backendRegisterer;

class Fixture_t {
 public:
  Fixture_t() : a(0), b(0), c(0), _backendInstance() {
    BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
    std::list<std::string> parameters;
    parameters.push_back(std::string(TEST_MAPPING_FILE));
    _dummyBackend = boost::shared_ptr<TestableDummyBackend>(new TestableDummyBackend(TEST_MAPPING_FILE));
  }

  boost::shared_ptr<TestableDummyBackend> _dummyBackend;
  TestableDummyBackend* getBackendInstance();
  friend class DummyBackendTestSuite;

  // stuff for the callback function test
  int a, b, c;
  void increaseA() { ++a; }
  void increaseB() { ++b; }
  void increaseC() { ++c; }
  boost::shared_ptr<ChimeraTK::DeviceBackend> _backendInstance;
};

static Fixture_t f;

/**********************************************************************************************************************/

TestableDummyBackend* Fixture_t::getBackendInstance() {
  if(_backendInstance == nullptr) _backendInstance = FactoryInstance.createBackend(EXISTING_DEVICE);
  _backendInstance->open();
  DeviceBackend* rawBasePointer = _backendInstance.get();
  return (static_cast<TestableDummyBackend*>(rawBasePointer));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCheckSizeIsMultipleOfWordSize) {
  // just some arbitrary numbers to test %4 = 0, 1, 2, 3
  BOOST_CHECK_NO_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(24));

  BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(25), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(26), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(TestableDummyBackend::checkSizeIsMultipleOfWordSize(27), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteSingleWordRegister) {
  // WORD_CLK_RST                      0x00000001    0x00000040    0x00000004    0x00000000       32         0         0
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  uint64_t offset = 0x40;
  uint64_t bar = 0;
  int32_t dataContent = -1;
  // BOOST_CHECK_NO_THROW(dummyBackend->readReg(bar, offset, &dataContent));
  BOOST_CHECK_NO_THROW(dummyBackend->read(bar, offset, &dataContent, 4));
  BOOST_CHECK(dataContent == 0);
  dataContent = 47;
  BOOST_CHECK_NO_THROW(dummyBackend->write(bar, offset, &dataContent, 4));
  dataContent = -1; // make sure the value is really being read
  // no need to test NO_THROW on the same register twice
  // dummyBackend->readReg(offset, &dataContent, bar);
  dummyBackend->read(bar, offset, &dataContent, 4);
  BOOST_CHECK(dataContent == 47);

  // the size as index is invalid, allowed range is 0..size-1 included.
  BOOST_CHECK_THROW(
      dummyBackend->read(
          bar, static_cast<uint64_t>(dummyBackend->_barContents[bar].size() * sizeof(int32_t)), &dataContent, 4),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      dummyBackend->write(
          bar, static_cast<uint64_t>(dummyBackend->_barContents[bar].size() * sizeof(int32_t)), &dataContent, 4),
      ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteMultiWordRegister) {
  // WORD_CLK_MUX                      0x00000004    0x00000020    0x00000010    0x00000000       32         0         0
  TestableDummyBackend* dummyBackend = f.getBackendInstance();

  uint64_t offset = 0x20;
  uint64_t bar = 0;
  size_t sizeInBytes = 0x10;
  size_t sizeInWords = 0x4;
  std::vector<int32_t> dataContent(sizeInWords, -1);

  BOOST_CHECK_NO_THROW(dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes));
  for(std::vector<int32_t>::iterator dataIter = dataContent.begin(); dataIter != dataContent.end(); ++dataIter) {
    std::stringstream errorMessage;
    errorMessage << "*dataIter = " << *dataIter;
    BOOST_CHECK_MESSAGE(*dataIter == 0, errorMessage.str());
  }

  for(unsigned int index = 0; index < dataContent.size(); ++index) {
    dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
  }
  BOOST_CHECK_NO_THROW(dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes));
  // make sure the value is really being read
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);

  // no need to test NO_THROW on the same register twice
  dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);

  // make sure the value is really being read
  for(unsigned int index = 0; index < dataContent.size(); ++index) {
    BOOST_CHECK(dataContent[index] == static_cast<int32_t>((index + 1) * (index + 1)));
  }

  // tests for exceptions
  // 1. base address too large
  BOOST_CHECK_THROW(
      dummyBackend->read(bar, static_cast<uint64_t>(dummyBackend->_barContents[bar].size() * sizeof(int32_t)),
          &(dataContent[0]), sizeInBytes),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      dummyBackend->write(bar, static_cast<uint64_t>(dummyBackend->_barContents[bar].size() * sizeof(int32_t)),
          &(dataContent[0]), sizeInBytes),
      ChimeraTK::logic_error);
  // 2. size too large (works because the target register is not at offfset 0)
  // resize the data vector for this test
  dataContent.resize(dummyBackend->_barContents[bar].size());
  BOOST_CHECK_THROW(
      dummyBackend->read(bar, offset, &(dataContent[0]), dummyBackend->_barContents[bar].size() * sizeof(int32_t)),
      ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      dummyBackend->write(bar, offset, &(dataContent[0]), dummyBackend->_barContents[bar].size() * sizeof(int32_t)),
      ChimeraTK::logic_error);
  // 3. size not multiple of 4
  BOOST_CHECK_THROW(dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes - 1), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes - 1), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

#if 0
BOOST_AUTO_TEST_CASE(testReadDeviceInfo) {
  std::string deviceInfo;
  auto dummyBackend = FactoryInstance.createBackend("DUMMYD0");
  deviceInfo = dummyBackend->readDeviceInfo();
  std::cout << deviceInfo << std::endl;

  // DummyDevice instances created using the factory now deals with absolute
  // paths to the dmap file. We frame an absolute path for comaprison
  std::string absolutePathToMapfile =
      ChimeraTK::parserUtilities::getCurrentWorkingDirectory() + "./" + TEST_MAPPING_FILE;

  BOOST_CHECK(deviceInfo == (std::string("DummyBackend with mapping file ") + absolutePathToMapfile));
}
#endif

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadOnly) {
  TestableDummyBackend* dummyBackend = f.getBackendInstance();

  // WORD_CLK_MUX                      0x00000004    0x00000020    0x00000010    0x00000000       32         0         0
  uint64_t offset = 0x20;
  uint64_t bar = 0;
  size_t sizeInBytes = 0x10;
  size_t sizeInWords = 0x4;
  std::stringstream errorMessage;
  errorMessage << "This register should have 4 words. If you changed your "
                  "mapping you have to adapt the testReadOnly() test.";
  BOOST_REQUIRE_MESSAGE(sizeInWords == 4, errorMessage.str());

  std::vector<int32_t> dataContent(sizeInWords);
  for(unsigned int index = 0; index < dataContent.size(); ++index) {
    dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
  }
  dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes);
  dummyBackend->setReadOnly(bar, offset, 1);

  // the actual test: write 42 to all registers, register 0 must not change, all
  // others have to
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 42);
  dummyBackend->write(bar, offset, &(dataContent[0]), sizeInBytes);
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
  dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);
  BOOST_CHECK(dataContent[0] == 1);
  BOOST_CHECK(dataContent[1] == 42);
  BOOST_CHECK(dataContent[2] == 42);
  BOOST_CHECK(dataContent[3] == 42);
  // also set the last two words to read only. Now only the second word has to
  // change.
  // We use the addressRange interface for it to also cover this.
  TestableDummyBackend::AddressRange lastTwoMuxRegisters(bar, offset + 2 * sizeof(int32_t), 2 * sizeof(int32_t));
  dummyBackend->setReadOnly(lastTwoMuxRegisters);
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 29);
  // also test with single write operations
  for(size_t index = 0; index < sizeInWords; ++index) {
    dummyBackend->write(bar, offset + index * sizeof(int32_t), &dataContent[index], 4);
  }

  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
  dummyBackend->read(bar, offset, &(dataContent[0]), sizeInBytes);
  BOOST_CHECK(dataContent[0] == 1);
  BOOST_CHECK(dataContent[1] == 29);
  BOOST_CHECK(dataContent[2] == 42);
  BOOST_CHECK(dataContent[3] == 42);

  // check that the next register is still writeable (boundary test)
  int32_t originalNextDataWord;
  dummyBackend->read(bar, offset + sizeInBytes, &originalNextDataWord, 4);
  int32_t writeWord = originalNextDataWord + 1;
  dummyBackend->write(bar, offset + sizeInBytes, &writeWord, 4);
  int32_t readbackWord;
  dummyBackend->read(bar, offset + sizeInBytes, &readbackWord, 4);
  BOOST_CHECK(originalNextDataWord + 1 == readbackWord);
}

BOOST_AUTO_TEST_CASE(testWriteCallbackFunctions) {
  // We just require the first bar to be 12 registers long.
  // Everything else would overcomplicate this test. For a real
  // application one would always use register names from mapping,
  // but this is not the purpose of this test.

  // from the previous test we know that adresses 32, 40 and 44 are write only
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  BOOST_REQUIRE(dummyBackend->_barContents[0].size() >= 13);
  f.a = 0;
  f.b = 0;
  f.c = 0;
  dummyBackend->setWriteCallbackFunction(TestableDummyBackend::AddressRange(0, 36, 4), [] { f.increaseA(); });
  dummyBackend->setWriteCallbackFunction(TestableDummyBackend::AddressRange(0, 28, 24), [] { f.increaseB(); });
  dummyBackend->setWriteCallbackFunction(TestableDummyBackend::AddressRange(0, 20, 12), [] { f.increaseC(); });

  // test single writes
  int32_t dataWord(42);
  dummyBackend->write(static_cast<uint64_t>(0), 12, &dataWord, 4); // nothing
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 0);
  BOOST_CHECK(f.c == 0);

  dummyBackend->write(static_cast<uint64_t>(0), 20, &dataWord, 4); // c
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 0);
  BOOST_CHECK(f.c == 1);
  dummyBackend->write(static_cast<uint64_t>(0), 24, &dataWord, 4); // c
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 0);
  BOOST_CHECK(f.c == 2);
  dummyBackend->write(static_cast<uint64_t>(0), 28, &dataWord, 4); // bc
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 1);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 32, &dataWord, 4); // read only
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 1);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 36, &dataWord, 4); // ab
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 2);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 40, &dataWord, 4); // read only
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 2);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 44, &dataWord, 4); // read only
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 2);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 48, &dataWord, 4); // b
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 3);
  BOOST_CHECK(f.c == 3);

  std::vector<int32_t> dataContents(8, 42); // eight words, each with content 42
  f.a = 0;
  f.b = 0;
  f.c = 0;
  dummyBackend->write(static_cast<uint64_t>(0), 20, &(dataContents[0]), 32); // abc
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 1);
  BOOST_CHECK(f.c == 1);
  dummyBackend->write(static_cast<uint64_t>(0), 20, &(dataContents[0]), 8); // c
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 1);
  BOOST_CHECK(f.c == 2);
  dummyBackend->write(static_cast<uint64_t>(0), 20, &(dataContents[0]), 12); // bc
  BOOST_CHECK(f.a == 1);
  BOOST_CHECK(f.b == 2);
  BOOST_CHECK(f.c == 3);
  dummyBackend->write(static_cast<uint64_t>(0), 28, &(dataContents[0]), 24); // abc
  BOOST_CHECK(f.a == 2);
  BOOST_CHECK(f.b == 3);
  BOOST_CHECK(f.c == 4);
  dummyBackend->write(static_cast<uint64_t>(0), 32, &(dataContents[0]), 16); // ab
  BOOST_CHECK(f.a == 3);
  BOOST_CHECK(f.b == 4);
  BOOST_CHECK(f.c == 4);
  dummyBackend->write(static_cast<uint64_t>(0), 40, &(dataContents[0]), 8); // readOnly
  BOOST_CHECK(f.a == 3);
  BOOST_CHECK(f.b == 4);
  BOOST_CHECK(f.c == 4);
  dummyBackend->write(static_cast<uint64_t>(0), 4, &(dataContents[0]), 8); // nothing
  BOOST_CHECK(f.a == 3);
  BOOST_CHECK(f.b == 4);
  BOOST_CHECK(f.c == 4);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteRegisterWithoutCallback) {
  f.a = 0;
  f.b = 0;
  f.c = 0;
  int32_t dataWord = 42;
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  dummyBackend->writeRegisterWithoutCallback(0, 20, dataWord); // c has callback installed on this register
  BOOST_CHECK(f.a == 0);
  BOOST_CHECK(f.b == 0);
  BOOST_CHECK(f.c == 0); // c must not change

  // read only is also disabled for this internal function
  dummyBackend->read(static_cast<uint64_t>(0), 40, &dataWord, 4);
  dummyBackend->writeRegisterWithoutCallback(0, 40, dataWord + 1);
  int32_t readbackDataWord;
  dummyBackend->read(static_cast<uint64_t>(0), 40, &readbackDataWord, 4);
  BOOST_CHECK(readbackDataWord == dataWord + 1);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteToReadOnlyRegister) {
  ChimeraTK::Device dummyDevice;
  dummyDevice.open("DUMMYD0");

  // Also get pointer to the backend in order to check the catalogue
  TestableDummyBackend* dummyBackend = f.getBackendInstance();

  const std::string DUMMY_WRITEABLE_SUFFIX{".DUMMY_WRITEABLE"};
  auto ro_register = dummyDevice.getScalarRegisterAccessor<int>(READ_ONLY_REGISTER_STRING);
  auto ro_register_dw = dummyDevice.getScalarRegisterAccessor<int>(READ_ONLY_REGISTER_STRING + DUMMY_WRITEABLE_SUFFIX);

  // The suffixed register must not appear when iterating the the catalogue.
  // However, the catalogue knows it when I "guess" the name.
  auto dummyCatalogue = dummyBackend->getRegisterCatalogue();
  bool found = false;
  // Test 1: DUMMY_WRITABLE not in iterable catalogue
  for(auto& info : dummyCatalogue) {
    if(info.getRegisterName() == READ_ONLY_REGISTER_STRING + DUMMY_WRITEABLE_SUFFIX) {
      found = true;
      break;
    }
  }
  BOOST_CHECK(found == false);

  // Test 2: Register without DUMMY_WRITEABLE is in the iterable catalogue
  for(auto& info : dummyCatalogue) {
    if(info.getRegisterName() == READ_ONLY_REGISTER_STRING) {
      found = true;
      break;
    }
  }
  BOOST_CHECK(found == true);

  // Test 3 (should be taken over by the unified test) Whe I know the name the register info is there)
  BOOST_CHECK(dummyCatalogue.hasRegister(READ_ONLY_REGISTER_STRING + DUMMY_WRITEABLE_SUFFIX));
  auto info = dummyCatalogue.getRegister(READ_ONLY_REGISTER_STRING + DUMMY_WRITEABLE_SUFFIX);
  // FIXME: This test is currently failing.
  // BOOST_CHECK_EQUAL(info.getRegisterName(), READ_ONLY_REGISTER_STRING + DUMMY_WRITEABLE_SUFFIX);
  BOOST_CHECK(info.isWriteable());

  // Read-only register and the DUMMY_WRITEABLE companion
  // should return appropriate read-only and writeable flags
  BOOST_CHECK(ro_register.isReadOnly());
  BOOST_CHECK(!ro_register.isWriteable());
  BOOST_CHECK(!ro_register_dw.isReadOnly());
  BOOST_CHECK(ro_register_dw.isWriteable());

  // Test writing to the DUMMY_WRITEABLE register and
  // read back through the real register
  ro_register_dw = 42;
  ro_register_dw.write();
  ro_register.read();

  BOOST_CHECK_EQUAL(ro_register, ro_register_dw);

  // Writeing to read-only register must throw and not effect the content
  ro_register = 84;
  BOOST_CHECK_THROW(ro_register.write(), ChimeraTK::logic_error);
  ro_register.read();
  BOOST_CHECK_NE(ro_register, 84);
  BOOST_CHECK_EQUAL(ro_register, ro_register_dw);

  // Don't close the device here because the backend needs to stay open
  // for the following test cases
}

BOOST_AUTO_TEST_CASE(testDummyInterrupt) {
  ChimeraTK::Device dummyDevice;
  dummyDevice.open("DUMMYD0");

  // Also get pointer to the backend in order to check the catalogue
  TestableDummyBackend* dummyBackend = f.getBackendInstance();

  const std::string DUMMY_INTERRUPT{"/DUMMY_INTERRUPT_3"};
  auto ro_register = dummyDevice.getScalarRegisterAccessor<int>(DUMMY_INTERRUPT);

  // The suffixed register must not appear in the catalogue when iterating
  auto dummyCatalogue = dummyBackend->getRegisterCatalogue();
  bool found = false;
  for(auto& info : dummyCatalogue) {
    if(info.getRegisterName() == DUMMY_INTERRUPT) {
      found = true;
      break;
    }
  }
  BOOST_CHECK(found == false);

  // If I guess the name correctly, the register info is there
  BOOST_CHECK(dummyBackend->getRegisterCatalogue().hasRegister(DUMMY_INTERRUPT));
  auto info = dummyCatalogue.getRegister(DUMMY_INTERRUPT);
  BOOST_CHECK_EQUAL(info.getRegisterName(), DUMMY_INTERRUPT);

  // Don't close the device here because the backend needs to stay open
  // for the following test cases
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAddressRange) {
  TestableDummyBackend::AddressRange range24_8_0(0, 24, 8);

  BOOST_CHECK(range24_8_0.offset == 24);
  BOOST_CHECK(range24_8_0.sizeInBytes == 8);
  BOOST_CHECK(range24_8_0.bar == 0);

  TestableDummyBackend::AddressRange range24_8_1(1, 24, 8);   // larger bar
  TestableDummyBackend::AddressRange range12_8_1(1, 12, 8);   // larger bar, smaller offset
  TestableDummyBackend::AddressRange range28_8_0(0, 28, 8);   // larger offset
  TestableDummyBackend::AddressRange range28_8_1(1, 28, 8);   // larger bar, larger offset
  TestableDummyBackend::AddressRange range24_12_0(0, 24, 12); // different size, compares equal with range1

  // compare 24_8_0 with the other cases as left argument
  BOOST_CHECK((range24_8_0 < range24_8_1));
  BOOST_CHECK((range24_8_0 < range12_8_1));
  BOOST_CHECK((range24_8_0 < range28_8_0));
  BOOST_CHECK((range24_8_0 < range28_8_1));
  BOOST_CHECK(!(range24_8_0 < range24_12_0));

  // compare 24_8_0 with the other cases as right argument
  BOOST_CHECK(!(range24_8_1 < range24_8_0));
  BOOST_CHECK(!(range12_8_1 < range24_8_0));
  BOOST_CHECK(!(range28_8_0 < range24_8_0));
  BOOST_CHECK(!(range28_8_1 < range24_8_0));
  BOOST_CHECK(!(range24_12_0 < range24_8_0));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testIsWriteRangeOverlap) {
  // the only test not covered by the writeCallbackFunction test:
  // An overlapping range in different bars
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  bool overlap = dummyBackend->isWriteRangeOverlap(
      TestableDummyBackend::AddressRange(0, 0, 12), TestableDummyBackend::AddressRange(1, 0, 12));
  BOOST_CHECK(overlap == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFinalClosing) {
  // all features have to be enabled before closing
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  BOOST_CHECK(dummyBackend->_barContents.size() != 0);
  BOOST_CHECK(dummyBackend->_readOnlyAddresses.size() != 0);
  BOOST_CHECK(dummyBackend->_writeCallbackFunctions.size() != 0);

  dummyBackend->close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testOpenClose) {
  TestableDummyBackend* dummyBackend = f.getBackendInstance();
  // there have to be bars 0 and 2  with sizes 0x14C and 0x1000 bytes,
  // plus the dma bar 0xD
  // BOOST_CHECK((*dummyBackend)._barContents.size() == 3 );
  BOOST_CHECK(dummyBackend->_barContents.size() == 3);
  std::map<uint64_t, std::vector<int32_t>>::const_iterator bar0Iter = dummyBackend->_barContents.find(0);
  BOOST_REQUIRE(bar0Iter != dummyBackend->_barContents.end());
  BOOST_CHECK(bar0Iter->second.size() == 0x53); // 0x14C bytes in 32 bit words
  std::map<uint64_t, std::vector<int32_t>>::const_iterator bar2Iter = dummyBackend->_barContents.find(2);
  BOOST_REQUIRE(bar2Iter != dummyBackend->_barContents.end());
  BOOST_CHECK(bar2Iter->second.size() == 0x400); // 0x1000 bytes in 32 bit words

  // the "portmapFile" has an implicit conversion to bool to check
  // if it points to NULL
  // BOOST_CHECK(dummyBackend->_registerMapping);
  BOOST_CHECK(dummyBackend->isOpen());
  // it must always be possible to re-open a backend
  dummyBackend->open();
  BOOST_CHECK(dummyBackend->isOpen());

  dummyBackend->close();
  BOOST_CHECK(dummyBackend->isOpen() == false);
  // it must always be possible to re-close a backend
  dummyBackend->close();
  BOOST_CHECK(dummyBackend->isOpen() == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testClose) {
  /** Try closing the backend */
  f._backendInstance->close();
  /** backend should not be open now */
  BOOST_CHECK(f._backendInstance->isOpen() == false);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testOpen) {
  f._backendInstance->open();
  BOOST_CHECK(f._backendInstance->isOpen() == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCreateBackend) {
  /** Only for testing purpose */
  std::map<std::string, std::string> pararmeters;
  BOOST_CHECK_THROW(DummyBackend::createInstance("", pararmeters), ChimeraTK::logic_error);
  /** Try creating a non existing backend */
  BOOST_CHECK_THROW(FactoryInstance.createBackend(NON_EXISTING_DEVICE), ChimeraTK::logic_error);
  std::string cdd1 = "(dummy?map=" TEST_MAPPING_FILE ")";
  auto backendInstance = FactoryInstance.createBackend(cdd1);
  BOOST_CHECK(backendInstance);
  /** backend should not be in open state */
  BOOST_CHECK(backendInstance->isOpen() == false);

  /** check that the creation of different instances with the same map file */
  auto instance2 = BackendFactory::getInstance().createBackend(cdd1);
  std::string cdd3 = "(dummy:FOO?map=" TEST_MAPPING_FILE ")";
  auto instance3 = BackendFactory::getInstance().createBackend(cdd3);
  auto instance4 = BackendFactory::getInstance().createBackend(cdd3);
  std::string cdd5 = "(dummy:BAR?map=" TEST_MAPPING_FILE ")";
  auto instance5 = BackendFactory::getInstance().createBackend(cdd5);

  // instance 1 and 2 are the same
  BOOST_CHECK(backendInstance.get() == instance2.get());
  // instance 3 and 4 are the same
  BOOST_CHECK(instance3.get() == instance4.get());

  // instances 1, 3 and 5 are all different
  BOOST_CHECK(backendInstance.get() != instance3.get());
  BOOST_CHECK(backendInstance.get() != instance5.get());
  BOOST_CHECK(instance3.get() != instance5.get());
}

/**********************************************************************************************************************/
