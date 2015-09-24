#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <cstring>

#include "Device.h"
#include "PcieBackend.h"
#include "DeviceException.h"
#include "PcieBackendException.h"
#include "DeviceBackend.h"
#include "MultiplexedDataAccessor.h"
#include "BackendFactory.h"
#include "MapFileParser.h"

using namespace boost::unit_test_framework;

class DeviceTest {
public:
	void testDeviceReadRegisterByName();
	void testDeviceReadRegister();
	void testDeviceReadArea();
	void testDeviceReadDMA();
	void testDeviceReadDMAErrors();
	void testDeviceWriteRegisterByName();
	void testDeviceWriteRegister();
	void testDeviceWriteDMA();
	void testDeviceCheckRegister();
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

class DeviceTestSuite : public test_suite {
public:
	DeviceTestSuite() : test_suite("Device class test suite") {

		boost::shared_ptr<DeviceTest> DeviceTestPtr(new DeviceTest());

		test_case* testDeviceReadRegisterByName = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceReadRegisterByName, DeviceTestPtr);

		test_case* testDeviceReadRegister = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceReadRegister, DeviceTestPtr);

		test_case* testDeviceReadArea =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadArea, DeviceTestPtr);

		test_case* testDeviceReadDMA =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceReadDMA, DeviceTestPtr);

		test_case* testDeviceReadDMAErrors = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceReadDMAErrors, DeviceTestPtr);

		test_case* testDeviceWriteRegisterByName = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceWriteRegisterByName, DeviceTestPtr);

		test_case* testDeviceWriteRegister = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceWriteRegister, DeviceTestPtr);

		test_case* testDeviceWriteDMA =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceWriteDMA, DeviceTestPtr);

		test_case* testDeviceCheckRegister = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDeviceCheckRegister, DeviceTestPtr);

		test_case* testRegAccsorReadDMA =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorReadDMA, DeviceTestPtr);

		test_case* testRegAccsorCheckRegister = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testRegAccsorCheckRegister, DeviceTestPtr);

		test_case* testRegAccsorWriteDMA = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testRegAccsorWriteDMA, DeviceTestPtr);

		test_case* testRegAccsorReadReg =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testRegAccsorReadReg, DeviceTestPtr);

		test_case* testRegAccsorWriteReg = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testRegAccsorWriteReg, DeviceTestPtr);

		test_case* testDeviceInfo =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testDeviceInfo, DeviceTestPtr);

		test_case* testReadBadReg =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testReadBadReg, DeviceTestPtr);

		test_case* testWriteBadReg =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testWriteBadReg, DeviceTestPtr);

		test_case* testDMAReadSizeTooSmall = BOOST_CLASS_TEST_CASE(
				&DeviceTest::testDMAReadSizeTooSmall, DeviceTestPtr);

		test_case* testDMAReadViaStruct =
				BOOST_CLASS_TEST_CASE(&DeviceTest::testDMAReadViaStruct, DeviceTestPtr);

		add(testDeviceReadRegisterByName);
		add(testDeviceReadRegister);
		add(testDeviceReadArea);
		add(testDeviceReadDMA);
		add(testDeviceReadDMAErrors);
		add(testDeviceWriteRegisterByName);
		add(testDeviceWriteRegister);
		add(testDeviceWriteDMA);
		add(testDeviceCheckRegister);
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

		add(BOOST_CLASS_TEST_CASE(&DeviceTest::testGetRegistersInModule,
				DeviceTestPtr));
		add(BOOST_CLASS_TEST_CASE(&DeviceTest::testGetRegisterAccessorsInModule,
				DeviceTestPtr));
		//add(BOOST_CLASS_TEST_CASE(&DeviceTest::testAccessorForMuxedData,
				//                          DeviceTestPtr));
	}
};
test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value = "Device class test suite";
	framework::master_test_suite().add(new DeviceTestSuite());

	return NULL;
}

void DeviceTest::testDeviceReadRegisterByName() {
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);


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
}

void DeviceTest::testDeviceReadArea() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

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
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

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

void DeviceTest::testDeviceReadDMAErrors() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	int32_t data = 0;
	size_t dataSizeInBytes = 1 * 4;
	BOOST_CHECK_THROW(device->readDMA("WORD_USER", &data, dataSizeInBytes),
			mtca4u::DeviceException);
	try {
		device->readDMA("WORD_USER", &data, dataSizeInBytes);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}
}

