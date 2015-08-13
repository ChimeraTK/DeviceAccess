#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#define PCIEDEV_TEST_SLOT 0
#define LLRFDRV_TEST_SLOT 4
#define PCIEUNI_TEST_SLOT 6

#include "PcieDevice.h"
#include "PcieDeviceException.h"
#include "DeviceFactory.h"

using namespace mtca4u;

//constants for the registers and their contents. We keep the hard coded
//values at one place and only use the constants in the code below.
#define WORD_FIRMWARE_OFFSET 0x0
#define WORD_COMPILATION_OFFSET 0x4
#define WORD_USER_OFFSET 0xC
#define WORD_CLK_CNT_OFFSET 0x10
#define WORD_DUMMY_OFFSET 0x3C
#define DMMY_AS_ASCII 0x444D4D59
#define WORD_ADC_ENA_OFFSET 0x44
#define N_WORDS_DMA 25
#define PCIE_DEVICE "PCIE6"
#define LLRF_DEVICE "LLRF10"
//#define PCIE_UNI_DEVICE "PCIEUNI11"
#define PCIE_UNI_DEVICE "PCIE0"
#define NON_EXISTING_DEVICE "DUMMY9"


/** The unit tests for the devPCIE class, based on the
 *  boost unit test library. We use a class which holds a private
 *  instance of the devPCIE to be tested, which avoids code duplication
 *  (instantiating and opening the device over and over again etc.)
 *
 *  The tests should be run in the order stated here. At least testOpen()
 *  has to be executed first, and testClose() has to be executed last.
 *  Further dependencies are implemented in the teste suite.
 */
static DeviceFactory FactoryInstance = DeviceFactory::getInstance();
class PcieDeviceTest
{
public:
	PcieDeviceTest(std::string const & deviceFileName, unsigned int slot);

	/** A simple test which calls the default constructor and checks that the
	 * device is closed. We keep this separate because in principle constructors
	 * might throw, and this should explicitly be tested. But not within the
	 * instances of the test class, so it's a static function.
	 */
	static void testConstructor();

	/** Tests whether openig of the device works, and that the exception is
	 *  thrown correctly if the device cannot be opened.
	 */


	void testFailIfClosed();

	// Try Creating a device and check if it is connected.
	void testCreateDevice();

	//Try opening the created device and check it's open status.
	void testOpenDevice();

	//Try closing the created device and check it's open status.
	void testCloseDevice();

	void testReadArea();
	void testWriteArea();

	void testReadRegister();
	void testWriteRegister();

	void testReadDMA();
	void testWriteDMA();

	void testReadDeviceInfo();
	/// Test that all functions throw an exception if the device is not opened.
	void testFailIfDeviceClosed();


private:
	PcieDevice _pcieDevice;
	std::string _deviceFileName;
	unsigned int _slot;

	BaseDevice *_pcieDeviceInstance;

	// Internal function for better code readablility.
	// Returns an error message. If the message is empty the test succeeded.
	std::string checkDmaValues( std::vector<int32_t> const & dmaBuffer );
};

class PcieDeviceTestSuite : public test_suite {
public:
	PcieDeviceTestSuite(std::string const & deviceFileName, unsigned int slot)
: test_suite("devPCIE test suite") {
		// add member function test cases to a test suite
		boost::shared_ptr<PcieDeviceTest> pcieDeviceTest( new PcieDeviceTest( deviceFileName, slot ) );

		test_case* createDeviceTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testCreateDevice, pcieDeviceTest );
		test_case* openDeviceTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testOpenDevice, pcieDeviceTest );

		test_case* readAreaTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadArea, pcieDeviceTest );
		test_case* writeAreaTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteArea, pcieDeviceTest );

		test_case* readRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadRegister, pcieDeviceTest );
		test_case* writeRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteRegister, pcieDeviceTest );

		test_case* readDMATestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadDMA, pcieDeviceTest );
		test_case* writeDMATestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteDMA, pcieDeviceTest );

		test_case* readDeviceInfoTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadDeviceInfo, pcieDeviceTest );

		test_case* closeDeviceTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testCloseDevice, pcieDeviceTest );

		test_case* failIfDeviceClosedTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testFailIfDeviceClosed, pcieDeviceTest );



		/*
	readRegisterTestCase->depends_on( openTestCase );
	writeRegisterTestCase->depends_on( readRegisterTestCase );
	readAreaTestCase->depends_on( openTestCase );
	writeAreaTestCase->depends_on( readAreaTestCase );
	readDMATestCase->depends_on( openTestCase );
	writeDMATestCase->depends_on( readDMATestCase );
	closeTestCase->depends_on( openTestCase ); */


		readRegisterTestCase->depends_on( openDeviceTestCase );
		writeRegisterTestCase->depends_on( readRegisterTestCase );
		readAreaTestCase->depends_on( openDeviceTestCase );
		writeAreaTestCase->depends_on( readAreaTestCase );
		readDMATestCase->depends_on( openDeviceTestCase );
		writeDMATestCase->depends_on( readDMATestCase );
		closeDeviceTestCase->depends_on( openDeviceTestCase );
		add( BOOST_TEST_CASE( &PcieDeviceTest::testConstructor ) );
		// Todo .. Add dependencies
		add (createDeviceTestCase);
		add (openDeviceTestCase);

		add (readAreaTestCase);
		add (writeAreaTestCase);

		add (readRegisterTestCase);
		add (writeRegisterTestCase);

		add (readDMATestCase);
		add (writeDMATestCase);

		add (readDeviceInfoTestCase);

		add (closeDeviceTestCase);
		add( failIfDeviceClosedTestCase );

	}
