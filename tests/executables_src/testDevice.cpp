///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include <cstring>

#include "Device.h"
#include "PcieBackend.h"
#include "DeviceBackend.h"
#include "BackendFactory.h"
#include "MapFileParser.h"
#include "Exception.h"
#include "DummyRegisterAccessor.h"

namespace ChimeraTK{
  using namespace ChimeraTK;
}
using namespace boost::unit_test_framework;

class TestableDevice : public ChimeraTK::Device {
  public:
    boost::shared_ptr<ChimeraTK::DeviceBackend> getBackend() { return _deviceBackendPointer; };
};

class DeviceTest {
  public:
    void testDeviceReadRegisterByName();
    void testCompatDeviceReadRegisterByName();
    void testDeviceReadRegister();
    void testDeviceReadArea();
    void testDeviceReadDMA();
    void testDeviceWriteRegisterByName();
    void testDeviceWriteRegister();
    void testDeviceCheckRegister();
    void testRegAccsorReadDMA();
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
    void testDeviceCreation();
};

class DeviceTestSuite : public test_suite {
  public:
    DeviceTestSuite() : test_suite("Device class test suite") {
      ChimeraTK::BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
      boost::shared_ptr<DeviceTest> DeviceTestPtr(new DeviceTest());

      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadRegisterByName, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testCompatDeviceReadRegisterByName, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadRegister, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadArea, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadDMA, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceWriteRegisterByName, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceWriteRegister, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceCheckRegister, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorReadDMA, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorCheckRegister, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorReadReg, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorWriteReg, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceInfo, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testReadBadReg, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testWriteBadReg, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDMAReadSizeTooSmall, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDMAReadViaStruct, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceCreation, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testGetRegistersInModule, DeviceTestPtr) );
      add( BOOST_CLASS_TEST_CASE(&DeviceTest::testGetRegisterAccessorsInModule, DeviceTestPtr) );
    }
};

bool init_unit_test(){
  framework::master_test_suite().p_name.value = "Device class test suite";
  framework::master_test_suite().add(new DeviceTestSuite());

  return true;
}

void DeviceTest::testDeviceReadRegisterByName() {
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;
  device.open("DUMMYD2");
  boost::shared_ptr< ChimeraTK::DummyBackend > backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(
      ChimeraTK::BackendFactory::getInstance().createBackend("DUMMYD2") );

  ChimeraTK::DummyRegisterAccessor<int32_t> wordStatus(backend.get(),"APP0","WORD_STATUS");
  ChimeraTK::DummyRegisterAccessor<int32_t> module0(backend.get(),"APP0","MODULE0");

  int32_t data;
  std::vector<int32_t> dataVector;

  wordStatus = 0x444d4d59;
  data = device.read<int32_t>("APP0.WORD_STATUS");
  BOOST_CHECK(data == 0x444d4d59);

  wordStatus = -42;
  data = device.read<int32_t>("APP0.WORD_STATUS");
  BOOST_CHECK(data == -42);

  module0[0] = 120;
  module0[1] = 0xDEADBEEF;

  data = device.read<int32_t>("APP0/MODULE0");
  BOOST_CHECK(data == 120);

  dataVector = device.read<int32_t>("APP0/MODULE0",2,0);
  BOOST_CHECK(dataVector.size() == 2);
  BOOST_CHECK(dataVector[0] == 120);
  BOOST_CHECK(dataVector[1] == (signed)0xDEADBEEF);

  module0[0] = 66;
  module0[1] = -33333;

  dataVector = device.read<int32_t>("APP0/MODULE0",1,0);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == 66);

  dataVector = device.read<int32_t>("APP0/MODULE0",1,1);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == -33333);

  BOOST_CHECK_THROW(device.read<int32_t>("APP0/DOESNT_EXIST"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(device.read<int32_t>("DOESNT_EXIST/AT_ALL",1,0), ChimeraTK::logic_error);

}

void DeviceTest::testCompatDeviceReadRegisterByName() {
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);


  int32_t data = 0;
  device->readReg("WORD_CLK_DUMMY", &data);
  BOOST_CHECK(data == 0x444d4d59);

  data = 1;
  size_t sizeInBytes = 4 * 4;
  uint32_t dataOffsetInBytes = 1 * 4;

  int32_t adcData[4];
  device->writeReg("WORD_ADC_ENA", &data);
  device->readReg("AREA_DMAABLE", adcData, sizeInBytes, dataOffsetInBytes);
  BOOST_CHECK(adcData[0] == 1);
  BOOST_CHECK(adcData[1] == 4);
  BOOST_CHECK(adcData[2] == 9);
  BOOST_CHECK(adcData[3] == 16);

  // same with modules
  device = boost::shared_ptr<ChimeraTK::Device>( new ChimeraTK::Device());
  std::string validMappingFileWithModules = "mtcadummy.map";
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackendWithModules( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFileWithModules));
  device->open(testBackendWithModules);


  data = 0;
  device->readReg("WORD_CLK_DUMMY", "ADC", &data);
  BOOST_CHECK(data == 0x444d4d59);

  data = 0;
  device->readReg("ADC.WORD_CLK_DUMMY", &data);
  BOOST_CHECK(data == 0x444d4d59);

  BOOST_CHECK_THROW( device->readReg("WORD_CLK_DUMMY", "WRONG_MODULE", &data), ChimeraTK::logic_error );

  data = 1;
  sizeInBytes = 4 * 4;
  dataOffsetInBytes = 1 * 4;

  device->writeReg("WORD_ADC_ENA", "ADC", &data);
  device->readReg("AREA_DMAABLE", "ADC", adcData, sizeInBytes, dataOffsetInBytes);
  BOOST_CHECK(adcData[0] == 1);
  BOOST_CHECK(adcData[1] == 4);
  BOOST_CHECK(adcData[2] == 9);
  BOOST_CHECK(adcData[3] == 16);

}