void DeviceTest::testDeviceWriteRegisterByName() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
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
	device->readReg("AREA_DMAABLE", retreivedData, sizeInBytes,
			dataOffsetInBytes);
	BOOST_CHECK(retreivedData[0] == 1);
	BOOST_CHECK(retreivedData[1] == 7);
	BOOST_CHECK(retreivedData[2] == 9);
}

void DeviceTest::testDeviceWriteDMA() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	int32_t data = 0;
	size_t dataSizeInBytes = 1 * 4;
	BOOST_CHECK_THROW(device->writeDMA("WORD_USER", &data, dataSizeInBytes),
			mtca4u::DeviceException);
	try {
		device->writeDMA("WORD_USER", &data, dataSizeInBytes);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}

	int32_t adcdata[6] = { 0 };
	dataSizeInBytes = 6 * 4;
	BOOST_CHECK_THROW(
			device->writeDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes),
			mtca4u::PcieBackendException);
}

void DeviceTest::testDeviceCheckRegister() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	size_t dataSize = 4;
	uint32_t addRegOffset = 3;
	int32_t data = 1;
	BOOST_CHECK_THROW(
			device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}

	dataSize = 3;
	addRegOffset = 4;
	BOOST_CHECK_THROW(
			device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}

	dataSize = 4;
	BOOST_CHECK_THROW(
			device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		device->writeReg("WORD_ADC_ENA", &data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}
}

void DeviceTest::testRegAccsorReadDMA() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	int32_t data = 1;
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	non_dma_accesible_reg = device->getRegisterAccessor("AREA_DMAABLE");
	BOOST_CHECK_THROW(non_dma_accesible_reg->readDMA(&data), mtca4u::DeviceException);

	device->writeReg("WORD_ADC_ENA", &data);
	int32_t retreived_data[6];
	uint32_t size = 6 * 4;
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	area_dma = device->getRegisterAccessor("AREA_DMA_VIA_DMA");
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
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	size_t dataSize = 4;
	uint32_t addRegOffset = 3;
	int32_t data = 1;
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	word_adc_ena = device->getRegisterAccessor("WORD_ADC_ENA");
	BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		word_adc_ena->writeRaw(&data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}

	dataSize = 3;
	addRegOffset = 4;
	BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		word_adc_ena->writeRaw(&data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}

	dataSize = 4;
	BOOST_CHECK_THROW(word_adc_ena->writeRaw(&data, dataSize, addRegOffset),
			mtca4u::DeviceException);
	try {
		word_adc_ena->writeRaw(&data, dataSize, addRegOffset);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}
}

void DeviceTest::testRegAccsorWriteDMA() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	int32_t data = 0;
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	non_dma_accesible_reg = device->getRegisterAccessor("WORD_USER");
	size_t dataSizeInBytes = 1 * 4;
	BOOST_CHECK_THROW(non_dma_accesible_reg->writeDMA(&data, dataSizeInBytes),
			mtca4u::DeviceException);
	try {
		device->writeDMA("WORD_USER", &data, dataSizeInBytes);
	}
	catch (mtca4u::DeviceException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::DeviceException::EX_WRONG_PARAMETER);
	}
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	dma_accesible_reg = device->getRegisterAccessor("AREA_DMA_VIA_DMA");
	int32_t adcdata[6] = { 0 };
	dataSizeInBytes = 6 * 4;
	BOOST_CHECK_THROW(dma_accesible_reg->writeDMA(adcdata, dataSizeInBytes),
			mtca4u::PcieBackendException);
}

void DeviceTest::testRegAccsorReadReg() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	word_clk_dummy = device->getRegisterAccessor("WORD_CLK_DUMMY");
	int32_t data = 0;
	word_clk_dummy->readRaw(&data);
	BOOST_CHECK(data == 0x444d4d59);
}

void DeviceTest::testRegAccsorWriteReg() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	word_clk_rst = device->getRegisterAccessor("WORD_CLK_RST");
	int32_t input_data = 16;
	int32_t read_data;
	word_clk_rst->writeRaw(&input_data);
	word_clk_rst->readRaw(&read_data);
	BOOST_CHECK(read_data == 16);
}

void DeviceTest::testDeviceReadRegister() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	uint32_t offset_word_clk_dummy = 0x0000003C;
	int32_t data = 0;
	uint8_t bar = 0;
	device->readReg(offset_word_clk_dummy, &data, bar);
	BOOST_CHECK(data == 0x444d4d59);
}

