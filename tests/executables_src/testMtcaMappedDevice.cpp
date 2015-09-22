#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "MapFileParser.h"
#include "PcieBackend.h"
#include <cmath>

using namespace mtca4u;

//typedef Device<PcieBackend> MtcaMappedDevice;
typedef boost::shared_ptr<mtca4u::Device> MtcaMappedDevice;
#define VALID_MAPPING_FILE_NAME "mtcadummy_withoutModules.map"
#define DUMMY_DEVICE_FILE_NAME "/dev/mtcadummys0"
#define DEVICE_ALIAS "PCIE2"

#define FXPNT_ERROR_1_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint1.map"
#define FXPNT_ERROR_2_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint2.map"
#define FXPNT_ERROR_3_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint3.map"

/// helper function to cast with silent over/underflow handling
template<typename T, typename V>
T silent_numeric_cast(V value) {
	try {
		return boost::numeric_cast<T>(value);
	}
	catch(boost::numeric::negative_overflow &e) {
		return std::numeric_limits<T>::min();
	}
	catch(boost::numeric::positive_overflow &e) {
		return std::numeric_limits<T>::max();
	}
}


class MtcaDeviceTest {
public:
	MtcaDeviceTest();

	void testOpenClose();
	static void testThrowIfNeverOpened();

	static void testMapFileParser_parse();

	void testRegObject_getRegisterInfo();
	/** Read reading more than one word and working with offset. Check with all
	 * different data types.
	 */
	void testRegObject_readBlock();

	/** Check that the default arguments work, which means reading of one word,
	 * and check the corner case
	 *  nWord==0;
	 *  This is only checked for int and double, not all types.
	 *  Also checks the read convenience function.
	 */
	void testRegObject_readSimple();

	/** Testing the write function with a multiple registers.
	 */
	void testRegObject_writeBlock();

	/** Check that the default arguments work, which means writing of one word,
	 * and check the corner case
	 *  nWord==0;
	 *  This is only checked for int and doubel, not all types.
	 *  Also checks the write convenience function.
	 */
	void testRegObject_writeSimple();

	void getRegistersInModule();
	void getRegisterAccerrossInModule();

private:
	MtcaMappedDevice _mappedDevice;

	template <typename DataType>
	void testRegObject_typedWriteBlock(DataType offsetValue);
};

class MtcaDeviceTestSuite : public test_suite {
public:
	MtcaDeviceTestSuite() : test_suite("PcieBackend test suite") {
		// add member function test cases to a test suite
		boost::shared_ptr<MtcaDeviceTest> mtcaDeviceTest(
				new MtcaDeviceTest);

		// add member functions using BOOST_CLASS_TEST_CASE
		add(BOOST_CLASS_TEST_CASE(&MtcaDeviceTest::testOpenClose,
				mtcaDeviceTest));
		add(BOOST_CLASS_TEST_CASE(
				&MtcaDeviceTest::testRegObject_getRegisterInfo,
				mtcaDeviceTest));
		add(BOOST_CLASS_TEST_CASE(&MtcaDeviceTest::testRegObject_readBlock,
				mtcaDeviceTest));
		add(BOOST_CLASS_TEST_CASE(&MtcaDeviceTest::testRegObject_readSimple,
				mtcaDeviceTest));
		add(BOOST_CLASS_TEST_CASE(&MtcaDeviceTest::testRegObject_writeBlock,
				mtcaDeviceTest));
		add(BOOST_CLASS_TEST_CASE(&MtcaDeviceTest::testRegObject_writeSimple,
				mtcaDeviceTest));

		add(BOOST_TEST_CASE(&MtcaDeviceTest::testMapFileParser_parse));
		add(BOOST_TEST_CASE(&MtcaDeviceTest::testThrowIfNeverOpened));
		//        test_case* writeTestCase = BOOST_CLASS_TEST_CASE(
		// &MtcaDeviceTest::testWrite, mtcaDeviceTest );

		//	writeTestCase->depends_on( readTestCase );

		//        add( readTestCase );
		//        add( writeTestCase );
	}
};

// Register the test suite with the testing system so it is executed.
test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
	framework::master_test_suite().p_name.value = "MtcaMappedDevice test suite";

	return new MtcaDeviceTestSuite;
}

// The implementations of the individual tests