void DeviceTest::testDeviceReadArea() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  int32_t data = 1;
  int32_t adcdata[4];
  uint32_t regOffset = 0;
  size_t dataSizeInBytes = 4 * 4;
  const uint8_t DMAAREA_BAR = 2;

  device->writeReg("WORD_ADC_ENA", &data);
  device->readArea(regOffset, adcdata, dataSizeInBytes, DMAAREA_BAR);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
  BOOST_CHECK(adcdata[2] == 4);
  BOOST_CHECK(adcdata[3] == 9);
}

void DeviceTest::testDeviceReadDMA() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  int32_t data = 1;
  int32_t adcdata[6];
  size_t dataSizeInBytes = 6 * 4;
  device->writeReg("WORD_ADC_ENA", &data);
  device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
  BOOST_CHECK(adcdata[2] == 4);
  BOOST_CHECK(adcdata[3] == 9);
  BOOST_CHECK(adcdata[4] == 16);
  BOOST_CHECK(adcdata[5] == 25);
}

void DeviceTest::testDeviceWriteRegisterByName() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  int32_t input_data = 16;
  int32_t read_data;
  device->writeReg("WORD_CLK_RST", &input_data);
  device->readReg("WORD_CLK_RST", &read_data);
  BOOST_CHECK(read_data == 16);

  int32_t adcData[3] = { 1, 7, 9 };
  int32_t retreivedData[3];
  size_t sizeInBytes = 3 * 4;
  uint32_t dataOffsetInBytes = 1 * 4;

  device->writeReg("AREA_DMAABLE", adcData, sizeInBytes, dataOffsetInBytes);
  device->readReg("AREA_DMAABLE", retreivedData, sizeInBytes, dataOffsetInBytes);
  BOOST_CHECK(retreivedData[0] == 1);
  BOOST_CHECK(retreivedData[1] == 7);
  BOOST_CHECK(retreivedData[2] == 9);
}

void DeviceTest::testDeviceCheckRegister() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  BOOST_CHECK_THROW( device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error );

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 4;
  BOOST_CHECK_THROW(device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error);
}

void DeviceTest::testRegAccsorReadDMA() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  int32_t data = 1;
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> non_dma_accesible_reg = device->getRegisterAccessor("AREA_DMAABLE");
  //BOOST_CHECK_THROW(non_dma_accesible_reg->readDMA(&data), ChimeraTK::DeviceException); // there is no distinction any more...

  device->writeReg("WORD_ADC_ENA", &data);
  int32_t retreived_data[6];
  uint32_t size = 6 * 4;
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> area_dma = device->getRegisterAccessor("AREA_DMA_VIA_DMA");
  area_dma->readDMA(retreived_data, size);
  BOOST_CHECK(retreived_data[0] == 0);
  BOOST_CHECK(retreived_data[1] == 1);
  BOOST_CHECK(retreived_data[2] == 4);
  BOOST_CHECK(retreived_data[3] == 9);
  BOOST_CHECK(retreived_data[4] == 16);
  BOOST_CHECK(retreived_data[5] == 25);
}

void DeviceTest::testRegAccsorCheckRegister() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor>
  word_adc_ena = device->getRegisterAccessor("WORD_ADC_ENA");
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);
}

void DeviceTest::testRegAccsorReadReg() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor>
  word_clk_dummy = device->getRegisterAccessor("WORD_CLK_DUMMY");
  int32_t data = 0;
  word_clk_dummy->readRaw(&data);
  BOOST_CHECK(data == 0x444d4d59);
}

void DeviceTest::testRegAccsorWriteReg() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor>
  word_clk_rst = device->getRegisterAccessor("WORD_CLK_RST");
  int32_t input_data = 16;
  int32_t read_data;
  word_clk_rst->writeRaw(&input_data);
  word_clk_rst->readRaw(&read_data);
  BOOST_CHECK(read_data == 16);
}