private:

};

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
	framework::master_test_suite().p_name.value = "devPCIE test suite";

	std::stringstream llrfdummyFileName;
	llrfdummyFileName << "/dev/llrfdummys" << LLRFDRV_TEST_SLOT;
	//framework::master_test_suite().add( new PcieDeviceTestSuite(llrfdummyFileName.str(), LLRFDRV_TEST_SLOT) );
	//framework::master_test_suite().add( new PcieDeviceTestSuite(LLRF_DEVICE, LLRFDRV_TEST_SLOT) );

	std::stringstream mtcadummyFileName;
	mtcadummyFileName << "/dev/mtcadummys" << PCIEDEV_TEST_SLOT;
	//framework::master_test_suite().add( new PcieDeviceTestSuite(PCIE_DEVICE, PCIEDEV_TEST_SLOT) );

	std::stringstream pcieunidummyFileName;
	pcieunidummyFileName << "/dev/pcieunidummys" << PCIEUNI_TEST_SLOT;
	framework::master_test_suite().add( new PcieDeviceTestSuite(PCIE_UNI_DEVICE, PCIEUNI_TEST_SLOT) );


	return NULL;
}

// The implementations of the individual tests

void PcieDeviceTest::testConstructor() {
	PcieDevice pcieDevice;
	BOOST_CHECK( pcieDevice.isOpen() == false );
}

PcieDeviceTest::PcieDeviceTest(std::string const & deviceFileName, unsigned int slot)
: _deviceFileName(deviceFileName), _slot(slot), _pcieDeviceInstance(0)
{}


std::string PcieDeviceTest::checkDmaValues( std::vector<int32_t> const & dmaBuffer ) {
	bool dmaValuesOK = true;
	size_t i; // we need this after the loop
	for ( i=0; i < dmaBuffer.size(); ++i)
	{
		if ( dmaBuffer[i] != static_cast<int32_t>(i*i) ){
			dmaValuesOK = false;
			break;
		}
	}

	if ( dmaValuesOK ){
		return std::string(); // an empty string means test is ok
	}

	std::stringstream errorMessage;
	errorMessage <<  "Content of transferred DMA block is not valid. First wrong value at index " << i
			<< " is " << dmaBuffer[i] << std::endl;

	return errorMessage.str();
}


