#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DeviceTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <cstring>

#include "BackendFactory.h"
#include "Device.h"
#include "DeviceBackend.h"
#include "DummyRegisterAccessor.h"
#include "Exception.h"
#include "MapFileParser.h"
#include "PcieBackend.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}

class TestableDevice : public ChimeraTK::Device {
 public:
  boost::shared_ptr<ChimeraTK::DeviceBackend> getBackend() { return _deviceBackendPointer; }
};

BOOST_AUTO_TEST_SUITE(DeviceTestSuite)

BOOST_AUTO_TEST_CASE(testConvenienceReadWrite) {
  ChimeraTK::setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<ChimeraTK::DummyBackend> backend = boost::dynamic_pointer_cast<ChimeraTK::DummyBackend>(
      ChimeraTK::BackendFactory::getInstance().createBackend("DUMMYD2"));

  ChimeraTK::DummyRegisterAccessor<int32_t> wordStatus(backend.get(), "APP0", "WORD_STATUS");
  ChimeraTK::DummyRegisterAccessor<int32_t> module0(backend.get(), "APP0", "MODULE0");

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

  dataVector = device.read<int32_t>("APP0/MODULE0", 2, 0);
  BOOST_CHECK(dataVector.size() == 2);
  BOOST_CHECK(dataVector[0] == 120);
  BOOST_CHECK(dataVector[1] == static_cast<signed>(0xDEADBEEF));

  module0[0] = 66;
  module0[1] = -33333;

  dataVector = device.read<int32_t>("APP0/MODULE0", 1, 0);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == 66);

  dataVector = device.read<int32_t>("APP0/MODULE0", 1, 1);
  BOOST_CHECK(dataVector.size() == 1);
  BOOST_CHECK(dataVector[0] == -33333);

  BOOST_CHECK_THROW(device.read<int32_t>("APP0/DOESNT_EXIST"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(device.read<int32_t>("DOESNT_EXIST/AT_ALL", 1, 0), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testDeviceCreation) {
  std::string initialDmapFilePath = ChimeraTK::BackendFactory::getInstance().getDMapFilePath();
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath("dMapDir/testRelativePaths.dmap");

  ChimeraTK::Device device1;
  BOOST_CHECK(device1.isOpened() == false);
  device1.open("DUMMYD0");
  BOOST_CHECK(device1.isOpened() == true);
  BOOST_CHECK_NO_THROW(device1.open("DUMMYD0"));
  { // scope to have a device which goes out of scope
    ChimeraTK::Device device1a;
    // open the same backend than device1
    device1a.open("DUMMYD0");
    BOOST_CHECK(device1a.isOpened() == true);
  }
  // check that device1 has not been closed by device 1a going out of scope
  BOOST_CHECK(device1.isOpened() == true);

  ChimeraTK::Device device1b;
  // open the same backend than device1
  device1b.open("DUMMYD0");
  // open another backend with the same device //ugly, might be deprecated soon
  device1b.open("DUMMYD0");
  // check that device1 has not been closed by device 1b being reassigned
  BOOST_CHECK(device1.isOpened() == true);

  ChimeraTK::Device device2;
  BOOST_CHECK(device2.isOpened() == false);
  device2.open("DUMMYD1");
  BOOST_CHECK(device2.isOpened() == true);
  BOOST_CHECK_NO_THROW(device2.open("DUMMYD1"));
  BOOST_CHECK(device2.isOpened() == true);

  ChimeraTK::Device device3;
  BOOST_CHECK(device3.isOpened() == false);
  BOOST_CHECK_NO_THROW(device3.open("DUMMYD0"));
  BOOST_CHECK(device3.isOpened() == true);
  ChimeraTK::Device device4;
  BOOST_CHECK(device4.isOpened() == false);
  BOOST_CHECK_NO_THROW(device4.open("DUMMYD1"));
  BOOST_CHECK(device4.isOpened() == true);

  // check if opening without alias name fails
  TestableDevice device5;
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK_THROW(device5.open(), ChimeraTK::logic_error);
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK_THROW(device5.open(), ChimeraTK::logic_error);
  BOOST_CHECK(device5.isOpened() == false);

  // check if opening device with different backend keeps old backend open.
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD0"));
  BOOST_CHECK(device5.isOpened() == true);
  auto backend5 = device5.getBackend();
  BOOST_CHECK_NO_THROW(device5.open("DUMMYD1"));
  BOOST_CHECK(backend5->isOpen()); // backend5 is still the current backend of device5
  BOOST_CHECK(device5.isOpened() == true);

  // check closing and opening again
  backend5 = device5.getBackend();
  BOOST_CHECK(backend5->isOpen());
  BOOST_CHECK(device5.isOpened() == true);
  device5.close();
  BOOST_CHECK(device5.isOpened() == false);
  BOOST_CHECK(!backend5->isOpen());
  device5.open();
  BOOST_CHECK(device5.isOpened() == true);
  BOOST_CHECK(backend5->isOpen());

  // Now that we are done with the tests, move the factory to the state it was
  // in before we started
  ChimeraTK::BackendFactory::getInstance().setDMapFilePath(initialDmapFilePath);
}

BOOST_AUTO_TEST_CASE(testDeviceInfo) {
  ChimeraTK::Device device;
  device.open("DUMMYD3");
  std::string deviceInfo = device.readDeviceInfo();
  std::cout << deviceInfo << std::endl;
  BOOST_CHECK(deviceInfo.substr(0, 31) == "DummyBackend with mapping file ");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

BOOST_AUTO_TEST_CASE(testCompatibilityLayer1) {
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
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
  device = boost::shared_ptr<ChimeraTK::Device>(new ChimeraTK::Device());
  std::string validMappingFileWithModules = "mtcadummy.map";
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackendWithModules(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFileWithModules));
  device->open(testBackendWithModules);

  data = 0;
  device->readReg("WORD_CLK_DUMMY", "ADC", &data);
  BOOST_CHECK(data == 0x444d4d59);

  data = 0;
  device->readReg("ADC.WORD_CLK_DUMMY", &data);
  BOOST_CHECK(data == 0x444d4d59);

  BOOST_CHECK_THROW(device->readReg("WORD_CLK_DUMMY", "WRONG_MODULE", &data), ChimeraTK::logic_error);

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

BOOST_AUTO_TEST_CASE(testCompatibilityLayer2) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
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

BOOST_AUTO_TEST_CASE(testCompatibilityLayer3) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
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

BOOST_AUTO_TEST_CASE(testCompatibilityLayer4) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  int32_t input_data = 16;
  int32_t read_data;
  device->writeReg("WORD_CLK_RST", &input_data);
  device->readReg("WORD_CLK_RST", &read_data);
  BOOST_CHECK(read_data == 16);

  int32_t adcData[3] = {1, 7, 9};
  int32_t retreivedData[3];
  size_t sizeInBytes = 3 * 4;
  uint32_t dataOffsetInBytes = 1 * 4;

  device->writeReg("AREA_DMAABLE", adcData, sizeInBytes, dataOffsetInBytes);
  device->readReg("AREA_DMAABLE", retreivedData, sizeInBytes, dataOffsetInBytes);
  BOOST_CHECK(retreivedData[0] == 1);
  BOOST_CHECK(retreivedData[1] == 7);
  BOOST_CHECK(retreivedData[2] == 9);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer5) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  BOOST_CHECK_THROW(device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 4;
  BOOST_CHECK_THROW(device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer6) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);

  int32_t data = 1;
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> non_dma_accesible_reg =
      device->getRegisterAccessor("AREA_DMAABLE");
  // BOOST_CHECK_THROW(non_dma_accesible_reg->readDMA(&data),
  // ChimeraTK::DeviceException); // there is no distinction any more...

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

BOOST_AUTO_TEST_CASE(testCompatibilityLayer7) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);

  size_t dataSize = 4;
  uint32_t addRegOffset = 3;
  int32_t data = 1;
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> word_adc_ena = device->getRegisterAccessor("WORD_ADC_ENA");
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 3;
  addRegOffset = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);

  dataSize = 4;
  BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer8) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> word_clk_dummy = device->getRegisterAccessor("WORD_CLK_DUMMY");
  int32_t data = 0;
  word_clk_dummy->readRaw(&data);
  BOOST_CHECK(data == 0x444d4d59);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer9) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  boost::shared_ptr<ChimeraTK::Device::RegisterAccessor> word_clk_rst = device->getRegisterAccessor("WORD_CLK_RST");
  int32_t input_data = 16;
  int32_t read_data;
  word_clk_rst->writeRaw(&input_data);
  word_clk_rst->readRaw(&read_data);
  BOOST_CHECK(read_data == 16);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer10) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  uint32_t offset_word_clk_dummy = 0x0000003C;
  int32_t data = 0;
  uint8_t bar = 0;
  device->readReg(offset_word_clk_dummy, &data, bar);
  BOOST_CHECK(data == 0x444d4d59);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer11) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  int32_t input_data = 16;
  int32_t read_data;
  uint8_t bar = 0;
  uint32_t offset_word_clk_reset = 0x00000040;
  device->writeReg(offset_word_clk_reset, input_data, bar);
  device->readReg(offset_word_clk_reset, &read_data, bar);
  BOOST_CHECK(read_data == 16);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer12) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);

  int32_t data = 0;
  BOOST_CHECK_THROW(device->readReg("NON_EXISTENT_REGISTER", &data), ChimeraTK::runtime_error);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer13) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  int32_t data = 0;
  BOOST_CHECK_THROW(device->writeReg("BROKEN_WRITE", &data), ChimeraTK::runtime_error);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer14) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/mtcadummys0", validMappingFile));
  device->open(testBackend);
  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;

  BOOST_CHECK_THROW(device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer15) {
  std::string validMappingFile = "mtcadummy_withoutModules.map";
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  boost::shared_ptr<ChimeraTK::DeviceBackend> testBackend(
      new ChimeraTK::PcieBackend("/dev/llrfdummys4", validMappingFile));
  device->open(testBackend);
  int32_t data = 1;
  int32_t adcdata[2];
  size_t dataSizeInBytes = 2 * 4;
  device->writeReg("WORD_ADC_ENA", &data);
  device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
  BOOST_CHECK(adcdata[0] == 0);
  BOOST_CHECK(adcdata[1] == 1);
}

