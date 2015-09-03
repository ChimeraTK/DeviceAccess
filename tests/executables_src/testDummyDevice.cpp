#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "DeviceFactory.h"
#include "DummyDevice.h"
#include "NotImplementedException.h"
using namespace mtca4u;

#define TEST_MAPPING_FILE "mtcadummy_withoutModules.map"
#define FIRMWARE_REGISTER_STRING "WORD_FIRMWARE"
#define STATUS_REGISTER_STRING "WORD_STATUS"
#define USER_REGISTER_STRING "WORD_USER"
#define CLOCK_MUX_REGISTER_STRING "WORD_CLK_MUX"
#define CLOCK_RESET_REGISTER_STRING "WORD_CLK_RST"
#define EXISTING_DEVICE "DUMMYD0"
#define NON_EXISTING_DEVICE "DUMMY9"

static DeviceFactory FactoryInstance = DeviceFactory::getInstance();
// forward declaration so we can declare it friend
// of TestableDummyDevice.
class DummyDeviceTest;

/** The TestableDummyDevice is derived from
 *  DummyDevice to get access to the protected members.
 *  This is done by declaring DummyDeviceTest as a friend.
 */
class TestableDummyDevice : public DummyDevice {
  friend class DummyDeviceTest;
};

class DummyDeviceTest {
public:
  DummyDeviceTest() : a(0), b(0), c(0), BaseDeviceInstance() {}

  static void testCalculateVirtualAddress();
  static void testCheckSizeIsMultipleOfWordSize();
  static void testAddressRange();
  // void testOpenClose();
  void testReadWriteSingleWordRegister();
  /// The read function can be readDMA or readArea, the writeFunction is always
  /// writeArea.
  /// WriteDMA probably does not make sense at all.
  /// The argument is a pointer to a member function of the DummyDevice class,
  /// which
  /// returns void and takes uint32_t, int32_t*, size_t, uint8_t as arguments.
  void testReadWriteMultiWordRegister(
      void (DummyDevice::*readFunction)(uint32_t, int32_t*, size_t, uint8_t));
  void testWriteDMA();
  void testReadDeviceInfo();
  void testReadOnly();
  void testWriteCallbackFunctions();
  void testIsWriteRangeOverlap();
  void testWriteRegisterWithoutCallback();
  /// Test that all registers, read-only flags and callback functions are
  /// removed
  void testFinalClosing();

  // Try Creating a device and check if it is connected.
  void testCreateDevice();

  // Try opening the created device and check it's open status.
  void testopenice();

  // Try closing the created device and check it's open status.
  void testcloseice();

  void testOpencloseice();

private:
  TestableDummyDevice _dummyDevice;
  void freshlyopenice();
  TestableDummyDevice* getBaseDeviceInstance(bool reOpen = false);
  friend class DummyDeviceTestSuite;

  // stuff for the callback function test
  int a, b, c;
  void increaseA() { ++a; }
  void increaseB() { ++b; }
  void increaseC() { ++c; }
  boost::shared_ptr<mtca4u::BaseDevice> BaseDeviceInstance;
};

class DummyDeviceTestSuite : public test_suite {
public:
  DummyDeviceTestSuite() : test_suite("DummyDevice test suite") {
    boost::shared_ptr<DummyDeviceTest> dummyDeviceTest(new DummyDeviceTest);

    // Pointers to test cases with dependencies. All other test cases are added
    // directly.
    test_case* readOnlyTestCase =
        BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testReadOnly, dummyDeviceTest);
    test_case* writeCallbackFunctionsTestCase = BOOST_CLASS_TEST_CASE(
        &DummyDeviceTest::testWriteCallbackFunctions, dummyDeviceTest);
    test_case* writeRegisterWithoutCallbackTestCase = BOOST_CLASS_TEST_CASE(
        &DummyDeviceTest::testWriteRegisterWithoutCallback, dummyDeviceTest);