void DeviceTest::testDeviceReadRegister() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  uint32_t offset_word_clk_dummy = 0x0000003C;
  int32_t data = 0;
  uint8_t bar = 0;
  device->readReg(offset_word_clk_dummy, &data, bar);
  BOOST_CHECK(data == 0x444d4d59);
}

void DeviceTest::testDeviceWriteRegister() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  int32_t input_data = 16;
  int32_t read_data;
  uint8_t bar = 0;
  uint32_t offset_word_clk_reset = 0x00000040;
  device->writeReg(offset_word_clk_reset, input_data, bar);
  device->readReg(offset_word_clk_reset, &read_data, bar);
  BOOST_CHECK(read_data == 16);
}

void DeviceTest::testDeviceInfo() {
  int slot, majorVersion, minorVersion;
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  std::string deviceInfo;
  deviceInfo = device->readDeviceInfo();
  int nParametersConverted =
      sscanf(deviceInfo.c_str(), "SLOT: %d DRV VER: %d.%d", &slot,
          &majorVersion, &minorVersion);
  BOOST_CHECK(nParametersConverted == 3);
}

void DeviceTest::testReadBadReg() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);

  int32_t data = 0;
  BOOST_CHECK_THROW(device->readReg("NON_EXISTENT_REGISTER", &data), ChimeraTK::runtime_error);
}

void DeviceTest::testWriteBadReg() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  int32_t data = 0;
  BOOST_CHECK_THROW(device->writeReg("BROKEN_WRITE", &data), ChimeraTK::runtime_error);
}

void DeviceTest::testDMAReadSizeTooSmall() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/mtcadummys0",validMappingFile));
  device->open(testBackend);
  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;

  BOOST_CHECK_THROW(
      device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes),
      ChimeraTK::logic_error);
}