BOOST_AUTO_TEST_CASE(testCompatibilityLayer16) {
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
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

BOOST_AUTO_TEST_CASE(testCompatibilityLayer17) {
  boost::shared_ptr<ChimeraTK::Device> device(new ChimeraTK::Device());
  device->open("DUMMYD1");
  // this test only makes sense for mapp files
  // std::string mapFileName = "goodMapFile.map";
  // the dummy device is opened with twice the map file name (use map file
  // instead of device node)

  std::list<boost::shared_ptr<ChimeraTK::Device::RegisterAccessor>> accessorList =
      device->getRegisterAccessorsInModule("APP0");
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

struct DysfunctDummy : public ChimeraTK::DummyBackend {
  std::atomic<bool> _hasErrors = {false};
  bool isFunctional() const override { return _opened && !_hasErrors; }
  DysfunctDummy (std::string mapFileName) : DummyBackend(mapFileName){
    _hasErrors = true;
  }
  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new DysfunctDummy(parameters["map"]));
  }
};

BOOST_AUTO_TEST_CASE(testIsFunctional) {
  ChimeraTK::Device d;
  // a disconnected device is not functional
  BOOST_CHECK(d.isFunctional() == false);

  d.open("DUMMYD1");
  BOOST_CHECK(d.isFunctional() == true);

  d.close();
  BOOST_CHECK(d.isFunctional() == false);

  ChimeraTK::BackendFactory::getInstance().registerBackendType("DysfunctDummy", &DysfunctDummy::createInstance);
  ChimeraTK::Device d2("(DysfunctDummy?map=goodMapFile.map)");
  d2.open();
  // A device can be opened but dysfyunctional if there are errors
  BOOST_CHECK(d2.isOpened() == true);
  BOOST_CHECK(d2.isFunctional() == false);
}

#pragma GCC diagnostic pop

BOOST_AUTO_TEST_SUITE_END()