    test_case* createDeviceTestCase = BOOST_CLASS_TEST_CASE(
        &DummyDeviceTest::testCreateDevice, dummyDeviceTest);
    test_case* openiceTestCase = BOOST_CLASS_TEST_CASE(
        &DummyDeviceTest::testopenice, dummyDeviceTest);
    test_case* opencloseiceTestCase = BOOST_CLASS_TEST_CASE(
        &DummyDeviceTest::testOpencloseice, dummyDeviceTest);
    //test_case* closeiceTestCase = BOOST_CLASS_TEST_CASE(
     //   &DummyDeviceTest::testcloseice, dummyDeviceTest);

    // we use the setup from the read-only test to check that the callback
    // function is not executed if the register is not writeable.
    writeCallbackFunctionsTestCase->depends_on(readOnlyTestCase);
    writeRegisterWithoutCallbackTestCase->depends_on(
        writeCallbackFunctionsTestCase);

    // As boost test cases need nullary functions (working with parameters would
    // not work with function pointers)
    // we create them by binding readArea and readDMA to the
    // testReadWriteMultiWordRegister function of the
    // dummyDeviceTest instance.
    boost::function<void(void)> testReadWriteArea =
        boost::bind(&DummyDeviceTest::testReadWriteMultiWordRegister,
                    dummyDeviceTest, &DummyDevice::readArea);

    boost::function<void(void)> testReadWriteDMA =
        boost::bind(&DummyDeviceTest::testReadWriteMultiWordRegister,
                    dummyDeviceTest, &DummyDevice::readDMA);

    add(BOOST_TEST_CASE(DummyDeviceTest::testCalculateVirtualAddress));
    add(BOOST_TEST_CASE(DummyDeviceTest::testCheckSizeIsMultipleOfWordSize));
    add(BOOST_TEST_CASE(DummyDeviceTest::testAddressRange));
    // add( BOOST_CLASS_TEST_CASE( &DummyDeviceTest::testOpenClose,
    // dummyDeviceTest ) );
    add(BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testReadWriteSingleWordRegister,
                              dummyDeviceTest));
    add(BOOST_TEST_CASE(testReadWriteArea)); // not BOOST_CLASS_TEST_CASE as the
                                             // functions are already bound to
                                             // the instance
    add(BOOST_TEST_CASE(testReadWriteDMA));
    add(BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testWriteDMA, dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testReadDeviceInfo,
                              dummyDeviceTest));
    add(readOnlyTestCase);
    add(writeCallbackFunctionsTestCase);
    add(writeRegisterWithoutCallbackTestCase);
    add(BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testIsWriteRangeOverlap,
                              dummyDeviceTest));
    add(BOOST_CLASS_TEST_CASE(&DummyDeviceTest::testFinalClosing,
                              dummyDeviceTest));
    add(createDeviceTestCase);
    add(openiceTestCase);
    add(opencloseiceTestCase);

    //add (closeiceTestCase);
  }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "DummyDevice test suite";
  framework::master_test_suite().add(new DummyDeviceTestSuite);

  return NULL;
}

void DummyDeviceTest::testCalculateVirtualAddress() {
  BOOST_CHECK(DummyDevice::calculateVirtualAddress(0, 0) == 0UL);
  BOOST_CHECK(DummyDevice::calculateVirtualAddress(0x35, 0) == 0x35UL);
  BOOST_CHECK(DummyDevice::calculateVirtualAddress(0x67875, 0x3) ==
              0x3000000000067875UL);
  BOOST_CHECK(DummyDevice::calculateVirtualAddress(0, 0x4) ==
              0x4000000000000000UL);

  // the first bit of the bar has to be cropped
  BOOST_CHECK(DummyDevice::calculateVirtualAddress(0x123, 0xD) ==
              0x5000000000000123UL);
}