void DeviceTest::testDeviceWriteRegister() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
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
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	std::string deviceInfo;
	deviceInfo = device->readDeviceInfo();
	int nParametersConverted =
			sscanf(deviceInfo.c_str(), "SLOT: %d DRV VER: %d.%d", &slot,
					&majorVersion, &minorVersion);
	BOOST_CHECK(nParametersConverted == 3);
}

void DeviceTest::testReadBadReg() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);

	int32_t data = 0;
	BOOST_CHECK_THROW(device->readReg("NON_EXISTENT_REGISTER", &data),
			mtca4u::PcieBackendException);
	try {
		device->readReg("NON_EXISTENT_REGISTER", &data);
	}
	catch (mtca4u::PcieBackendException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::PcieBackendException::EX_READ_ERROR);
	}
}

void DeviceTest::testWriteBadReg() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	int32_t data = 0;
	BOOST_CHECK_THROW(device->writeReg("BROKEN_WRITE", &data),
			mtca4u::PcieBackendException);
	try {
		device->writeReg("BROKEN_WRITE", &data);
	}
	catch (mtca4u::PcieBackendException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::PcieBackendException::EX_WRITE_ERROR);
	}
}

void DeviceTest::testDMAReadSizeTooSmall() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	int32_t adcdata[2];
	size_t dataSizeInBytes = 2 * 4;

	BOOST_CHECK_THROW(
			device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes),
			mtca4u::PcieBackendException);
	try {
		device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
	}
	catch (mtca4u::PcieBackendException& exception) {
		BOOST_CHECK(exception.getID() == mtca4u::PcieBackendException::EX_DMA_READ_ERROR);
	}
}

void DeviceTest::testDMAReadViaStruct() {
	std::string validMappingFile = "mtcadummy_withoutModules.map";
	boost::shared_ptr<mtca4u::Device> device ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testBackend ( new mtca4u::PcieBackend(".","llrfdummys4",parameters));
	mtca4u::mapFileParser fileParser;
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMapping = fileParser.parse(validMappingFile);
	device->open(testBackend, registerMapping);
	int32_t data = 1;
	int32_t adcdata[2];
	size_t dataSizeInBytes = 2 * 4;
	device->writeReg("WORD_ADC_ENA", &data);
	device->readDMA("AREA_DMA_VIA_DMA", adcdata, dataSizeInBytes);
	BOOST_CHECK(adcdata[0] == 0);
	BOOST_CHECK(adcdata[1] == 1);
}

void DeviceTest::testGetRegistersInModule() {

	boost::shared_ptr<mtca4u::Device> device( new mtca4u::Device());
	device->open("DUMMYD1");
	std::list<mtca4u::RegisterInfoMap::RegisterInfo> registerInfoList =
			device->getRegistersInModule("APP0");

	BOOST_CHECK(registerInfoList.size() == 4);
	std::list<mtca4u::RegisterInfoMap::RegisterInfo>::iterator registerInfo =
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

	//delete device;
}

#include <DeviceBackend.h>
void DeviceTest::testGetRegisterAccessorsInModule() {
	boost::shared_ptr<mtca4u::Device> device( new mtca4u::Device());
	device->open("DUMMYD1");
	// this test only makes sense for mapp files
	//std::string mapFileName = "goodMapFile.map";
	// the dummy device is opened with twice the map file name (use map file
	// instead of device node)
	std::list<mtca4u::Device::RegisterAccessor>
	accessorList = device->getRegisterAccessorsInModule("APP0");
	BOOST_CHECK(accessorList.size() == 4);

	std::list<mtca4u::Device::RegisterAccessor>::iterator
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
#ifdef _0
void DeviceTest::testAccessorForMuxedData() {
	// create mapped dummy device
	boost::shared_ptr<mtca4u::RegisterInfoMap> registerMap =
			mtca4u::mapFileParser().parse("sequences.map");
	boost::shared_ptr<mtca4u::DeviceBackend> ioDevice(new mtca4u::DeviceBackend);
	ioDevice->open("sequences.map");

	mtca4u::RegisterInfoMap::RegisterInfo sequenceInfo;
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
	mtca4u::Device device;
	device.open(ioDevice, registerMap);

	boost::shared_ptr<mtca4u::MultiplexedDataAccessor<double> > deMultiplexer =
			device.getCustomAccessor<mtca4u::MultiplexedDataAccessor<double> >(
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
#endif
