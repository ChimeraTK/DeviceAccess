#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#define PCIEDEV_TEST_SLOT 0
#define LLRFDRV_TEST_SLOT 4
#define PCIEUNI_TEST_SLOT 6

#include "PcieBackend.h"
#include "PcieBackendException.h"
#include "BackendFactory.h"

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


/** The unit tests for the PcieBackend class, based on the
 *  boost unit test library. We use a class which holds a private
 *  instance of the PcieBackend to be tested, which avoids code duplication
 *  (instantiating and opening the backend over and over again etc.)
 *
 *  The tests should be run in the order stated here. At least testOpen()
 *  has to be executed first, and testClose() has to be executed last.
 *  Further dependencies are implemented in the teste suite.
 */
static BackendFactory FactoryInstance = BackendFactory::getInstance();
class PcieBackendTest
{
public:
	PcieBackendTest(std::string const & deviceFileName, unsigned int slot);

	/** A simple test which calls the default constructor and checks that the
	 * backend is closed. We keep this separate because in principle constructors
	 * might throw, and this should explicitly be tested. But not within the
	 * instances of the test class, so it's a static function.
	 */
	static void testConstructor();

	/** Tests whether openig of the backend works, and that the exception is
	 *  thrown correctly if the backend cannot be opened.
	 */


	void testFailIfClosed();

	// Try Creating a backend and check if it is connected.
	void testCreateBackend();

	//Try opening the created backend and check it's open status.
	void testOpen();

	//Try closing the created backend and check it's open status.
	void testClose();

	void testRead();
	void testWriteArea();

	void testReadRegister();
	void testWriteRegister();

	void testReadDMA();
	void testWriteDMA();

	void testReadDeviceInfo();
	/// Test that all functions throw an exception if the backend is not opened.
	void testFailIfBackendClosed();

private:
	PcieBackend _pcieBackend;
	std::string _deviceFileName;
	unsigned int _slot;

	boost::shared_ptr<DeviceBackend>_pcieBackendInstance;

	// Internal function for better code readablility.
	// Returns an error message. If the message is empty the test succeeded.
	std::string checkDmaValues( std::vector<int32_t> const & dmaBuffer );
};

class PcieBackendTestSuite : public test_suite {
public:
	PcieBackendTestSuite(std::string const & deviceFileName, unsigned int slot)
	: test_suite("PcieBackend test suite") {
		// add member function test cases to a test suite
		boost::shared_ptr<PcieBackendTest> pcieBackendTest( new PcieBackendTest( deviceFileName, slot ) );

		test_case* createBackendTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testCreateBackend, pcieBackendTest );
		test_case* openTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testOpen, pcieBackendTest );

		test_case* readTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testRead, pcieBackendTest );
		test_case* writeAreaTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testWriteArea, pcieBackendTest );

		test_case* readRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testReadRegister, pcieBackendTest );
		test_case* writeRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testWriteRegister, pcieBackendTest );

		test_case* readDMATestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testReadDMA, pcieBackendTest );
		test_case* writeDMATestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testWriteDMA, pcieBackendTest );

		test_case* readDeviceInfoTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testReadDeviceInfo, pcieBackendTest );

		test_case* closeTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testClose, pcieBackendTest );

		test_case* failIfBackendClosedTestCase = BOOST_CLASS_TEST_CASE( &PcieBackendTest::testFailIfBackendClosed, pcieBackendTest );

		readRegisterTestCase->depends_on( openTestCase );
		writeRegisterTestCase->depends_on( readRegisterTestCase );
		readTestCase->depends_on( openTestCase );
		writeAreaTestCase->depends_on( readTestCase );
		readDMATestCase->depends_on( openTestCase );
		writeDMATestCase->depends_on( readDMATestCase );
		closeTestCase->depends_on( openTestCase );
		add( BOOST_TEST_CASE( &PcieBackendTest::testConstructor ) );
		// Todo .. Add dependencies
		add (createBackendTestCase);
		add (openTestCase);

		add (readTestCase);
		add (writeAreaTestCase);

		add (readRegisterTestCase);
		add (writeRegisterTestCase);

		add (readDMATestCase);
		add (writeDMATestCase);

		add (readDeviceInfoTestCase);

		add (closeTestCase);
		add( failIfBackendClosedTestCase );

	}
