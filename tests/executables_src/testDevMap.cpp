#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <cstring>

#include "MappedDevice.h"
#include "PcieDevice.h"
#include "ExcMappedDevice.h"
#include "ExcPcieDevice.h"
#include "DummyDevice.h"
#include "MultiplexedDataAccessor.h"
#include "DeviceFactory.h"
//#define DUMMY_DEVICE_FILE_NAME
using namespace boost::unit_test_framework;

class DevMapTest {
public:
  void testDevMapReadRegisterByName();
  void testDevMapReadRegister();
  void testDevMapReadArea();
  void testDevMapReadDMA();
  void testDevMapReadDMAErrors();
  void testDevMapWriteRegisterByName();
  void testDevMapWriteRegister();
  void testDevMapWriteDMA();
  void testDevMapCheckRegister();
  void testRegAccsorReadDMA();
  void testRegAccsorWriteDMA();
  void testRegAccsorCheckRegister();
  void testRegAccsorReadReg();
  void testRegAccsorWriteReg();
  void testDeviceInfo();

  void testReadBadReg();
  void testWriteBadReg();
  void testDMAReadSizeTooSmall();
  void testDMAReadViaStruct();

  void testGetRegistersInModule();
  void testGetRegisterAccessorsInModule();
  void testAccessorForMuxedData();
};

class DevMapTestSuite : public test_suite {
public:
  DevMapTestSuite() : test_suite("devMap class test suite") {

    boost::shared_ptr<DevMapTest> DevMapTestPtr(new DevMapTest());

    test_case* testDevMapReadRegisterByName = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapReadRegisterByName, DevMapTestPtr);

    test_case* testDevMapReadRegister = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapReadRegister, DevMapTestPtr);

    test_case* testDevMapReadArea =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testDevMapReadArea, DevMapTestPtr);

    test_case* testDevMapReadDMA =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testDevMapReadDMA, DevMapTestPtr);

    test_case* testDevMapReadDMAErrors = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapReadDMAErrors, DevMapTestPtr);

    test_case* testDevMapWriteRegisterByName = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapWriteRegisterByName, DevMapTestPtr);

    test_case* testDevMapWriteRegister = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapWriteRegister, DevMapTestPtr);

    test_case* testDevMapWriteDMA =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testDevMapWriteDMA, DevMapTestPtr);

    test_case* testDevMapCheckRegister = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDevMapCheckRegister, DevMapTestPtr);

    test_case* testRegAccsorReadDMA =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testRegAccsorReadDMA, DevMapTestPtr);

    test_case* testRegAccsorCheckRegister = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testRegAccsorCheckRegister, DevMapTestPtr);

    test_case* testRegAccsorWriteDMA = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testRegAccsorWriteDMA, DevMapTestPtr);

    test_case* testRegAccsorReadReg =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testRegAccsorReadReg, DevMapTestPtr);

    test_case* testRegAccsorWriteReg = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testRegAccsorWriteReg, DevMapTestPtr);

    test_case* testDeviceInfo =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testDeviceInfo, DevMapTestPtr);

    test_case* testReadBadReg =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testReadBadReg, DevMapTestPtr);

    test_case* testWriteBadReg =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testWriteBadReg, DevMapTestPtr);

    test_case* testDMAReadSizeTooSmall = BOOST_CLASS_TEST_CASE(
        &DevMapTest::testDMAReadSizeTooSmall, DevMapTestPtr);

    test_case* testDMAReadViaStruct =
        BOOST_CLASS_TEST_CASE(&DevMapTest::testDMAReadViaStruct, DevMapTestPtr);

    add(testDevMapReadRegisterByName);
    add(testDevMapReadRegister);
    add(testDevMapReadArea);
    add(testDevMapReadDMA);
    add(testDevMapReadDMAErrors);
    add(testDevMapWriteRegisterByName);
    add(testDevMapWriteRegister);
    add(testDevMapWriteDMA);
    add(testDevMapCheckRegister);
    add(testRegAccsorReadDMA);
    add(testRegAccsorCheckRegister);
    add(testRegAccsorWriteDMA);
    add(testRegAccsorReadReg);
    add(testRegAccsorWriteReg);
    add(testDeviceInfo);
    add(testReadBadReg);
    add(testWriteBadReg);
    add(testDMAReadSizeTooSmall);
    add(testDMAReadViaStruct);

    add(BOOST_CLASS_TEST_CASE(&DevMapTest::testGetRegistersInModule,
                              DevMapTestPtr));
    add(BOOST_CLASS_TEST_CASE(&DevMapTest::testGetRegisterAccessorsInModule,
                              DevMapTestPtr));
    add(BOOST_CLASS_TEST_CASE(&DevMapTest::testAccessorForMuxedData,
                              DevMapTestPtr));
  }
};
test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "devMap class test suite";
  framework::master_test_suite().add(new DevMapTestSuite());

  return NULL;
}