void DummyDeviceTest::testCheckSizeIsMultipleOfWordSize() {
  // just some arbitrary numbers to test %4 = 0, 1, 2, 3
  BOOST_CHECK_NO_THROW(TestableDummyDevice::checkSizeIsMultipleOfWordSize(24));

  BOOST_CHECK_THROW(TestableDummyDevice::checkSizeIsMultipleOfWordSize(25),
                    DummyDeviceException);

  BOOST_CHECK_THROW(TestableDummyDevice::checkSizeIsMultipleOfWordSize(26),
                    DummyDeviceException);

  BOOST_CHECK_THROW(TestableDummyDevice::checkSizeIsMultipleOfWordSize(27),
                    DummyDeviceException);
}

void DummyDeviceTest::testReadWriteSingleWordRegister() {
  // freshlyopenice();
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance(true);
  RegisterInfoMap::RegisterInfo mappingElement;
  dummyDevice->_registerMapping->getRegisterInfo(CLOCK_RESET_REGISTER_STRING,
                                                 mappingElement);
  uint32_t offset = mappingElement.reg_address;
  uint8_t bar = mappingElement.reg_bar;
  int32_t dataContent = -1;
  BOOST_CHECK_NO_THROW(dummyDevice->readReg(offset, &dataContent, bar));
  BOOST_CHECK(dataContent == 0);
  dataContent = 47;
  BOOST_CHECK_NO_THROW(dummyDevice->writeReg(offset, dataContent, bar));
  dataContent = -1; // make sure the value is really being read
  // no need to test NO_THROW on the same register twice
  dummyDevice->readReg(offset, &dataContent, bar);
  BOOST_CHECK(dataContent == 47);

  // the size as index is invalid, allowed range is 0..size-1 included.
  BOOST_CHECK_THROW(dummyDevice->readReg(dummyDevice->_barContents[bar].size() *
                                             sizeof(int32_t),
                                         &dataContent, bar),
                    DummyDeviceException);
  BOOST_CHECK_THROW(dummyDevice->writeReg(
                        dummyDevice->_barContents[bar].size() * sizeof(int32_t),
                        dataContent, bar),
                    DummyDeviceException);
}

// void
// DummyDeviceTest::testReadWriteMultiWordRegister(boost::function<void(uint32_t,
// int32_t*, size_t, uint8_t)> readFunction){
void DummyDeviceTest::testReadWriteMultiWordRegister(
    void (DummyDevice::*readFunction)(uint32_t, int32_t*, size_t, uint8_t)) {
  // freshlyopenice();
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance(true);
  RegisterInfoMap::RegisterInfo mappingElement;
  dummyDevice->_registerMapping->getRegisterInfo(CLOCK_MUX_REGISTER_STRING,
                                                 mappingElement);

  uint32_t offset = mappingElement.reg_address;
  uint8_t bar = mappingElement.reg_bar;
  size_t sizeInBytes = mappingElement.reg_size;
  size_t sizeInWords = mappingElement.reg_size / sizeof(int32_t);
  std::vector<int32_t> dataContent(sizeInWords, -1);

  BOOST_CHECK_NO_THROW((dummyDevice->*readFunction)(offset, &(dataContent[0]),
                                                    sizeInBytes, bar));
  for (std::vector<int32_t>::iterator dataIter = dataContent.begin();
       dataIter != dataContent.end(); ++dataIter) {
    std::stringstream errorMessage;
    errorMessage << "*dataIter = " << *dataIter;
    BOOST_CHECK_MESSAGE(*dataIter == 0, errorMessage.str());
  }

  for (unsigned int index = 0; index < dataContent.size(); ++index) {
    dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
  }
  BOOST_CHECK_NO_THROW(
      dummyDevice->writeArea(offset, &(dataContent[0]), sizeInBytes, bar));
  // make sure the value is really being read
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);

  // no need to test NO_THROW on the same register twice
  dummyDevice->readArea(offset, &(dataContent[0]), sizeInBytes, bar);

  // make sure the value is really being read
  for (unsigned int index = 0; index < dataContent.size(); ++index) {
    BOOST_CHECK(dataContent[index] ==
                static_cast<int32_t>((index + 1) * (index + 1)));
  }

  // tests for exceptions
  // 1. base address too large
  BOOST_CHECK_THROW(dummyDevice->readArea(
                        dummyDevice->_barContents[bar].size() * sizeof(int32_t),
                        &(dataContent[0]), sizeInBytes, bar),
                    DummyDeviceException);
  BOOST_CHECK_THROW(dummyDevice->writeArea(
                        dummyDevice->_barContents[bar].size() * sizeof(int32_t),
                        &(dataContent[0]), sizeInBytes, bar),
                    DummyDeviceException);
  // 2. size too large (works because the target register is not at offfset 0)
  // resize the data vector for this test
  dataContent.resize(dummyDevice->_barContents[bar].size());
  BOOST_CHECK_THROW(dummyDevice->readArea(
                        offset, &(dataContent[0]),
                        dummyDevice->_barContents[bar].size() * sizeof(int32_t),
                        bar),
                    DummyDeviceException);
  BOOST_CHECK_THROW(dummyDevice->writeArea(
                        offset, &(dataContent[0]),
                        dummyDevice->_barContents[bar].size() * sizeof(int32_t),
                        bar),
                    DummyDeviceException);
  // 3. size not multiple of 4
  BOOST_CHECK_THROW(
      dummyDevice->readArea(offset, &(dataContent[0]), sizeInBytes - 1, bar),
      DummyDeviceException);
  BOOST_CHECK_THROW(
      dummyDevice->writeArea(offset, &(dataContent[0]), sizeInBytes - 1, bar),
      DummyDeviceException);
}