private:

};

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
	framework::master_test_suite().p_name.value = "PcieBackend test suite";

	std::stringstream llrfdummyFileName;
	llrfdummyFileName << "/dev/llrfdummys" << LLRFDRV_TEST_SLOT;
	//framework::master_test_suite().add( new PcieBackendTestSuite(llrfdummyFileName.str(), LLRFDRV_TEST_SLOT) );
	//framework::master_test_suite().add( new PcieBackendTestSuite(LLRF_DEVICE, LLRFDRV_TEST_SLOT) );

	std::stringstream mtcadummyFileName;
	mtcadummyFileName << "/dev/mtcadummys" << PCIEDEV_TEST_SLOT;
	//framework::master_test_suite().add( new PcieBackendTestSuite(PCIE_DEVICE, PCIEDEV_TEST_SLOT) );

	std::stringstream pcieunidummyFileName;
	pcieunidummyFileName << "/dev/pcieunidummys" << PCIEUNI_TEST_SLOT;
	framework::master_test_suite().add( new PcieBackendTestSuite(PCIE_UNI_DEVICE, PCIEUNI_TEST_SLOT) );


	return NULL;
}

// The implementations of the individual tests

void PcieBackendTest::testConstructor() {
  PcieBackend pcieBackend("");
  BOOST_CHECK( pcieBackend.isOpen() == false );
  BOOST_CHECK( pcieBackend.isConnected() == true );
}

PcieBackendTest::PcieBackendTest(std::string const & deviceFileName, unsigned int slot)
  : _pcieBackend(deviceFileName), _deviceFileName(deviceFileName), _slot(slot){
}


std::string PcieBackendTest::checkDmaValues( std::vector<int32_t> const & dmaBuffer ) {
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


//check that the exception is thrown if the backend is not opened
void PcieBackendTest::testFailIfBackendClosed()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// We use the  WORD_USER_OFFSET register in bar 0 for all operations. It is read/write.
	int32_t dataWord = 0; // Just use one single word, even for dma. nothing should be executed anyway.

	_pcieBackendInstance->close();
	BOOST_CHECK_THROW( // _pcieBackendInstance->readReg(WORD_USER_OFFSET, &dataWord, /*bar*/ 0),
			_pcieBackendInstance->read(/*bar*/ 0, WORD_USER_OFFSET, &dataWord, 4),
			PcieBackendException );
	//BOOST_CHECK_THROW(  _pcieBackendInstance->read(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
	BOOST_CHECK_THROW(  _pcieBackendInstance->read(/*bar*/ 0, WORD_USER_OFFSET, &dataWord, sizeof(dataWord)),
			PcieBackendException );
	BOOST_CHECK_THROW( _pcieBackendInstance->readDMA(/*bar*/ 0, 0, &dataWord, sizeof(dataWord)),
			PcieBackendException );
	//BOOST_CHECK_THROW(  _pcieBackendInstance->writeReg(/*bar*/ 0, WORD_USER_OFFSET, 0),
	BOOST_CHECK_THROW(  _pcieBackendInstance->write(/*bar*/ 0, WORD_USER_OFFSET, 0, 4),
			PcieBackendException );
	BOOST_CHECK_THROW(  _pcieBackendInstance->write(/*bar*/ 0, WORD_USER_OFFSET, &dataWord, sizeof(dataWord) ),
			PcieBackendException );
	BOOST_CHECK_THROW(  _pcieBackendInstance->writeDMA( /*bar*/ 0, WORD_USER_OFFSET, &dataWord, sizeof(dataWord)),
			PcieBackendException );

	//std::string deviceInfo;
	BOOST_CHECK_THROW(  _pcieBackendInstance->readDeviceInfo(),
			PcieBackendException );

}

void PcieBackendTest::testReadDeviceInfo(){
	// The device info returns slot and driver version (major and minor).
	// For the dummy major and minor are the same as firmware and compilation, respectively.
	int32_t major;
	//_pcieBackendInstance->readReg(WORD_FIRMWARE_OFFSET, &major, /*bar*/ 0);
	_pcieBackendInstance->read(/*bar*/ 0, WORD_FIRMWARE_OFFSET, &major, 4);
	int32_t minor;
	//_pcieBackendInstance->readReg(WORD_COMPILATION_OFFSET, &minor, /*bar*/ 0);
	_pcieBackendInstance->read(/*bar*/ 0, WORD_COMPILATION_OFFSET, &minor, 4);
	std::stringstream referenceInfo;
	referenceInfo << "SLOT: "<< _slot << " DRV VER: "
			<< major << "." << minor;

	std::string deviceInfo;
	deviceInfo = _pcieBackendInstance->readDeviceInfo();
	BOOST_CHECK( referenceInfo.str() == deviceInfo );
}