void DevMapTest::testDevMapReadRegisterByName() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data;
  pcieDevice.readReg("WORD_CLK_DUMMY", &data);
  BOOST_CHECK(data == 0x444d4d59);

  data = 1;
  size_t sizeInBytes = 4 * 4;
  uint32_t dataOffsetInBytes = 1 * 4;

  int32_t adcData[4];
  pcieDevice.writeReg("WORD_ADC_ENA", &data);
  pcieDevice.readReg("AREA_DMAABLE", adcData, sizeInBytes, dataOffsetInBytes);
  BOOST_CHECK(adcData[0] == 1);
  BOOST_CHECK(adcData[1] == 4);
  BOOST_CHECK(adcData[2] == 9);
  BOOST_CHECK(adcData[3] == 16);
}

void DevMapTest::testDevMapReadArea() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data = 1;
  int32_t adcdata[4];
  uint32_t regOffset = 0;
  size_t dataSizeInBytes = 4 * 4;
  const uint8_t DMAAREA_BAR = 2;

  pcieDevice.writeReg("WORD_ADC_ENA", &data);
  pcieDevice.readArea(regOffset, adcdata, dataSizeInBytes, DMAAREA_BAR);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
  BOOST_CHECK(adcdata[2] == 4);
  BOOST_CHECK(adcdata[3] == 9);
}

void DevMapTest::testDevMapReadDMA() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data = 1;
  int32_t adcdata[6];
  size_t dataSizeInBytes = 6 * 4;
  pcieDevice.writeReg("WORD_ADC_ENA", &data);
  pcieDevice.readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
  BOOST_CHECK(adcdata[2] == 4);
  BOOST_CHECK(adcdata[3] == 9);
  BOOST_CHECK(adcdata[4] == 16);
  BOOST_CHECK(adcdata[5] == 25);
}

void DevMapTest::testDevMapReadDMAErrors() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  int32_t data;
  size_t dataSizeInBytes = 1 * 4;
  BOOST_CHECK_THROW(pcieDevice.readDMA("WORD_USER", &data, dataSizeInBytes),
                    mtca4u::ExcMappedDevice);
  try {
    pcieDevice.readDMA("WORD_USER", &data, dataSizeInBytes);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }
}

void DevMapTest::testDevMapWriteRegisterByName() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  int32_t input_data = 16;
  int32_t read_data;
  pcieDevice.writeReg("WORD_CLK_RST", &input_data);
  pcieDevice.readReg("WORD_CLK_RST", &read_data);
  BOOST_CHECK(read_data == 16);

  int32_t adcData[3] = { 1, 7, 9 };
  int32_t retreivedData[3];
  size_t sizeInBytes = 3 * 4;
  uint32_t dataOffsetInBytes = 1 * 4;

  pcieDevice.writeReg("AREA_DMAABLE", adcData, sizeInBytes, dataOffsetInBytes);
  pcieDevice.readReg("AREA_DMAABLE", retreivedData, sizeInBytes,
                     dataOffsetInBytes);
  BOOST_CHECK(retreivedData[0] == 1);
  BOOST_CHECK(retreivedData[1] == 7);
  BOOST_CHECK(retreivedData[2] == 9);
}

