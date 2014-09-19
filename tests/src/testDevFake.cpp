#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include "devFake.h"
#include <cstdio>
#include "exDevFake.h"
using namespace boost::unit_test_framework;

class FakeDeviceTest {
 public:
  void testReOpenExistingDevice();
  void testCreateFakeDevice();
  void testReadRegister();
  void testReadRegisterWithInvalidParams();
  void testReadArea();
  void testReadDMA();
  void testReadAreaWithInvalidParams();
  void testWriteReg();
  void testWriteRegErrors();
  void testWriteArea();
  void testWriteDMA();
  void testWriteAreaWithInvalidParams();
  void testDeviceInfo();
  //void testReadFseekException();
};

class FakeDeviceTestSuite : public test_suite {
 public:
  FakeDeviceTestSuite() : test_suite("devFake class test suite") {

    boost::shared_ptr<FakeDeviceTest> FakeDeviceTestPtr(new FakeDeviceTest());

    test_case* testOpendevice = BOOST_CLASS_TEST_CASE(
        &FakeDeviceTest::testReOpenExistingDevice, FakeDeviceTestPtr);
    test_case* testCreateFakeDevice = BOOST_CLASS_TEST_CASE(
        &FakeDeviceTest::testCreateFakeDevice, FakeDeviceTestPtr);
    test_case* testReadElement = BOOST_CLASS_TEST_CASE(
        &FakeDeviceTest::testReadRegister, FakeDeviceTestPtr);
    test_case* testReadRegisterWithInvalidParams = BOOST_CLASS_TEST_CASE(
        &FakeDeviceTest::testReadRegisterWithInvalidParams, FakeDeviceTestPtr);
    test_case* testReadArea =
        BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadArea, FakeDeviceTestPtr);
    test_case* testReadDMA =
        BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadDMA, FakeDeviceTestPtr);
    test_case* testReadAreaWithInvalidParams = BOOST_CLASS_TEST_CASE(
        &FakeDeviceTest::testReadAreaWithInvalidParams, FakeDeviceTestPtr);
    test_case* testWriteReg = BOOST_CLASS_TEST_CASE(
           &FakeDeviceTest::testWriteReg, FakeDeviceTestPtr);
    test_case* testWriteRegErrors = BOOST_CLASS_TEST_CASE(
           &FakeDeviceTest::testWriteRegErrors, FakeDeviceTestPtr);
    test_case* testWriteArea = BOOST_CLASS_TEST_CASE(
            &FakeDeviceTest::testWriteArea, FakeDeviceTestPtr);
    test_case* testWriteDMA = BOOST_CLASS_TEST_CASE(
            &FakeDeviceTest::testWriteDMA, FakeDeviceTestPtr);
    test_case* testWriteAreaWithInvalidParams = BOOST_CLASS_TEST_CASE(
            &FakeDeviceTest::testWriteAreaWithInvalidParams, FakeDeviceTestPtr);
    test_case* testDeviceInfo = BOOST_CLASS_TEST_CASE(
                &FakeDeviceTest::testDeviceInfo, FakeDeviceTestPtr);
/*    test_case* testReadFseekException = BOOST_CLASS_TEST_CASE(
                &FakeDeviceTest::testReadFseekException, FakeDeviceTestPtr);*/

    add(testOpendevice);
    add(testCreateFakeDevice);
    add(testReadElement);
    add(testReadRegisterWithInvalidParams);
    add(testReadArea);
    add(testReadDMA);
    add(testReadAreaWithInvalidParams);
    add(testWriteReg);
    add(testWriteRegErrors);
    add(testWriteArea);
    add(testWriteDMA);
    add(testWriteAreaWithInvalidParams);
    add(testDeviceInfo);
    //add(testReadFseekException);
  }
};
test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "devFake class test suite";
  framework::master_test_suite().add(new FakeDeviceTestSuite());

  return NULL;
}