void DummyDeviceTest::freshlyopenice() {
  try {
    _dummyDevice.open(TEST_MAPPING_FILE);
  }
  catch (DummyDeviceException&) {
    // make sure the device was freshly opened, so
    // registers are set to 0.
    _dummyDevice.close();
    _dummyDevice.open(TEST_MAPPING_FILE);
  }
}

void DummyDeviceTest::testWriteDMA() {
  // will probably never be implemented
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  BOOST_CHECK_THROW(dummyDevice->writeDMA(0, NULL, 0, 0),
                    NotImplementedException);
}

TestableDummyDevice* DummyDeviceTest::getBaseDeviceInstance(bool reOpen) {
  if (BaseDeviceInstance == 0)
    BaseDeviceInstance = FactoryInstance.createDevice(EXISTING_DEVICE);
  if (reOpen || (!BaseDeviceInstance->isOpen())) {
    if (BaseDeviceInstance->isOpen())
      BaseDeviceInstance->close();
    BaseDeviceInstance->open();
  }
  BaseDevice * rawBasePointer = BaseDeviceInstance.get();
  return (static_cast<TestableDummyDevice*>(rawBasePointer));
}

void DummyDeviceTest::testReadDeviceInfo() {
  std::string deviceInfo;
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  deviceInfo = dummyDevice->readDeviceInfo();
  std::cout << deviceInfo << std::endl;
  BOOST_CHECK(deviceInfo ==
              (std::string("DummyDevice with mapping file ../tests/") +
               TEST_MAPPING_FILE));
}