void DevMapTest::testDevMapWriteDMA() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data;
  size_t dataSizeInBytes = 1 * 4;
  BOOST_CHECK_THROW(pcieDevice.writeDMA("WORD_USER", &data, dataSizeInBytes),
                    mtca4u::ExcMappedDevice);
  try {
    pcieDevice.writeDMA("WORD_USER", &data, dataSizeInBytes);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }

  int32_t adcdata[6] = { 0 };
  dataSizeInBytes = 6 * 4;
  BOOST_CHECK_THROW(
      pcieDevice.writeDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes),
      mtca4u::ExcPcieDevice);
}

void DevMapTest::testDevMapCheckRegister() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  BOOST_CHECK_THROW(
      pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
      mtca4u::ExcMappedDevice);
  try {
    pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(
      pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
      mtca4u::ExcMappedDevice);
  try {
    pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }

  dataSize = 4;
  BOOST_CHECK_THROW(
      pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
      mtca4u::ExcMappedDevice);
  try {
    pcieDevice.writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }
}

void DevMapTest::testRegAccsorReadDMA() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data = 1;
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  non_dma_accesible_reg = pcieDevice.getRegisterAccessor("AREA_DMAABLE");
  BOOST_CHECK_THROW(non_dma_accesible_reg->readDMA(&data), mtca4u::ExcMappedDevice);

  pcieDevice.writeReg("WORD_ADC_ENA", &data);
  int32_t retreived_data[6];
  uint32_t size = 6 * 4;
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  area_dma = pcieDevice.getRegisterAccessor("AREA_DMA_VIA_DMA");
  area_dma->readDMA(retreived_data, size);
  BOOST_CHECK(retreived_data[0] == 0);
  BOOST_CHECK(retreived_data[1] == 1);
  BOOST_CHECK(retreived_data[2] == 4);
  BOOST_CHECK(retreived_data[3] == 9);
  BOOST_CHECK(retreived_data[4] == 16);
  BOOST_CHECK(retreived_data[5] == 25);
}

void DevMapTest::testRegAccsorCheckRegister() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  word_adc_ena = pcieDevice.getRegisterAccessor("WORD_ADC_ENA");
  BOOST_CHECK_THROW(word_adc_ena->writeReg(&data, dataSize, addRegOffset),
                    mtca4u::ExcMappedDevice);
  try {
    word_adc_ena->writeReg(&data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeReg(&data, dataSize, addRegOffset),
                    mtca4u::ExcMappedDevice);
  try {
    word_adc_ena->writeReg(&data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }

  dataSize = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeReg(&data, dataSize, addRegOffset),
                    mtca4u::ExcMappedDevice);
  try {
    word_adc_ena->writeReg(&data, dataSize, addRegOffset);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }
}

void DevMapTest::testRegAccsorWriteDMA() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data;
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  non_dma_accesible_reg = pcieDevice.getRegisterAccessor("WORD_USER");
  size_t dataSizeInBytes = 1 * 4;
  BOOST_CHECK_THROW(non_dma_accesible_reg->writeDMA(&data, dataSizeInBytes),
                    mtca4u::ExcMappedDevice);
  try {
    pcieDevice.writeDMA("WORD_USER", &data, dataSizeInBytes);
  }
  catch (mtca4u::ExcMappedDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcMappedDevice::EX_WRONG_PARAMETER);
  }
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  dma_accesible_reg = pcieDevice.getRegisterAccessor("AREA_DMA_VIA_DMA");
  int32_t adcdata[6] = { 0 };
  dataSizeInBytes = 6 * 4;
  BOOST_CHECK_THROW(dma_accesible_reg->writeDMA(adcdata, dataSizeInBytes),
                    mtca4u::ExcPcieDevice);
}

void DevMapTest::testRegAccsorReadReg() {

  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  word_clk_dummy = pcieDevice.getRegisterAccessor("WORD_CLK_DUMMY");
  int32_t data;
  word_clk_dummy->readReg(&data);
  BOOST_CHECK(data == 0x444d4d59);
}

void DevMapTest::testRegAccsorWriteReg() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  boost::shared_ptr<mtca4u::MappedDevice<mtca4u::PcieDevice>::RegisterAccessor>
  word_clk_rst = pcieDevice.getRegisterAccessor("WORD_CLK_RST");
  int32_t input_data = 16;
  int32_t read_data;
  word_clk_rst->writeReg(&input_data);
  word_clk_rst->readReg(&read_data);
  BOOST_CHECK(read_data == 16);
}