void PcieBackendTest::testReadDMA(){
	// Start the ADC on the dummy device. This will fill the "DMA" buffer with the default values (index^2) in the first 25 words.
	//_pcieBackendInstance->writeReg(/*bar*/ 0, WORD_ADC_ENA_OFFSET, 1);
	int32_t data = 1;
	_pcieBackendInstance->write(/*bar*/ 0, WORD_ADC_ENA_OFFSET, &data, 4);

	std::vector<int32_t> dmaUserBuffer(N_WORDS_DMA,-1);

	_pcieBackendInstance->readDMA( /*the dma bar*/ 2, /*offset*/ 0, &dmaUserBuffer[0], N_WORDS_DMA*sizeof(int32_t) );

	std::string errorMessage = checkDmaValues( dmaUserBuffer );
	BOOST_CHECK_MESSAGE( errorMessage.empty(), errorMessage  );

	// test dma with offset
	// read 20 words from address 5
	std::vector<int32_t> smallBuffer(20, -1);
	static const unsigned int readOffset = 5;
	_pcieBackendInstance->readDMA(  /*the dma bar*/ 2, /*offset*/ readOffset*sizeof(int32_t),
			&smallBuffer[0],
			smallBuffer.size()*sizeof(int32_t) );

	for (size_t i=0 ; i < smallBuffer.size() ; ++i){
		BOOST_CHECK( smallBuffer[i] == static_cast<int32_t>((i+readOffset)*(i+readOffset)) );
	}
}

void PcieBackendTest::testWriteDMA(){

}

void PcieBackendTest::testRead(){
	//FIXME: Change the driver to have the standard register set and adapt this code

	// Read the first two words, which are WORD_FIRMWARE and WORD_COMPILATION
	// We checked that single reading worked, so we use it to create the reference.
	int32_t firmwareContent;
	_pcieBackendInstance->read(/*bar*/ 0, WORD_FIRMWARE_OFFSET, &firmwareContent, 4);
	int32_t compilationContent;
	_pcieBackendInstance->read( /*bar*/ 0, WORD_COMPILATION_OFFSET, &compilationContent, 4);

	// Now try reading them as area
	int32_t twoWords[2];
	twoWords[0]=0xFFFFFFFF;
	twoWords[1]=0xFFFFFFFF;

	_pcieBackendInstance->read( /*bar*/ 0, WORD_FIRMWARE_OFFSET, twoWords, 2*sizeof(int32_t) );
	BOOST_CHECK( (twoWords[0] == firmwareContent) && (twoWords[1] ==compilationContent) );

	// now try to read only six of the eight bytes. This should throw an exception because it is
	// not a multiple of 4.
	BOOST_CHECK_THROW( _pcieBackendInstance->read( /*bar*/ 0, /*offset*/ 0, twoWords, /*nBytes*/ 6 ),
			PcieBackendException );

	// also check another bar
	// Start the ADC on the dummy device. This will fill bar 2 (the "DMA" buffer) with the default values (index^2) in the first 25 words.
	int32_t data = 1;
	_pcieBackendInstance->write(/*bar*/ 0,WORD_ADC_ENA_OFFSET, &data, 4 );
	// use the same test as for DMA
	std::vector<int32_t> bar2Buffer(N_WORDS_DMA, -1);
	_pcieBackendInstance->readDMA(  /*the dma bar*/ 2, /*offset*/ 0, &bar2Buffer[0], N_WORDS_DMA*sizeof(int32_t) );

	std::string errorMessage = checkDmaValues( bar2Buffer );
	BOOST_CHECK_MESSAGE( errorMessage.empty(), errorMessage  );
}

void PcieBackendTest::testWriteArea(){
	//FIXME: Change the driver to have the standard register set and adapt this code

	// Read the two WORD_CLK_CNT words, write them and read them back
	int32_t originalClockCounts[2];
	int32_t increasedClockCounts[2];
	int32_t readbackClockCounts[2];

	_pcieBackendInstance->read( /*bar*/ 0, WORD_CLK_CNT_OFFSET, originalClockCounts, 2*sizeof(int32_t) );
	increasedClockCounts[0] = originalClockCounts[0]+1;
	increasedClockCounts[1] = originalClockCounts[1]+1;
	_pcieBackendInstance->write( /*bar*/ 0, WORD_CLK_CNT_OFFSET, increasedClockCounts, 2*sizeof(int32_t) );
	_pcieBackendInstance->read(  /*bar*/ 0, WORD_CLK_CNT_OFFSET, readbackClockCounts, 2*sizeof(int32_t) );
	BOOST_CHECK( (increasedClockCounts[0] == readbackClockCounts[0]) &&
			(increasedClockCounts[1] == readbackClockCounts[1]) );

	// now try to write only six of the eight bytes. This should throw an exception because it is
	// not a multiple of 4.
	BOOST_CHECK_THROW( _pcieBackendInstance->write(  /*bar*/ 0 , WORD_CLK_CNT_OFFSET, originalClockCounts, /*nBytes*/ 6 ),
			PcieBackendException );

	// also test another bar (area in bar 2), the usual drill: write and read back,
	// we know that reading works from the previous test
	std::vector<int32_t> writeBuffer(N_WORDS_DMA, 0xABCDEF01);
	std::vector<int32_t> readbackBuffer(N_WORDS_DMA, -1);
	_pcieBackendInstance->write( /*bar*/ 2, 0, &writeBuffer[0], N_WORDS_DMA*sizeof(int32_t) );
	_pcieBackendInstance->read( /*bar*/ 2, 0, &readbackBuffer[0], N_WORDS_DMA*sizeof(int32_t) );
	BOOST_CHECK(readbackBuffer == writeBuffer);
}