void DeviceTest::testDMAReadViaStruct() {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device ( new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend ( new ChimeraTK::PcieBackend("/dev/llrfdummys4",validMappingFile));
  device->open(testBackend);
  int32_t data = 1;
  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;
  device->writeReg("WORD_ADC_ENA", &data);
  device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
}

void DeviceTest::testGetRegistersInModule() {

  boost::shared_ptr<ChimeraTK::Device> device( new ChimeraTK::Device());
  device->open("DUMMYD1");
  std::list<ChimeraTK::RegisterInfoMap::RegisterInfo> registerInfoList = device->getRegistersInModule("APP0");

  BOOST_CHECK(registerInfoList.size() == 4);
  std::list<ChimeraTK::RegisterInfoMap::RegisterInfo>::iterator registerInfo = registerInfoList.begin();
  BOOST_CHECK(registerInfo->name == "MODULE0");
  BOOST_CHECK(registerInfo->module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->name == "MODULE1");
  BOOST_CHECK(registerInfo->module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->name == "WORD_SCRATCH");
  BOOST_CHECK(registerInfo->module == "APP0");
  ++registerInfo;
  BOOST_CHECK(registerInfo->name == "WORD_STATUS");
  BOOST_CHECK(registerInfo->module == "APP0");

}

void DeviceTest::testGetRegisterAccessorsInModule() {
  boost::shared_ptr<ChimeraTK::Device> device( new ChimeraTK::Device());
  device->open("DUMMYD1");
  // this test only makes sense for mapp files
  //std::string mapFileName = "goodMapFile.map";
  // the dummy device is opened with twice the map file name (use map file
  // instead of device node)

  std::list< boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> > accessorList = device->getRegisterAccessorsInModule("APP0");
  BOOST_CHECK(accessorList.size() == 4);

  auto accessor = accessorList.begin();
  BOOST_CHECK((*accessor)->getRegisterInfo().name == "MODULE0");
  BOOST_CHECK((*accessor)->getRegisterInfo().module == "APP0");
  ++accessor;
  BOOST_CHECK((*accessor)->getRegisterInfo().name == "MODULE1");
  BOOST_CHECK((*accessor)->getRegisterInfo().module == "APP0");
  ++accessor;
  BOOST_CHECK((*accessor)->getRegisterInfo().name == "WORD_SCRATCH");
  BOOST_CHECK((*accessor)->getRegisterInfo().module == "APP0");
  ++accessor;
  BOOST_CHECK((*accessor)->getRegisterInfo().name == "WORD_STATUS");
  BOOST_CHECK((*accessor)->getRegisterInfo().module == "APP0");
}

void DeviceTest::testDeviceCreation() {
  std::string initialDmapFilePath = ChimeraTK::BackendFactory::getInstance().getDMapFilePath();
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("dMapDir/testRelativePaths.dmap");

  ChimeraTK::Device device1;
  BOOST_CHECK( device1.isOpened() == false );
  device1.open("PCIE0");
  BOOST_CHECK( device1.isOpened() == true );
  BOOST_CHECK_NO_THROW(device1.open("PCIE0"));
  {// scope to have a device which goes out of scope
      ChimeraTK::Device device1a;
      // open the same backend than device1
      device1a.open("PCIE0");
      BOOST_CHECK( device1a.isOpened() == true );
  }
  // check that device1 has not been closed by device 1a going out of scope
  BOOST_CHECK( device1.isOpened() == true );

  ChimeraTK::Device device1b;
  // open the same backend than device1
  device1b.open("PCIE0");
  // open another backend with the same device //ugly, might be deprecated soon
  device1b.open("PCIE2");
  // check that device1 has not been closed by device 1b being reassigned
  BOOST_CHECK( device1.isOpened() == true );

  ChimeraTK::Device device2;
  BOOST_CHECK( device2.isOpened() == false );
  device2.open("PCIE1");
  BOOST_CHECK( device2.isOpened() == true );
  BOOST_CHECK_NO_THROW(device2.open("PCIE1"));
  BOOST_CHECK( device2.isOpened() == true );

  ChimeraTK::Device device3;
  BOOST_CHECK( device3.isOpened() == false );
  BOOST_CHECK_NO_THROW(device3.open("DUMMYD0"));
  BOOST_CHECK( device3.isOpened() == true );
  ChimeraTK::Device device4;
  BOOST_CHECK( device4.isOpened() == false );
  BOOST_CHECK_NO_THROW(device4.open("DUMMYD1"));
  BOOST_CHECK( device4.isOpened() == true );

  // check if opening without alias name fails
  TestableDevice device5;
  BOOST_CHECK( device5.isOpened() == false );
  BOOST_CHECK_THROW( device5.open(), ChimeraTK::logic_error );
  BOOST_CHECK( device5.isOpened() == false );
  BOOST_CHECK_THROW(device5.open(), ChimeraTK::logic_error);
  BOOST_CHECK( device5.isOpened() == false );

  // check if opening device with different backend keeps old backend open.
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD0"));
  BOOST_CHECK( device5.isOpened() == true );
  auto backend5 = device5.getBackend();
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD1"));
  BOOST_CHECK( backend5->isOpen() );    // backend5 is still the current backend of device5
  BOOST_CHECK( device5.isOpened() == true );

  // check closing and opening again
  backend5 = device5.getBackend();
  BOOST_CHECK( backend5->isOpen() );
  BOOST_CHECK( device5.isOpened() == true );
  device5.close();
  BOOST_CHECK( device5.isOpened() == false );
  BOOST_CHECK( !backend5->isOpen() );
  device5.open();
  BOOST_CHECK( device5.isOpened() == true );
  BOOST_CHECK( backend5->isOpen() );

  //Now that we are done with the tests, move the factory to the state it was in
  //before we started
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath(initialDmapFilePath);

}

#ifdef _0
void DeviceTest::testAccessorForMuxedData() {
  // create dummy device without using the factory
  boost::shared_ptr<ChimeraTK::RegisterInfoMap> registerMap =
      ChimeraTK::mapFileParser().parse("sequences.map");
  boost::shared_ptr<ChimeraTK::DeviceBackend> backend(new ChimeraTK::DummyBackend);
  backend->open("sequences.map");

  ChimeraTK::RegisterInfoMap::RegisterInfo sequenceInfo;
  registerMap->getRegisterInfo("AREA_MULTIPLEXED_SEQUENCE_DMA", sequenceInfo,
      "TEST");

  // Fill in the sequences
  std::vector<int16_t> ioBuffer(sequenceInfo.reg_size / sizeof(int16_t));

  for (size_t i = 0; i < ioBuffer.size(); ++i) {
    ioBuffer[i] = i;
  }

  backend->writeArea(sequenceInfo.reg_address,
      reinterpret_cast<int32_t*>(&(ioBuffer[0])),
      sequenceInfo.reg_size, sequenceInfo.reg_bar);

  // mor create the device itself
  ChimeraTK::Device device;
  device.open(backend, registerMap);

  boost::shared_ptr<ChimeraTK::MultiplexedDataAccessor<double> > deMultiplexer =
      device.getCustomAccessor<ChimeraTK::MultiplexedDataAccessor<double> >(
          "DMA", "TEST");

  deMultiplexer->read();

  int j = 0;
  for (size_t i = 0; i < 4; ++i) {
    for (size_t sequenceIndex = 0; sequenceIndex < 16; ++sequenceIndex) {
      BOOST_CHECK((*deMultiplexer)[sequenceIndex][i] == 4 * j++);
    }
  }

  BOOST_CHECK_THROW(deMultiplexer->write(), ChimeraTK::NotImplementedException)
}
#endif