void DevMapTest::testDevMapReadRegister() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  uint32_t offset_word_clk_dummy = 0x0000003C;
  int32_t data;
  uint8_t bar = 0;
  pcieDevice.readReg(offset_word_clk_dummy, &data, bar);
  BOOST_CHECK(data == 0x444d4d59);
}

void DevMapTest::testDevMapWriteRegister() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  uint32_t offset_word_clk_reset = 0x00000040;
  int32_t input_data = 16;
  int32_t read_data;
  uint8_t bar = 0;
  pcieDevice.writeReg(offset_word_clk_reset, input_data, bar);
  pcieDevice.readReg(offset_word_clk_reset, &read_data, bar);
  BOOST_CHECK(read_data == 16);
}

void DevMapTest::testDeviceInfo() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  int slot, majorVersion, minorVersion;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  std::string deviceInfo;
  pcieDevice.readDeviceInfo(&deviceInfo);
  int nParametersConverted =
      sscanf(deviceInfo.c_str(), "SLOT: %d DRV VER: %d.%d", &slot,
             &majorVersion, &minorVersion);
  BOOST_CHECK(nParametersConverted == 3);
}

void DevMapTest::testReadBadReg() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);

  int32_t data;
  BOOST_CHECK_THROW(pcieDevice.readReg("NON_EXISTENT_REGISTER", &data),
                    mtca4u::ExcPcieDevice);
  try {
    pcieDevice.readReg("NON_EXISTENT_REGISTER", &data);
  }
  catch (mtca4u::ExcPcieDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcPcieDevice::EX_READ_ERROR);
  }
}

void DevMapTest::testWriteBadReg() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  int32_t data;
  BOOST_CHECK_THROW(pcieDevice.writeReg("BROKEN_WRITE", &data),
                    mtca4u::ExcPcieDevice);
  try {
    pcieDevice.writeReg("BROKEN_WRITE", &data);
  }
  catch (mtca4u::ExcPcieDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcPcieDevice::EX_WRITE_ERROR);
  }
}

void DevMapTest::testDMAReadSizeTooSmall() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> mtcaDevice;
  std::string dummyDevice = "/dev/mtcadummys0";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  mtcaDevice.openDev(dummyDevice, validMappingFile);

  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;

  BOOST_CHECK_THROW(
      mtcaDevice.readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes),
      mtca4u::ExcPcieDevice);
  try {
    mtcaDevice.readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  }
  catch (mtca4u::ExcPcieDevice& exception) {
    BOOST_CHECK(exception.getID() == mtca4u::ExcPcieDevice::EX_DMA_READ_ERROR);
  }
}

void DevMapTest::testDMAReadViaStruct() {
  mtca4u::MappedDevice<mtca4u::PcieDevice> pcieDevice;
  std::string dummyDevice = "/dev/llrfdummys4";
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  pcieDevice.openDev(dummyDevice, validMappingFile);
  /*
    mtca4u::DeviceFactory FactoryInstance =
    mtca4u::DeviceFactory::getInstance();
    mtca4u::devMap<mtca4u::PcieDevice> pcieDevice =
    FactoryInstance.createMappedDevice("DUMMY5");//,true);
    pcieDevice.openDev(pcieDevice._dmapElem.dev_name,pcieDevice._dmapElem.map_file_name);*/

  // mtca4u::PcieDevice ptr = static_cast<mtca4u::PcieDevice >(pcieDevice);
  int32_t data = 1;
  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;
  pcieDevice.writeReg("WORD_ADC_ENA", &data);
  pcieDevice.readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
}