void DummyDeviceTest::testReadOnly() {
  // freshlyopenice();
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance(true);
  RegisterInfoMap::RegisterInfo mappingElement;
  dummyDevice->_registerMapping->getRegisterInfo(CLOCK_MUX_REGISTER_STRING,
                                                 mappingElement);

  uint32_t offset = mappingElement.reg_address;
  uint8_t bar = mappingElement.reg_bar;
  size_t sizeInBytes = mappingElement.reg_size;
  size_t sizeInWords = mappingElement.reg_size / sizeof(int32_t);
  std::stringstream errorMessage;
  errorMessage << "This register should have 4 words. "
               << "If you changed your mapping you have to adapt "
               << "the testReadOnly() test.";
  BOOST_REQUIRE_MESSAGE(sizeInWords == 4, errorMessage.str());

  std::vector<int32_t> dataContent(sizeInWords);
  for (unsigned int index = 0; index < dataContent.size(); ++index) {
    dataContent[index] = static_cast<int32_t>((index + 1) * (index + 1));
  }
  dummyDevice->writeArea(offset, &(dataContent[0]), sizeInBytes, bar);
  dummyDevice->setReadOnly(offset, bar, 1);

  // the actual test: write 42 to all registers, register 0 must not change, all
  // others have to
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 42);
  dummyDevice->writeArea(offset, &(dataContent[0]), sizeInBytes, bar);
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
  dummyDevice->readArea(offset, &(dataContent[0]), sizeInBytes, bar);
  BOOST_CHECK(dataContent[0] == 1);
  BOOST_CHECK(dataContent[1] == 42);
  BOOST_CHECK(dataContent[2] == 42);
  BOOST_CHECK(dataContent[3] == 42);

  // also set the last two words to read only. Now only the second word has to
  // change.
  // We use the addressRange interface for it to also cover this.
  TestableDummyDevice::AddressRange lastTwoMuxRegisters(
      offset + 2 * sizeof(int32_t), 2 * sizeof(int32_t), bar);
  dummyDevice->setReadOnly(lastTwoMuxRegisters);
  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = 29);
  // also test with single write operations
  for (size_t index = 0; index < sizeInWords; ++index) {
    dummyDevice->writeReg(offset + index * sizeof(int32_t), dataContent[index],
                          bar);
  }

  std::for_each(dataContent.begin(), dataContent.end(), boost::lambda::_1 = -1);
  dummyDevice->readArea(offset, &(dataContent[0]), sizeInBytes, bar);
  BOOST_CHECK(dataContent[0] == 1);
  BOOST_CHECK(dataContent[1] == 29);
  BOOST_CHECK(dataContent[2] == 42);
  BOOST_CHECK(dataContent[3] == 42);

  // check that the next register is still writeable (boundary test)
  int32_t originalNextDataWord;
  dummyDevice->readReg(offset + sizeInBytes, &originalNextDataWord, bar);
  dummyDevice->writeReg(offset + sizeInBytes, originalNextDataWord + 1, bar);
  int32_t readbackWord;
  dummyDevice->readReg(offset + sizeInBytes, &readbackWord, bar);
  BOOST_CHECK(originalNextDataWord + 1 == readbackWord);
}