void FakeDeviceTest::testReOpenExistingDevice() {
  mtca4u::devFake dummyDevice;
  FILE* file = fopen("._fake_file", "w");
  fclose(file);
  dummyDevice.openDev("fake_file");
  BOOST_CHECK_THROW(dummyDevice.openDev("fake_file"), mtca4u::exDevFake);
  try {
    dummyDevice.openDev("fake_file");
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_OPENED);
  }
  remove("_.fake_file");
}

void FakeDeviceTest::testCreateFakeDevice() {
  remove("._fakeDevice");
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("fakeDevice");
  boost::filesystem::path pathToCreatedFile("._fakeDevice");
  BOOST_CHECK(boost::filesystem::exists(pathToCreatedFile) == true);
  remove("._fakeDevice");
}
void FakeDeviceTest::testReadRegister() {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("Reference_Device");
  int32_t data = 0;
  uint32_t offset = 12;
  uint8_t bar = 2;
  dummyDevice.readReg(offset, &data, bar);
  BOOST_CHECK((uint32_t)data == 0xFFF0FFFF);
}

void FakeDeviceTest::testReadRegisterWithInvalidParams() {
  mtca4u::devFake dummyDevice;
  int32_t data = 0;
  uint32_t offset = 12;
  uint8_t bar = 2;
  BOOST_CHECK_THROW(dummyDevice.readReg(offset, &data, bar), mtca4u::exDevFake);
  try {
    dummyDevice.readReg(offset, &data, bar);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_CLOSED);
  }

  dummyDevice.openDev("DummyDevice");
  BOOST_CHECK_THROW(dummyDevice.readReg(offset, &data, 8), mtca4u::exDevFake);
  try {
    dummyDevice.readReg(offset, &data, 8);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
  }

  BOOST_CHECK_THROW(
      dummyDevice.readReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, &data, 2),
      mtca4u::exDevFake);
  try {
    dummyDevice.readReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, &data, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
  }
  remove("._DummyDevice");
}

void FakeDeviceTest::testReadArea() {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("Reference_Device");
  int32_t data[4];
  uint32_t offset = 12;
  uint8_t bar = 2;
  uint8_t sizeToReadInBytes = 16;
  dummyDevice.readArea(offset, data, sizeToReadInBytes, bar);
  BOOST_CHECK((uint32_t)data[0] == 0xFFF0FFFF);
  BOOST_CHECK((uint32_t)data[1] == 0x01EFCDAB);
  BOOST_CHECK((uint32_t)data[2] == 0x55555555);
  BOOST_CHECK((uint32_t)data[3] == 0x00000000);
}

void FakeDeviceTest::testReadDMA() {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("Reference_Device");
  int32_t data[4];
  uint32_t offset = 12;
  uint8_t bar = 2;
  uint8_t sizeToReadInBytes = 16;
  dummyDevice.readDMA(offset, data, sizeToReadInBytes, bar);
  BOOST_CHECK((uint32_t)data[0] == 0xFFF0FFFF);
  BOOST_CHECK((uint32_t)data[1] == 0x01EFCDAB);
  BOOST_CHECK((uint32_t)data[2] == 0x55555555);
  BOOST_CHECK((uint32_t)data[3] == 0x00000000);
}

void FakeDeviceTest::testReadAreaWithInvalidParams() {
  mtca4u::devFake dummyDevice;
  int32_t data[4];
  BOOST_CHECK_THROW(dummyDevice.readDMA(10, data, 3, 2), mtca4u::exDevFake);
  try {
    dummyDevice.readDMA(10, data, 3, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_CLOSED);
  }
  dummyDevice.openDev("DummyDevice");
  BOOST_CHECK_THROW(dummyDevice.readDMA(10, data, 3, 2), mtca4u::exDevFake);
  try {
    dummyDevice.readDMA(10, data, 3, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_READ_DATA_ERROR);
  }
  remove("._DummyDevice");
}

void FakeDeviceTest::testWriteReg () {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("DummyDevice");
  dummyDevice.writeReg(8, 0x01020304, 5);
  int32_t data;
  dummyDevice.readReg(8, &data, 5);
  BOOST_CHECK((uint32_t) data == 0x01020304);
  remove("._DummyDevice");
}