//check that the exception is thrown if the device is not opened
void PcieDeviceTest::testFailIfDeviceClosed()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// We use the  WORD_USER_OFFSET register in bar 0 for all operations. It is read/write.
	int32_t dataWord = 0; // Just use one single word, even for dma. nothing should be executed anyway.

	_pcieDeviceInstance->closeDev();
	BOOST_CHECK_THROW(  _pcieDeviceInstance->readReg(WORD_USER_OFFSET, &dataWord, /*bar*/ 0),
			PcieDeviceException );
	BOOST_CHECK_THROW(  _pcieDeviceInstance->readArea(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
			PcieDeviceException );
	BOOST_CHECK_THROW( _pcieDeviceInstance->readDMA(0, &dataWord, sizeof(dataWord), /*bar*/ 0),
			PcieDeviceException );
	BOOST_CHECK_THROW(  _pcieDeviceInstance->writeReg(WORD_USER_OFFSET, 0, /*bar*/ 0),
			PcieDeviceException );
	BOOST_CHECK_THROW(  _pcieDeviceInstance->writeArea(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
			PcieDeviceException );
	BOOST_CHECK_THROW(  _pcieDeviceInstance->writeDMA(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
			PcieDeviceException );

	std::string deviceInfo;
	BOOST_CHECK_THROW(  _pcieDeviceInstance->readDeviceInfo(&deviceInfo),
			PcieDeviceException );

}


void PcieDeviceTest::testReadDeviceInfo(){
	// The device info returns slot and driver version (major and minor).
	// For the dummy major and minor are the same as firmware and compilation, respectively.
	int32_t major;
	_pcieDeviceInstance->readReg(WORD_FIRMWARE_OFFSET, &major, /*bar*/ 0);
	int32_t minor;
	_pcieDeviceInstance->readReg(WORD_COMPILATION_OFFSET, &minor, /*bar*/ 0);
	std::stringstream referenceInfo;
	referenceInfo << "SLOT: "<< _slot << " DRV VER: "
			<< major << "." << minor;

	std::string deviceInfo;
	_pcieDeviceInstance->readDeviceInfo( &deviceInfo );
	BOOST_CHECK( referenceInfo.str() == deviceInfo );
}

void PcieDeviceTest::testReadDMA(){
	// Start the ADC on the dummy device. This will fill the "DMA" buffer with the default values (index^2) in the first 25 words.
	_pcieDeviceInstance->writeReg(WORD_ADC_ENA_OFFSET, 1 , /*bar*/ 0);

	std::vector<int32_t> dmaUserBuffer(N_WORDS_DMA,-1);

	_pcieDeviceInstance->readDMA( /*offset*/ 0, &dmaUserBuffer[0], N_WORDS_DMA*sizeof(int32_t), /*the dma bar*/ 2 );

	std::string errorMessage = checkDmaValues( dmaUserBuffer );
	BOOST_CHECK_MESSAGE( errorMessage.empty(), errorMessage  );

	// test dma with offset
	// read 20 words from address 5
	std::vector<int32_t> smallBuffer(20, -1);
	static const unsigned int readOffset = 5;
	_pcieDeviceInstance->readDMA( /*offset*/ readOffset*sizeof(int32_t),
			&smallBuffer[0],
			smallBuffer.size()*sizeof(int32_t), /*the dma bar*/ 2 );

	for (size_t i=0 ; i < smallBuffer.size() ; ++i){
		BOOST_CHECK( smallBuffer[i] == static_cast<int32_t>((i+readOffset)*(i+readOffset)) );
	}
}

void PcieDeviceTest::testWriteDMA(){

}

void PcieDeviceTest::testReadArea(){
	//FIXME: Change the driver to have the standard register set and adapt this code

	// Read the first two words, which are WORD_FIRMWARE and WORD_COMPILATION
	// We checked that single reading worked, so we use it to create the reference.
	int32_t firmwareContent;
	_pcieDeviceInstance->readReg(WORD_FIRMWARE_OFFSET, &firmwareContent, /*bar*/ 0);
	int32_t compilationContent;
	_pcieDeviceInstance->readReg(WORD_COMPILATION_OFFSET, &compilationContent, /*bar*/ 0);

	// Now try reading them as area
	int32_t twoWords[2];
	twoWords[0]=0xFFFFFFFF;
	twoWords[1]=0xFFFFFFFF;

	_pcieDeviceInstance->readArea( WORD_FIRMWARE_OFFSET, twoWords, 2*sizeof(int32_t), /*bar*/ 0 );
	BOOST_CHECK( (twoWords[0] == firmwareContent) && (twoWords[1] ==compilationContent) );

	// now try to read only six of the eight bytes. This should throw an exception because it is
	// not a multiple of 4.
	BOOST_CHECK_THROW( _pcieDeviceInstance->readArea( /*offset*/ 0, twoWords, /*nBytes*/ 6, /*bar*/ 0 ),
			PcieDeviceException );

	// also check another bar
	// Start the ADC on the dummy device. This will fill bar 2 (the "DMA" buffer) with the default values (index^2) in the first 25 words.
	_pcieDeviceInstance->writeReg(WORD_ADC_ENA_OFFSET, 1 , /*bar*/ 0);

	// use the same test as for DMA
	std::vector<int32_t> bar2Buffer(N_WORDS_DMA, -1);
	_pcieDeviceInstance->readDMA( /*offset*/ 0, &bar2Buffer[0], N_WORDS_DMA*sizeof(int32_t), /*the dma bar*/ 2 );

	std::string errorMessage = checkDmaValues( bar2Buffer );
	BOOST_CHECK_MESSAGE( errorMessage.empty(), errorMessage  );
}

void PcieDeviceTest::testWriteArea(){
	//FIXME: Change the driver to have the standard register set and adapt this code

	// Read the two WORD_CLK_CNT words, write them and read them back
	int32_t originalClockCounts[2];
	int32_t increasedClockCounts[2];
	int32_t readbackClockCounts[2];

	_pcieDeviceInstance->readArea( WORD_CLK_CNT_OFFSET, originalClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
	increasedClockCounts[0] = originalClockCounts[0]+1;
	increasedClockCounts[1] = originalClockCounts[1]+1;
	_pcieDeviceInstance->writeArea( WORD_CLK_CNT_OFFSET, increasedClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
	_pcieDeviceInstance->readArea( WORD_CLK_CNT_OFFSET, readbackClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
	BOOST_CHECK( (increasedClockCounts[0] == readbackClockCounts[0]) &&
			(increasedClockCounts[1] == readbackClockCounts[1]) );

	// now try to write only six of the eight bytes. This should throw an exception because it is
	// not a multiple of 4.
	BOOST_CHECK_THROW( _pcieDeviceInstance->writeArea(  WORD_CLK_CNT_OFFSET, originalClockCounts, /*nBytes*/ 6, /*bar*/ 0 ),
			PcieDeviceException );

	// also test another bar (area in bar 2), the usual drill: write and read back,
	// we know that reading works from the previous test
	std::vector<int32_t> writeBuffer(N_WORDS_DMA, 0xABCDEF01);
	std::vector<int32_t> readbackBuffer(N_WORDS_DMA, -1);
	_pcieDeviceInstance->writeArea( 0, &writeBuffer[0], N_WORDS_DMA*sizeof(int32_t), /*bar*/ 2 );
	_pcieDeviceInstance->readArea( 0, &readbackBuffer[0], N_WORDS_DMA*sizeof(int32_t), /*bar*/ 2 );
	BOOST_CHECK(readbackBuffer == writeBuffer);
}

void PcieDeviceTest::testReadRegister()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// read the WORD_COMPILATION register in bar 0. It's value is not 0.
	int32_t dataWord = 0; // initialise with 0 so we can check if reading the content works.

	//check that the exception is thrown if the device is not opened
	_pcieDeviceInstance->closeDev();
	BOOST_CHECK_THROW( _pcieDeviceInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0),
			PcieDeviceException );

	_pcieDeviceInstance->openDev();// no need to check if this works because we did the open test first
	_pcieDeviceInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0);
	BOOST_CHECK_EQUAL( dataWord, DMMY_AS_ASCII );

	/** There has to be an exception if the bar is wrong. 6 is definitely out of range. */
	BOOST_CHECK_THROW( _pcieDeviceInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 6),
			PcieDeviceException );

}

void PcieDeviceTest::testWriteRegister()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// We read the user register, increment it by one, write it and reread it.
	// As we checked that reading work, this is a reliable test that writing is ok.
	int32_t originalUserWord, newUserWord;
	_pcieDeviceInstance->readReg(WORD_USER_OFFSET, &originalUserWord, /*bar*/ 0);
	_pcieDeviceInstance->writeReg(WORD_USER_OFFSET, originalUserWord +1 , /*bar*/ 0);
	_pcieDeviceInstance->readReg(WORD_USER_OFFSET, &newUserWord, /*bar*/ 0);

	BOOST_CHECK_EQUAL( originalUserWord +1, newUserWord );

	/** There has to be an exception if the bar is wrong. 6 is definitely out of range. */
	BOOST_CHECK_THROW( _pcieDeviceInstance->writeReg(WORD_DUMMY_OFFSET, newUserWord, /*bar*/ 6),
			PcieDeviceException );
}