void PcieBackendTest::testReadRegister()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// read the WORD_COMPILATION register in bar 0. It's value is not 0.
	int32_t dataWord = 0; // initialise with 0 so we can check if reading the content works.

	//check that the exception is thrown if the backend is not opened
	_pcieBackendInstance->close();
	BOOST_CHECK_THROW( _pcieBackendInstance->read(/*bar*/ 0, WORD_DUMMY_OFFSET, &dataWord, 4),
			PcieBackendException );

	_pcieBackendInstance->open();// no need to check if this works because we did the open test first
	//_pcieBackendInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0);
	_pcieBackendInstance->read(/*bar*/ 0, WORD_DUMMY_OFFSET, &dataWord, 4);
	BOOST_CHECK_EQUAL( dataWord, DMMY_AS_ASCII );

	/** There has to be an exception if the bar is wrong. 6 is definitely out of range. */
	//BOOST_CHECK_THROW( _pcieBackendInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 6),
	BOOST_CHECK_THROW( _pcieBackendInstance->read( /*bar*/ 6, WORD_DUMMY_OFFSET, &dataWord, 4),
			PcieBackendException );

}

void PcieBackendTest::testWriteRegister()
{
	//FIXME: Change the driver to have the standard register set and adapt this code

	// We read the user register, increment it by one, write it and reread it.
	// As we checked that reading work, this is a reliable test that writing is ok.
	int32_t originalUserWord, newUserWord;
	_pcieBackendInstance->read(/*bar*/ 0, WORD_USER_OFFSET, &originalUserWord, 4);
	int32_t data = originalUserWord+1;
	_pcieBackendInstance->write( /*bar*/ 0, WORD_USER_OFFSET, &data ,4);
	_pcieBackendInstance->read( /*bar*/ 0, WORD_USER_OFFSET, &newUserWord, 4);

	BOOST_CHECK_EQUAL( originalUserWord +1, newUserWord );

	/** There has to be an exception if the bar is wrong. 6 is definitely out of range. */
	//BOOST_CHECK_THROW( _pcieBackendInstance->writeReg( /*bar*/ 6, WORD_DUMMY_OFFSET, newUserWord),
	BOOST_CHECK_THROW( _pcieBackendInstance->write( /*bar*/ 6, WORD_DUMMY_OFFSET, &newUserWord, 4),
			PcieBackendException );
}


void PcieBackendTest::testClose(){
	/** Try closing the backend */
	_pcieBackendInstance->close();
	/** backend should not be open now */
	BOOST_CHECK(_pcieBackendInstance->isOpen() == false );
	/** backend should remain in connected state */
	BOOST_CHECK(_pcieBackendInstance->isConnected() == true );
}


void PcieBackendTest::testOpen(){
	_pcieBackendInstance->open();
	BOOST_CHECK(_pcieBackendInstance->isOpen() == true );
	BOOST_CHECK(_pcieBackendInstance->isConnected() == true );
}

void PcieBackendTest::testCreateBackend(){
	/** Try creating a non existing device */
	BOOST_CHECK_THROW(FactoryInstance.createDevice(NON_EXISTING_DEVICE),BackendFactoryException);
	/** Try creating an existing device */
	std::cout<<"DeviceName"<<_deviceFileName<<std::endl;
	_pcieBackendInstance = FactoryInstance.createDevice(_deviceFileName);
	BOOST_CHECK(_pcieBackendInstance != 0);
	/** Backend should be in connect state now */
	BOOST_CHECK(_pcieBackendInstance->isConnected() == true );
	/** Backend should not be in open state */
	BOOST_CHECK(_pcieBackendInstance->isOpen() == false );
}