void DummyDeviceTest::testWriteCallbackFunctions() {
  // We just require the first bar to be 12 registers long.
  // Everything else would overcomplicate this test. For a real
  // application one would always use register names from mapping,
  // but this is not the purpose of this test.

  // from the previous test we know that adresses 32, 40 and 44 are write only
  // TestableDummyDevice *dummyDevice = static_cast<TestableDummyDevice*
  // >(BaseDeviceInstance);
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  BOOST_REQUIRE(dummyDevice->_barContents[0].size() >= 13);
  a = 0;
  b = 0;
  c = 0;
  dummyDevice->setWriteCallbackFunction(
      TestableDummyDevice::AddressRange(36, 4, 0),
      boost::bind(&DummyDeviceTest::increaseA, this));
  dummyDevice->setWriteCallbackFunction(
      TestableDummyDevice::AddressRange(28, 24, 0),
      boost::bind(&DummyDeviceTest::increaseB, this));
  dummyDevice->setWriteCallbackFunction(
      TestableDummyDevice::AddressRange(20, 12, 0),
      boost::bind(&DummyDeviceTest::increaseC, this));

  // test single writes
  int32_t dataWord(42);
  dummyDevice->writeReg(12, dataWord, 0); // nothing
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 0);
  BOOST_CHECK(c == 0);

  dummyDevice->writeReg(20, dataWord, 0); // c
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 0);
  BOOST_CHECK(c == 1);
  dummyDevice->writeReg(24, dataWord, 0); // c
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 0);
  BOOST_CHECK(c == 2);
  dummyDevice->writeReg(28, dataWord, 0); // bc
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 1);
  BOOST_CHECK(c == 3);
  dummyDevice->writeReg(32, dataWord, 0); // read only
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 1);
  BOOST_CHECK(c == 3);
  dummyDevice->writeReg(36, dataWord, 0); // ab
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 2);
  BOOST_CHECK(c == 3);
  dummyDevice->writeReg(40, dataWord, 0); // read only
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 2);
  BOOST_CHECK(c == 3);
  dummyDevice->writeReg(44, dataWord, 0); // read only
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 2);
  BOOST_CHECK(c == 3);
  dummyDevice->writeReg(48, dataWord, 0); // b
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 3);
  BOOST_CHECK(c == 3);

  std::vector<int32_t> dataContents(8, 42); // eight words, each with content 42
  a = 0;
  b = 0;
  c = 0;
  dummyDevice->writeArea(20, &(dataContents[0]), 32, 0); // abc
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 1);
  BOOST_CHECK(c == 1);
  dummyDevice->writeArea(20, &(dataContents[0]), 8, 0); // c
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 1);
  BOOST_CHECK(c == 2);
  dummyDevice->writeArea(20, &(dataContents[0]), 12, 0); // bc
  BOOST_CHECK(a == 1);
  BOOST_CHECK(b == 2);
  BOOST_CHECK(c == 3);
  dummyDevice->writeArea(28, &(dataContents[0]), 24, 0); // abc
  BOOST_CHECK(a == 2);
  BOOST_CHECK(b == 3);
  BOOST_CHECK(c == 4);
  dummyDevice->writeArea(32, &(dataContents[0]), 16, 0); // ab
  BOOST_CHECK(a == 3);
  BOOST_CHECK(b == 4);
  BOOST_CHECK(c == 4);
  dummyDevice->writeArea(40, &(dataContents[0]), 8, 0); // readOnly
  BOOST_CHECK(a == 3);
  BOOST_CHECK(b == 4);
  BOOST_CHECK(c == 4);
  dummyDevice->writeArea(4, &(dataContents[0]), 8, 0); // nothing
  BOOST_CHECK(a == 3);
  BOOST_CHECK(b == 4);
  BOOST_CHECK(c == 4);
}

void DummyDeviceTest::testWriteRegisterWithoutCallback() {
  a = 0;
  b = 0;
  c = 0;
  int32_t dataWord = 42;
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  dummyDevice->writeRegisterWithoutCallback(
      20, dataWord, 0); // c has callback installed on this register
  BOOST_CHECK(a == 0);
  BOOST_CHECK(b == 0);
  BOOST_CHECK(c == 0); // c must not change

  // read only is also disabled for this internal function
  dummyDevice->readReg(40, &dataWord, 0);
  dummyDevice->writeRegisterWithoutCallback(40, dataWord + 1, 0);
  int32_t readbackDataWord;
  dummyDevice->readReg(40, &readbackDataWord, 0);
  BOOST_CHECK(readbackDataWord == dataWord + 1);
}

