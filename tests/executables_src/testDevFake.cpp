#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include "FakeDevice.h"
#include <cstdio>

#include "DeviceFactory.h"
#include "FakeDeviceException.h"

//#define REFERENCE_DEVICE "FAKE7"
//#define DUMMY_DEVICE "FAKE12"
//#define FAKE_DEVICE "FAKE13"
#define REFERENCE_DEVICE "FAKE0"
#define DUMMY_DEVICE "FAKE1"
#define FAKE_DEVICE "FAKE3"

#define NON_EXISTING_DEVICE "DUMMY9"
using namespace boost::unit_test_framework;


static mtca4u::DeviceFactory FactoryInstance = mtca4u::DeviceFactory::getInstance();
class FakeDeviceTest {
private:
	boost::shared_ptr<mtca4u::BaseDevice> _fakeDevice;
public:
	FakeDeviceTest():_fakeDevice() {};
	// Try Creating a device and check if it is connected.
	void testCreateDevice();

	//Try opening the created device and check it's open status.
	void testOpenDevice();

	//Try closing the created device and check it's open status.
	void testCloseDevice();

	void testReadRegister();

	void testReadArea();

	void testReadDMA();

	void testReadAreaWithInvalidParams();
	void testWriteReg();
	void testWriteRegErrors();
	void testWriteArea();
	void testWriteDMA();
	void testWriteAreaWithInvalidParams();
	void testDeviceInfo();
	void testReOpenExistingDevice();
	void testCreateFakeDevice();


	//void testReadFseekException();
};

class FakeDeviceTestSuite : public test_suite {
public:
	FakeDeviceTestSuite() : test_suite("devFake class test suite") {

		boost::shared_ptr<FakeDeviceTest> FakeDeviceTestPtr(new FakeDeviceTest());

		test_case* createDeviceTestCase = BOOST_CLASS_TEST_CASE( &FakeDeviceTest::testCreateDevice, FakeDeviceTestPtr );

		test_case* openDeviceTestCase = BOOST_CLASS_TEST_CASE( &FakeDeviceTest::testOpenDevice, FakeDeviceTestPtr );

		test_case* ReadElementTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadRegister, FakeDeviceTestPtr);

		test_case* readAreaTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadArea, FakeDeviceTestPtr);

		test_case* readDMATestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadDMA, FakeDeviceTestPtr);

		test_case* closeDeviceTestCase = BOOST_CLASS_TEST_CASE( &FakeDeviceTest::testCloseDevice, FakeDeviceTestPtr );

		test_case* readAreaWithInvalidParamsTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReadAreaWithInvalidParams, FakeDeviceTestPtr);
		test_case* writeRegTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testWriteReg, FakeDeviceTestPtr);
		test_case* writeRegErrorsTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testWriteRegErrors, FakeDeviceTestPtr);
		test_case* writeAreaTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testWriteArea, FakeDeviceTestPtr);
		test_case* writeDMATestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testWriteDMA, FakeDeviceTestPtr);
		test_case* WriteAreaWithInvalidParamsTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testWriteAreaWithInvalidParams, FakeDeviceTestPtr);
		test_case* deviceInfoTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testDeviceInfo, FakeDeviceTestPtr);

		test_case* reOpenExistingdeviceTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testReOpenExistingDevice, FakeDeviceTestPtr);
		test_case* createFakeDeviceTestCase = BOOST_CLASS_TEST_CASE(&FakeDeviceTest::testCreateFakeDevice, FakeDeviceTestPtr);

		add (createDeviceTestCase);
		add (openDeviceTestCase);
		add (ReadElementTestCase);
		add (readAreaTestCase);
		add (readDMATestCase);
		add (closeDeviceTestCase);

		add(readAreaWithInvalidParamsTestCase);
		add(writeRegTestCase);
		add(writeRegErrorsTestCase);
		add(writeAreaTestCase);
		add(writeDMATestCase);
		add(WriteAreaWithInvalidParamsTestCase);
		add(deviceInfoTestCase);

		add(reOpenExistingdeviceTestCase);
		add(createFakeDeviceTestCase);

		//add(testReadFseekException);
	}
};
test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value = "devFake class test suite";
	framework::master_test_suite().add(new FakeDeviceTestSuite());

	return NULL;
}

//Todo add fakeDevice file to cmake.
void FakeDeviceTest::testReOpenExistingDevice() {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(FAKE_DEVICE);
	FILE* file = fopen("._fakeDevice", "w");
	fclose(file);
	dummyDevice->openDev();
	BOOST_CHECK_THROW(dummyDevice->openDev(), mtca4u::FakeDeviceException);
	try {
		dummyDevice->openDev();
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_OPENED);
	}
	remove("_._fakeDevice");
}