void FakeDeviceTest::testWriteRegErrors () {
  mtca4u::devFake dummyDevice;
  int32_t data = 0x01020304;
  uint32_t offset = 12;
  uint8_t bar = 2;
  BOOST_CHECK_THROW(dummyDevice.writeReg(offset, data, bar), mtca4u::exDevFake);
  try {
    dummyDevice.writeReg(offset, data, bar);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_CLOSED);
  }

  dummyDevice.openDev("DummyDevice");
  BOOST_CHECK_THROW(dummyDevice.writeReg(offset, data, 8), mtca4u::exDevFake);
  try {
    dummyDevice.writeReg(offset, data, 8);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
  }

  BOOST_CHECK_THROW(
      dummyDevice.writeReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, data, 2),
      mtca4u::exDevFake);
  try {
    dummyDevice.writeReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, data, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
  }
  remove("._DummyDevice");
}

void FakeDeviceTest::testWriteArea () {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("DummyDevice");
  const int dataSize = 4;
  int dataSizeInBytes = 16;
  int32_t input_data[dataSize] = {1, 4, 6, 7};
  int32_t output_data[dataSize] = {0};
  uint32_t offset = 12;
  uint8_t bar = 2;

  dummyDevice.writeArea(offset, input_data, dataSizeInBytes, bar);
  dummyDevice.readArea(offset, output_data, dataSizeInBytes, bar);

  BOOST_CHECK((uint32_t)output_data[0] == 0x00000001);
  BOOST_CHECK((uint32_t)output_data[1] == 0x00000004);
  BOOST_CHECK((uint32_t)output_data[2] == 0x00000006);
  BOOST_CHECK((uint32_t)output_data[3] == 0x00000007);
  remove("._DummyDevice");
}

void FakeDeviceTest::testWriteDMA () {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("DummyDevice");
  const int dataSize = 4;
  int dataSizeInBytes = 16;
  int32_t input_data[dataSize] = {1, 4, 6, 7};
  int32_t output_data[dataSize] = {0};
  uint32_t offset = 12;
  uint8_t bar = 2;
  dummyDevice.writeDMA(offset, input_data, dataSizeInBytes, bar);
  dummyDevice.readDMA(offset, output_data, dataSizeInBytes, bar);

  BOOST_CHECK((uint32_t)output_data[0] == 0x00000001);
  BOOST_CHECK((uint32_t)output_data[1] == 0x00000004);
  BOOST_CHECK((uint32_t)output_data[2] == 0x00000006);
  BOOST_CHECK((uint32_t)output_data[3] == 0x00000007);
  remove("._DummyDevice");
}

void FakeDeviceTest::testWriteAreaWithInvalidParams () {
  mtca4u::devFake dummyDevice;
  int32_t data[4];
  BOOST_CHECK_THROW(dummyDevice.writeDMA(10, data, 3, 2), mtca4u::exDevFake);
  try {
    dummyDevice.writeDMA(10, data, 3, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_CLOSED);
  }
  dummyDevice.openDev("DummyDevice");
  int wrongDataSize = 3;
  BOOST_CHECK_THROW(dummyDevice.writeDMA(10, data, wrongDataSize, 2), mtca4u::exDevFake);
  try {
    dummyDevice.writeDMA(10, data, 3, 2);
  }
  catch (mtca4u::exDevFake& a) {
    BOOST_CHECK(a.getID() == mtca4u::exDevFake::EX_DEVICE_FILE_WRITE_DATA_ERROR);
  }
  remove("._DummyDevice");
}

void FakeDeviceTest::testDeviceInfo () {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("DummyDevice");
  std::string deviceInformation;
  std::string expectedInfoString = "fake device: ._DummyDevice";
  dummyDevice.readDeviceInfo(&deviceInformation);
  BOOST_CHECK(deviceInformation == expectedInfoString );
  remove("._DummyDevice");
}

/*void FakeDeviceTest::testReadFseekException () {
  mtca4u::devFake dummyDevice;
  dummyDevice.openDev("DummyDevice");
}*/