void DummyDeviceTest::testAddressRange() {
  TestableDummyDevice::AddressRange range24_8_0(24, 8, 0);

  BOOST_CHECK(range24_8_0.offset == 24);
  BOOST_CHECK(range24_8_0.sizeInBytes == 8);
  BOOST_CHECK(range24_8_0.bar == 0);

  TestableDummyDevice::AddressRange range24_8_1(24, 8, 1); // larger bar
  TestableDummyDevice::AddressRange range12_8_1(
      12, 8, 1); // larger bar, smaller offset
  TestableDummyDevice::AddressRange range28_8_0(28, 8, 0); // larger offset
  TestableDummyDevice::AddressRange range28_8_1(28, 8,
                                                1); // larger bar, larger offset
  TestableDummyDevice::AddressRange range24_12_0(
      24, 12, 0); // different size, compares equal with range1

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

void DummyDeviceTest::testIsWriteRangeOverlap() {
  // the only test not covered by the writeCallbackFunction test:
  // An overlapping range in different bars
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  bool overlap = dummyDevice->isWriteRangeOverlap(
      TestableDummyDevice::AddressRange(0, 12, 0),
      TestableDummyDevice::AddressRange(0, 12, 1));
  BOOST_CHECK(overlap == false);
}

void DummyDeviceTest::testFinalClosing() {
  // all features have to be enabled before closing
  TestableDummyDevice* dummyDevice = getBaseDeviceInstance();
  BOOST_CHECK(dummyDevice->_barContents.size() != 0);
  BOOST_CHECK(dummyDevice->_readOnlyAddresses.size() != 0);
  BOOST_CHECK(dummyDevice->_writeCallbackFunctions.size() != 0);

  dummyDevice->close();

  // all features lists have to be empty now
  BOOST_CHECK(dummyDevice->_barContents.size() == 0);
  BOOST_CHECK(dummyDevice->_readOnlyAddresses.size() == 0);
  BOOST_CHECK(dummyDevice->_writeCallbackFunctions.size() == 0);
}

void DummyDeviceTest::testOpencloseice() {

  TestableDummyDevice* dummyDevice = getBaseDeviceInstance(true);
  // there have to be bars 0 and 2  with sizes 0x14C and 0x1000 bytes,
  // plus the dma bar 0xD
  // BOOST_CHECK((*dummyDevice)._barContents.size() == 3 );
  BOOST_CHECK(dummyDevice->_barContents.size() == 3);
  std::map<uint8_t, std::vector<int32_t> >::const_iterator bar0Iter =
      dummyDevice->_barContents.find(0);
  BOOST_REQUIRE(bar0Iter != dummyDevice->_barContents.end());
  BOOST_CHECK(bar0Iter->second.size() == 0x53); // 0x14C bytes in 32 bit words
  std::map<uint8_t, std::vector<int32_t> >::const_iterator bar2Iter =
      dummyDevice->_barContents.find(2);
  BOOST_REQUIRE(bar2Iter != dummyDevice->_barContents.end());
  BOOST_CHECK(bar2Iter->second.size() == 0x400); // 0x1000 bytes in 32 bit words

  // the "prtmapFile" has an implicit converion to bool to check
  // if it points to NULL
  BOOST_CHECK(dummyDevice->_registerMapping);
  BOOST_CHECK(dummyDevice->isOpen());
  BOOST_CHECK_THROW(dummyDevice->open(TEST_MAPPING_FILE),
                    DummyDeviceException);

  dummyDevice->close();
  // The map has to be empty and the reference counting pointer to the
  // mapping has to point to NULL.
  BOOST_CHECK(dummyDevice->_barContents.empty());
  BOOST_CHECK(dummyDevice->_registerMapping == false);
  BOOST_CHECK(dummyDevice->isOpen() == false);
  BOOST_CHECK_THROW(dummyDevice->close(), DummyDeviceException);
}

void DummyDeviceTest::testcloseice() {
  /** Try closing the device */
  BaseDeviceInstance->close();
  /** device should not be open now */
  BOOST_CHECK(BaseDeviceInstance->isOpen() == false);
  /** device should remain in connected state */
  BOOST_CHECK(BaseDeviceInstance->isConnected() == true);
}

void DummyDeviceTest::testopenice() {
  BaseDeviceInstance->open();
  BOOST_CHECK(BaseDeviceInstance->isOpen() == true);
  BOOST_CHECK(BaseDeviceInstance->isConnected() == true);
}

void DummyDeviceTest::testCreateDevice() {
  /** Try creating a non existing device */
  BOOST_CHECK_THROW(FactoryInstance.createDevice(NON_EXISTING_DEVICE),DeviceFactoryException);
  /** Try creating an existing device */
  BaseDeviceInstance = FactoryInstance.createDevice(EXISTING_DEVICE);
  BOOST_CHECK(BaseDeviceInstance != 0);
  /** Device should be in connect state now */
  BOOST_CHECK(BaseDeviceInstance->isConnected() == true);
  /** Device should not be in open state */
  BOOST_CHECK(BaseDeviceInstance->isOpen() == false);
}