void PcieDeviceTest::testCloseDevice(){
	/** Try closing the device */
	_pcieDeviceInstance->closeDev();
	/** device should not be open now */
	BOOST_CHECK(_pcieDeviceInstance->isOpen() == false );
	/** device should remain in connected state */
	BOOST_CHECK(_pcieDeviceInstance->isConnected() == true );
}


void PcieDeviceTest::testOpenDevice(){
	_pcieDeviceInstance->openDev();
	BOOST_CHECK(_pcieDeviceInstance->isOpen() == true );
	BOOST_CHECK(_pcieDeviceInstance->isConnected() == true );
}

void PcieDeviceTest::testCreateDevice(){
	/** Try creating a non existing device */
	_pcieDeviceInstance = FactoryInstance.createDevice(NON_EXISTING_DEVICE);
	BOOST_CHECK(_pcieDeviceInstance == 0);
	/** Try creating an existing device */
	std::cout<<"DeviceName"<<_deviceFileName<<std::endl;
	_pcieDeviceInstance = FactoryInstance.createDevice(_deviceFileName);
	BOOST_CHECK(_pcieDeviceInstance != 0);
	/** Device should be in connect state now */
	BOOST_CHECK(_pcieDeviceInstance->isConnected() == true );
	/** Device should not be in open state */
	BOOST_CHECK(_pcieDeviceInstance->isOpen() == false );
}