void FakeDeviceTest::testCreateFakeDevice() {
	remove("._fakeDevice");
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(FAKE_DEVICE);
	dummyDevice->openDev();
	boost::filesystem::path pathToCreatedFile("._fakeDevice");
	BOOST_CHECK(boost::filesystem::exists(pathToCreatedFile) == true);
	remove("._fakeDevice");
}

void FakeDeviceTest::testReadAreaWithInvalidParams() {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);
	int32_t data[4];
	BOOST_CHECK_THROW(dummyDevice->readDMA(10, data, 3, 2), mtca4u::FakeDeviceException);
	try {
		dummyDevice->readDMA(10, data, 3, 2);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_CLOSED);
	}
	dummyDevice->openDev();
	BOOST_CHECK_THROW(dummyDevice->readDMA(10, data, 3, 2), mtca4u::FakeDeviceException);
	try {
		dummyDevice->readDMA(10, data, 3, 2);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_FILE_READ_DATA_ERROR);
	}
	remove("._DummyDevice");
}

void FakeDeviceTest::testWriteReg () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);//,true);
	dummyDevice->openDev();
	dummyDevice->writeReg(8, 0x01020304, 5);
	int32_t data;
	dummyDevice->readReg(8, &data, 5);
	BOOST_CHECK((uint32_t) data == 0x01020304);
	remove("._DummyDevice");
}

void FakeDeviceTest::testWriteRegErrors () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);//,true);
	int32_t data = 0x01020304;
	uint32_t offset = 12;
	uint8_t bar = 2;
	BOOST_CHECK_THROW(dummyDevice->writeReg(offset, data, bar), mtca4u::FakeDeviceException);
	try {
		dummyDevice->writeReg(offset, data, bar);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_CLOSED);
	}

	dummyDevice->openDev();
	BOOST_CHECK_THROW(dummyDevice->writeReg(offset, data, 8), mtca4u::FakeDeviceException);
	try {
		dummyDevice->writeReg(offset, data, 8);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}

	BOOST_CHECK_THROW(
			dummyDevice->writeReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, data, 2),
			mtca4u::FakeDeviceException);
	try {
		dummyDevice->writeReg((uint32_t)MTCA4U_LIBDEV_BAR_MEM_SIZE, data, 2);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	remove("._DummyDevice");
}

void FakeDeviceTest::testWriteArea () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);//,true);
	dummyDevice->openDev();
	const int dataSize = 4;
	int dataSizeInBytes = 16;
	int32_t input_data[dataSize] = {1, 4, 6, 7};
	int32_t output_data[dataSize] = {0};
	uint32_t offset = 12;
	uint8_t bar = 2;

	dummyDevice->writeArea(offset, input_data, dataSizeInBytes, bar);
	dummyDevice->readArea(offset, output_data, dataSizeInBytes, bar);

	BOOST_CHECK((uint32_t)output_data[0] == 0x00000001);
	BOOST_CHECK((uint32_t)output_data[1] == 0x00000004);
	BOOST_CHECK((uint32_t)output_data[2] == 0x00000006);
	BOOST_CHECK((uint32_t)output_data[3] == 0x00000007);
	remove("._DummyDevice");
}

void FakeDeviceTest::testWriteDMA () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);//,true);
	dummyDevice->openDev();
	const int dataSize = 4;
	int dataSizeInBytes = 16;
	int32_t input_data[dataSize] = {1, 4, 6, 7};
	int32_t output_data[dataSize] = {0};
	uint32_t offset = 12;
	uint8_t bar = 2;
	dummyDevice->writeDMA(offset, input_data, dataSizeInBytes, bar);
	dummyDevice->readDMA(offset, output_data, dataSizeInBytes, bar);

	BOOST_CHECK((uint32_t)output_data[0] == 0x00000001);
	BOOST_CHECK((uint32_t)output_data[1] == 0x00000004);
	BOOST_CHECK((uint32_t)output_data[2] == 0x00000006);
	BOOST_CHECK((uint32_t)output_data[3] == 0x00000007);
	remove("._DummyDevice");
}