void MtcaDeviceTest::testOpenClose() {
	// test all tree open functions
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	BOOST_CHECK_NO_THROW(mappedDevice->open(DEVICE_ALIAS));
	BOOST_CHECK_NO_THROW(mappedDevice->close());
	std::list<std::string> parameters;
	boost::shared_ptr<DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	BOOST_CHECK_NO_THROW(mappedDevice->open(testDevice, registerMapping));
	BOOST_CHECK_NO_THROW(mappedDevice->close());

	boost::shared_ptr<mtca4u::Device> mappedDeviceAsBase (new mtca4u::Device());
	// you cannot directly open a devMap of BaseDevice. BaseDevice is purely virtual and
	// cannot be instantiated.
	/*  BOOST_CHECK_THROW(mappedDeviceAsBase.open(DUMMY_DEVICE_FILE_NAME,
                                               VALID_MAPPING_FILE_NAME),
                    DeviceException);*/

	// you have to create an instance of an implementation like PcieBackend and
	// pass it as a BaseDevice pointer
	boost::shared_ptr<DeviceBackend> dummyDevice(new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	//dummyDevice->open(DUMMY_DEVICE_FILE_NAME);



	BOOST_CHECK_NO_THROW(
			mappedDeviceAsBase->open(dummyDevice, registerMapping));

	// get of a smart pointer gives a raw pointer of the object it points to
	BOOST_CHECK(registerMapping.get() ==
			mappedDeviceAsBase->getRegisterMap().get());

	BOOST_CHECK_NO_THROW(mappedDeviceAsBase->close());
}

MtcaDeviceTest::MtcaDeviceTest() {}

void MtcaDeviceTest::testThrowIfNeverOpened() {
	boost::shared_ptr<mtca4u::Device> virginDevice ( new mtca4u::Device());

	int32_t dataWord;
	BOOST_CHECK_THROW(virginDevice->close(), DeviceException);
	BOOST_CHECK_THROW(
			virginDevice->readReg(0 /*regOffset*/, &dataWord, 0 /*bar*/),
			DeviceException);
	BOOST_CHECK_THROW(
			virginDevice->writeReg(0 /*regOffset*/, dataWord, 0 /*bar*/),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->readArea(0 /*regOffset*/, &dataWord,
			0 /*bar*/,  4 /*size*/),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->writeArea(0 /*regOffset*/, &dataWord,
			4 /*size*/, 0 /*bar*/),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->readDMA(0 /*regOffset*/, &dataWord,
			4 /*size*/, 0 /*bar*/),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->writeDMA(0 /*regOffset*/, &dataWord,
			4 /*size*/, 0 /*bar*/),
			DeviceException);

	//std::string deviceInfo;
	//BOOST_CHECK_THROW(virginDevice.readDeviceInfo(&deviceInfo), DeviceException);
	BOOST_CHECK_THROW(virginDevice->readDeviceInfo(), DeviceException);

	BOOST_CHECK_THROW(virginDevice->readReg("irrelevant", &dataWord),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->writeReg("irrelevant", &dataWord),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->readDMA("irrelevant", &dataWord),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->writeDMA("irrelevant", &dataWord),
			DeviceException);

	BOOST_CHECK_THROW(mtca4u::Device::regObject myRegObject =
			virginDevice->getRegObject("irrelevant"),
			DeviceException);
	BOOST_CHECK_THROW(
			//boost::shared_ptr<mtca4u::Device<mtca4u::PcieBackend>::RegisterAccessor>
			boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	myRegisterAccessor =
			virginDevice->getRegisterAccessor("irrelevant"),
			DeviceException);
	BOOST_CHECK_THROW(
			//boost::shared_ptr<mtca4u::Device<mtca4u::PcieBackend>::RegisterAccessor>
			boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	myRegisterAccessor =
			virginDevice->getRegisterAccessor("irrelevant"),
			DeviceException);
	BOOST_CHECK_THROW(virginDevice->getRegistersInModule("irrelevant"),
			DeviceException);
	BOOST_CHECK_THROW(
			virginDevice->getRegisterAccessorsInModule("irrelevant"), DeviceException);
}

void MtcaDeviceTest::testMapFileParser_parse() {
	boost::shared_ptr<mtca4u::Device> virginDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	BOOST_CHECK_THROW(boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(FXPNT_ERROR_1_MAPPING_FILE_NAME),MapFileParserException);
	BOOST_CHECK_THROW(boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(FXPNT_ERROR_2_MAPPING_FILE_NAME),MapFileParserException);
	BOOST_CHECK_THROW(boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(FXPNT_ERROR_3_MAPPING_FILE_NAME),MapFileParserException);
	/*BOOST_CHECK_THROW(virginDevice->open(testDevice,
 																						 registerMapping), //FXPNT_ERROR_1_MAPPING_FILE_NAME),
                     MapFileParserException);*/

}

void MtcaDeviceTest::testRegObject_getRegisterInfo() {
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	mappedDevice->open(testDevice, registerMapping);
	// Sorry, this test is hard coded against the mtcadummy implementation.
	// PP: Is there a different way of testing it?
	mtca4u::Device::regObject registerAccessor =
			mappedDevice->getRegObject("AREA_DMAABLE");
	RegisterInfoMap::RegisterInfo registerInfo = registerAccessor.getRegisterInfo();
	BOOST_CHECK(registerInfo.reg_address == 0x0);
	BOOST_CHECK(registerInfo.reg_elem_nr == 0x400);
	BOOST_CHECK(registerInfo.reg_size = 0x1000);
	BOOST_CHECK(registerInfo.reg_bar == 2);
	// the dmaable area has no fixed point settings, which has to result in
	// the default 32 0 true (like raw words)
	BOOST_CHECK(registerInfo.reg_width == 32);
	BOOST_CHECK(registerInfo.reg_frac_bits == 0);
	BOOST_CHECK(registerInfo.reg_signed == true);
	BOOST_CHECK(registerInfo.reg_name == "AREA_DMAABLE");

	registerAccessor = mappedDevice->getRegObject("WORD_FIRMWARE");
	registerInfo = registerAccessor.getRegisterInfo();
	BOOST_CHECK(registerInfo.reg_name == "WORD_FIRMWARE");
	BOOST_CHECK(registerInfo.reg_address == 0x0);
	BOOST_CHECK(registerInfo.reg_elem_nr == 0x1);
	BOOST_CHECK(registerInfo.reg_size = 0x4);
	BOOST_CHECK(registerInfo.reg_bar == 0);
	BOOST_CHECK(registerInfo.reg_width == 32);
	BOOST_CHECK(registerInfo.reg_frac_bits == 0);
	BOOST_CHECK(registerInfo.reg_signed == false);

	registerAccessor = mappedDevice->getRegObject("WORD_INCOMPLETE_1");
	registerInfo = registerAccessor.getRegisterInfo();
	BOOST_CHECK(registerInfo.reg_name == "WORD_INCOMPLETE_1");
	BOOST_CHECK(registerInfo.reg_address == 0x60);
	BOOST_CHECK(registerInfo.reg_elem_nr == 0x1);
	BOOST_CHECK(registerInfo.reg_size = 0x4);
	BOOST_CHECK(registerInfo.reg_bar == 0);
	BOOST_CHECK(registerInfo.reg_width == 13);
	BOOST_CHECK(registerInfo.reg_frac_bits == 0);
	BOOST_CHECK(registerInfo.reg_signed == true);

	registerAccessor = mappedDevice->getRegObject("WORD_INCOMPLETE_2");
	registerInfo = registerAccessor.getRegisterInfo();
	BOOST_CHECK(registerInfo.reg_name == "WORD_INCOMPLETE_2");
	BOOST_CHECK(registerInfo.reg_address == 0x64);
	BOOST_CHECK(registerInfo.reg_elem_nr == 0x1);
	BOOST_CHECK(registerInfo.reg_size = 0x4);
	BOOST_CHECK(registerInfo.reg_bar == 0);
	BOOST_CHECK(registerInfo.reg_width == 13);
	BOOST_CHECK(registerInfo.reg_frac_bits == 8);
	BOOST_CHECK(registerInfo.reg_signed == true);
}

void MtcaDeviceTest::testRegObject_readBlock() {
	// trigger the "DAQ" sequence which writes i*i into the first 25 registers, so
	// we know what we have
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	mappedDevice->open(testDevice, registerMapping);
	int32_t tempWord = 0;
	mappedDevice->writeReg("WORD_ADC_ENA", &tempWord);
	tempWord = 1;
	mappedDevice->writeReg("WORD_ADC_ENA", &tempWord);
	mtca4u::Device::regObject registerAccessor =
			mappedDevice->getRegObject("AREA_DMAABLE");
	// there are 25 elements with value i*i. ignore the first 2
	static const size_t N_ELEMENTS = 23;
	static const size_t OFFSET_ELEMENTS = 2;

	std::vector<int32_t> int32Buffer(N_ELEMENTS, 0);
	registerAccessor.read(&int32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS);
	// pre-check: make sure we know what we get
	for (size_t i = 0; i < N_ELEMENTS; ++i) {
		BOOST_CHECK(int32Buffer[i] == static_cast<int>((i + OFFSET_ELEMENTS) * (i + OFFSET_ELEMENTS)));
	}

	// Change the fractional parameters and test the read again.
	// We use another accessor which accesses the same area with different
	// fractioanal
	// settings (1 fractional bit, 10 bits, signed)
	mtca4u::Device::regObject registerAccessor10_1 =
			mappedDevice->getRegObject("AREA_DMAABLE_FIXEDPOINT10_1");

	registerAccessor10_1.read(&int32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

	std::vector<uint32_t> uint32Buffer(N_ELEMENTS, 0);
	BOOST_CHECK_THROW( registerAccessor10_1.read(&uint32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS), boost::numeric::negative_overflow );

	std::vector<int16_t> int16Buffer(N_ELEMENTS, 0);
	registerAccessor10_1.read(&int16Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

	std::vector<uint16_t> uint16Buffer(N_ELEMENTS, 0);
	BOOST_CHECK_THROW( registerAccessor10_1.read(&uint16Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS), boost::numeric::negative_overflow );

	std::vector<int8_t> int8Buffer(N_ELEMENTS, 0);
	BOOST_CHECK_THROW( registerAccessor10_1.read(&int8Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS), boost::numeric::positive_overflow );;

	std::vector<uint8_t> uint8Buffer(N_ELEMENTS, 0);
	BOOST_CHECK_THROW( registerAccessor10_1.read(&uint8Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS), boost::numeric::negative_overflow );;

	std::vector<float> floatBuffer(N_ELEMENTS, 0);
	registerAccessor10_1.read(&floatBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

	std::vector<double> doubleBuffer(N_ELEMENTS, 0);
	registerAccessor10_1.read(&doubleBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

	// now test different template types:
	for (size_t i = 0; i < N_ELEMENTS; ++i) {
		int rawValue = (i + OFFSET_ELEMENTS) * (i + OFFSET_ELEMENTS);
		double value;
		if (rawValue & 0x200) { // the sign bit for a 10 bit integer
			value = static_cast<double>(static_cast<int>(rawValue | 0xFFFFFE00)) / 2.;        // negative, 1 fractional bit
		} else {
			value = static_cast<double>(0x1FF & rawValue) / 2.;                               // positive, 1 fractional bit
		}

		std::stringstream errorMessage;

		errorMessage << "Index " << i << ", expected " << silent_numeric_cast<int>(round(value))
                		 << "(" << value << ") and read " << int32Buffer[i];
		BOOST_CHECK_MESSAGE(int32Buffer[i] == silent_numeric_cast<int>(round(value)), errorMessage.str());

		//BOOST_CHECK(uint32Buffer[i] == silent_numeric_cast<uint32_t>(round(value)));
		BOOST_CHECK(int16Buffer[i] == silent_numeric_cast<int16_t>(round(value)));
		//BOOST_CHECK(uint16Buffer[i] == silent_numeric_cast<uint16_t>(round(value)));
		//BOOST_CHECK(int8Buffer[i] == silent_numeric_cast<int8_t>(round(value)));
		//BOOST_CHECK(uint8Buffer[i] == silent_numeric_cast<uint8_t>(round(value)));

		BOOST_CHECK(floatBuffer[i] == value);
		BOOST_CHECK(doubleBuffer[i] == value);
	}
}

void MtcaDeviceTest::testRegObject_readSimple() {
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	mappedDevice->open(testDevice, registerMapping);
	//boost::shared_ptr<mtca4u::Device<mtca4u::PcieBackend>::RegisterAccessor>
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	registerAccessor = mappedDevice->getRegisterAccessor("WORD_USER");
	// 3 fractional bits, 12 bits, signed (from the map file)

	static const int inputValue = 0xFA5;
	registerAccessor->writeRaw(&inputValue);

	int32_t myInt = 0;
	registerAccessor->read(&myInt);

	BOOST_CHECK(myInt == -11);

	myInt = 17;
	registerAccessor->read(&myInt, 0);

	// the int has to be untouched
	BOOST_CHECK(myInt == 17);

	myInt = registerAccessor->read<int>();
	BOOST_CHECK(myInt == static_cast<int>(0xFFFFFFF5));

	double myDouble = 0;
	registerAccessor->read(&myDouble);
	BOOST_CHECK(myDouble == -11.375);

	myDouble = 0;
	myDouble = registerAccessor->read<double>();
	BOOST_CHECK(myDouble == -11.375);
}

template <typename DataType>
void MtcaDeviceTest::testRegObject_typedWriteBlock(DataType offsetValue) {
	// there are 25 elements. ignore the first 2
	static const size_t N_ELEMENTS = 23;
	static const size_t N_BYTES = N_ELEMENTS * sizeof(int32_t);
	static const size_t OFFSET_ELEMENTS = 2;

	std::vector<DataType> writeBuffer(N_ELEMENTS);

	// write i+offset to all arrays
	for (size_t i = 0; i < N_ELEMENTS; ++i) {
		writeBuffer[i] = i + offsetValue;
	}
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	mappedDevice->open(testDevice, registerMapping);
	//boost::shared_ptr<mtca4u::Device<mtca4u::PcieBackend>::RegisterAccessor>
	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	registerAccessor =
			mappedDevice->getRegisterAccessor("AREA_DMAABLE_FIXEDPOINT16_3");
	// 16 bits, 3 fractional signed

	// use raw write to zero the registers
	static const std::vector<int32_t> zeroedBuffer(N_ELEMENTS, 0);
	registerAccessor->writeRaw(&zeroedBuffer[0], N_BYTES,
			OFFSET_ELEMENTS * sizeof(int32_t));

	registerAccessor->write(&writeBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

	// we already tested that read works, so just read back and compare that we
	// get what we wrote
	std::vector<DataType> readBuffer(N_ELEMENTS, 0);
	registerAccessor->read(&readBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);
	for (size_t i = 0; i < N_ELEMENTS; ++i) {
		BOOST_CHECK(writeBuffer[i] == readBuffer[i]);
	}
}

void MtcaDeviceTest::testRegObject_writeBlock() {
	// the tested values run from startValue to startValue+23, so small negative
	// numbers test positive and
	// negative values
	testRegObject_typedWriteBlock(static_cast<uint32_t>(14));
	testRegObject_typedWriteBlock(
			static_cast<int32_t>(-14)); // also test negative values with signed ints
	testRegObject_typedWriteBlock(static_cast<uint16_t>(14));
	testRegObject_typedWriteBlock(
			static_cast<int16_t>(-14)); // also test negative values with signed ints
	testRegObject_typedWriteBlock(static_cast<uint8_t>(14));
	testRegObject_typedWriteBlock(
			static_cast<int8_t>(-14)); // also test negative values with signed ints
	testRegObject_typedWriteBlock(static_cast<double>(-13.75));
	testRegObject_typedWriteBlock(static_cast<float>(-13.75));
}

void MtcaDeviceTest::testRegObject_writeSimple() {
	//boost::shared_ptr<mtca4u::Device<mtca4u::PcieBackend>::RegisterAccessor>
	boost::shared_ptr<mtca4u::Device> mappedDevice ( new mtca4u::Device());
	std::list<std::string> parameters;
	boost::shared_ptr<mtca4u::DeviceBackend> testDevice ( new mtca4u::PcieBackend(".","mtcadummys0",parameters));
	mapFileParser fileParser;
	boost::shared_ptr<RegisterInfoMap> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
	mappedDevice->open(testDevice, registerMapping);

	boost::shared_ptr<mtca4u::Device::RegisterAccessor>
	registerAccessor = mappedDevice->getRegisterAccessor("WORD_USER");
	// the word has 3 fractional bits, 12 bits, signed, just to be different from
	// the
	// other setting. Values coming from the map file.

	static const int startValue = 0;
	// write something we are going to change
	registerAccessor->writeRaw(&startValue);

	int32_t myInt = -14;
	// write and read back
	registerAccessor->write(&myInt, 1);

	int32_t readbackValue = 0;
	registerAccessor->readRaw(&readbackValue);
	BOOST_CHECK(static_cast<uint32_t>(readbackValue) == 0xF90);

	myInt = 17;
	registerAccessor->write(&myInt, 0);
	// nothing should be written, should still read the same
	readbackValue = 0;
	registerAccessor->readRaw(&readbackValue);
	BOOST_CHECK(static_cast<uint32_t>(readbackValue) == 0xF90);

	registerAccessor->write(-17);
	BOOST_CHECK(registerAccessor->read<int>() == -17);

	double myDouble = -13.75;
	registerAccessor->write(&myDouble, 1);
	readbackValue = 0;
	registerAccessor->readRaw(&readbackValue);
	BOOST_CHECK(static_cast<uint32_t>(readbackValue) == 0xF92);

	registerAccessor->write(-17.25);
	BOOST_CHECK(registerAccessor->read<double>() == -17.25);
}