void DevMapTest::testGetRegistersInModule() {


  mtca4u::DeviceFactory FactoryInstance = mtca4u::DeviceFactory::getInstance();
  mtca4u::MappedDevice<mtca4u::BaseDevice>* mappedDevice =
	FactoryInstance.createMappedDevice("DUMMYD0");

  std::list<mtca4u::mapFile::mapElem> registerInfoList =
      mappedDevice->getRegistersInModule("APP0");
  BOOST_CHECK(registerInfoList.size() == 4);
  std::list<mtca4u::mapFile::mapElem>::iterator registerInfo =
      registerInfoList.begin();
  BOOST_CHECK(registerInfo->reg_name == "MODULE0");
  BOOST_CHECK(registerInfo->reg_module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->reg_name == "MODULE1");
  BOOST_CHECK(registerInfo->reg_module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->reg_name == "WORD_SCRATCH");
  BOOST_CHECK(registerInfo->reg_module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->reg_name == "WORD_STATUS");
  BOOST_CHECK(registerInfo->reg_module == "APP0");

  delete mappedDevice;
}

#include <BaseDevice.h>
void DevMapTest::testGetRegisterAccessorsInModule() {
  mtca4u::MappedDevice<mtca4u::DummyDevice> mappedDevice;
  // this test only makes sense for mapp files
  std::string mapFileName = "goodMapFile.map";
  // the dummy device is opened with twice the map file name (use map file
  // instead of device node)
  mappedDevice.openDev(mapFileName, mapFileName);

  std::list<mtca4u::MappedDevice<mtca4u::DummyDevice>::RegisterAccessor>
  accessorList = mappedDevice.getRegisterAccessorsInModule("APP0");
  BOOST_CHECK(accessorList.size() == 4);
  std::list<mtca4u::MappedDevice<mtca4u::DummyDevice>::RegisterAccessor>::iterator
  accessor = accessorList.begin();
  BOOST_CHECK(accessor->getRegisterInfo().reg_name == "MODULE0");
  BOOST_CHECK(accessor->getRegisterInfo().reg_module == "APP0");
  ++accessor;
  BOOST_CHECK(accessor->getRegisterInfo().reg_name == "MODULE1");
  BOOST_CHECK(accessor->getRegisterInfo().reg_module == "APP0");
  ++accessor;
  BOOST_CHECK(accessor->getRegisterInfo().reg_name == "WORD_SCRATCH");
  BOOST_CHECK(accessor->getRegisterInfo().reg_module == "APP0");
  ++accessor;
  BOOST_CHECK(accessor->getRegisterInfo().reg_name == "WORD_STATUS");
  BOOST_CHECK(accessor->getRegisterInfo().reg_module == "APP0");
}

void DevMapTest::testAccessorForMuxedData() {
  // create mapped dummy device
  boost::shared_ptr<mtca4u::mapFile> registerMap =
	mtca4u::mapFileParser().parse("sequences.map");
  boost::shared_ptr<mtca4u::BaseDevice> ioDevice(new mtca4u::DummyDevice);
  ioDevice->openDev("sequences.map");

  mtca4u::mapFile::mapElem sequenceInfo;
  registerMap->getRegisterInfo("AREA_MULTIPLEXED_SEQUENCE_DMA", sequenceInfo,
                               "TEST");

  // Fill in the sequences
  std::vector<int16_t> ioBuffer(sequenceInfo.reg_size / sizeof(int16_t));

  for (size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i] = i;
  }

  ioDevice->writeArea(sequenceInfo.reg_address,
                      reinterpret_cast<int32_t*>(&(ioBuffer[0])),
                      sequenceInfo.reg_size, sequenceInfo.reg_bar);

  // create the mapped device
  mtca4u::MappedDevice<mtca4u::BaseDevice> mappedDevice;
  mappedDevice.openDev(ioDevice, registerMap);

  boost::shared_ptr<mtca4u::MultiplexedDataAccessor<double> > deMultiplexer =
      mappedDevice.getCustomAccessor<mtca4u::MultiplexedDataAccessor<double> >(
          "DMA", "TEST");

  deMultiplexer->read();

  int j = 0;
  for (size_t i = 0; i < 4; ++i) {
    for (size_t sequenceIndex = 0; sequenceIndex < 16; ++sequenceIndex) {
      BOOST_CHECK((*deMultiplexer)[sequenceIndex][i] == 4 * j++);
    }
  }

  BOOST_CHECK_THROW(deMultiplexer->write(), mtca4u::NotImplementedException)
}