void FakeDeviceTest::testWriteAreaWithInvalidParams () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);
	int32_t data[4];
	BOOST_CHECK_THROW(dummyDevice->writeDMA(10, data, 3, 2), mtca4u::FakeDeviceException);
	try {
		dummyDevice->writeDMA(10, data, 3, 2);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_CLOSED);
	}
	dummyDevice->openDev();
	int wrongDataSize = 3;
	BOOST_CHECK_THROW(dummyDevice->writeDMA(10, data, wrongDataSize, 2), mtca4u::FakeDeviceException);
	try {
		dummyDevice->writeDMA(10, data, 3, 2);
	}
	catch (mtca4u::FakeDeviceException& a) {
		BOOST_CHECK(a.getID() == mtca4u::FakeDeviceException::EX_DEVICE_FILE_WRITE_DATA_ERROR);
	}
	remove("._DummyDevice");
}

void FakeDeviceTest::testDeviceInfo () {
	boost::shared_ptr<mtca4u::BaseDevice> dummyDevice;
	dummyDevice = FactoryInstance.createDevice(DUMMY_DEVICE);
	dummyDevice->openDev();
	std::string deviceInformation;
	std::string expectedInfoString = "fake device: ._DummyDevice";
	dummyDevice->readDeviceInfo(&deviceInformation);
	BOOST_CHECK(deviceInformation == expectedInfoString );
	remove("._DummyDevice");
}


void FakeDeviceTest::testReadDMA() {
	int32_t data[4];
	uint32_t offset = 12;
	uint8_t bar = 2;
	uint8_t sizeToReadInBytes = 16;
	_fakeDevice->readDMA(offset, data, sizeToReadInBytes, bar);
	BOOST_CHECK((uint32_t)data[0] == 0xFFF0FFFF);
	BOOST_CHECK((uint32_t)data[1] == 0x01EFCDAB);
	BOOST_CHECK((uint32_t)data[2] == 0x55555555);
	BOOST_CHECK((uint32_t)data[3] == 0x00000000);
}

void FakeDeviceTest::testReadArea() {
	int32_t data[4];
	uint32_t offset = 12;
	uint8_t bar = 2;
	uint8_t sizeToReadInBytes = 16;
	_fakeDevice->readArea(offset, data, sizeToReadInBytes, bar);
	BOOST_CHECK((uint32_t)data[0] == 0xFFF0FFFF);
	BOOST_CHECK((uint32_t)data[1] == 0x01EFCDAB);
	BOOST_CHECK((uint32_t)data[2] == 0x55555555);
	BOOST_CHECK((uint32_t)data[3] == 0x00000000);
}

void FakeDeviceTest::testReadRegister() {
	int32_t data = 0;
	uint32_t offset = 12;
	uint8_t bar = 2;
	try{
		_fakeDevice->readReg(offset, &data, bar);
		std::cout<<"data:"<<std::hex<<data<<std::endl;
		BOOST_CHECK((uint32_t)data == 0xFFF0FFFF);
	}
	catch (mtca4u::FakeDeviceException& a) {
		std::cout<<a.getID()<<std::endl;
	}
}

void FakeDeviceTest::testCloseDevice(){
	/** Try closing the device */
	_fakeDevice->closeDev();
	/** device should not be open now */
	BOOST_CHECK(_fakeDevice->isOpen() == false );
	/** device should remain in connected state */
	BOOST_CHECK(_fakeDevice->isConnected() == true );
}


void FakeDeviceTest::testOpenDevice(){
	_fakeDevice->openDev();
	BOOST_CHECK(_fakeDevice->isOpen() == true );
	BOOST_CHECK(_fakeDevice->isConnected() == true );
}

void FakeDeviceTest::testCreateDevice(){
	/** Try creating a non existing device */
	BOOST_CHECK_THROW(FactoryInstance.createDevice(NON_EXISTING_DEVICE),mtca4u::DeviceFactoryException);
	/**Try creating local Fake device*/
	boost::shared_ptr<mtca4u::BaseDevice> mappedfakeDevice = FactoryInstance.createDevice(REFERENCE_DEVICE);
	std::cout<<"allo2"<<std::endl;
	BOOST_CHECK(mappedfakeDevice != 0);
	/** Device should be in connect state now */
	BOOST_CHECK(mappedfakeDevice->isConnected() == true );
	/** Device should not be in open state */
	BOOST_CHECK(mappedfakeDevice->isOpen() == false );

	/** Try creating an existing device,mapped */
	_fakeDevice = FactoryInstance.createDevice(REFERENCE_DEVICE);//true);
	BOOST_CHECK(_fakeDevice != 0);
	/** Device should be in connect state now */
	BOOST_CHECK(_fakeDevice->isConnected() == true );
	/** Device should not be in open state */
	BOOST_CHECK(_fakeDevice->isOpen() == false );
}

