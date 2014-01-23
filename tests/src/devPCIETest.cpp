#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#define PCIEDEV_TEST_SLOT 0
#define LLRFDRV_TEST_SLOT 4

#include "devPCIE.h"
#include "exDevPCIE.h"

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

/** The unit tests for the devPCIE class, based on the 
 *  boost unit test library. We use a class which holds a private 
 *  instance of the devPCIE to be tested, which avoids code duplication
 *  (instantiating and opening the device over and over again etc.)
 * 
 *  The tests should be run in the order stated here. At least testOpen()
 *  has to be executed first, and testClose() has to be executed last.
 *  Further dependencies are implemented in the teste suite.
 */
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
  void testOpen();

  void testReadRegister();
  void testWriteRegister();
\
  void testReadArea();
  void testWriteArea();

  void testReadDMA();
  void testWriteDMA();

  void testReadDeviceInfo();

  void testClose();
  /// Test that all functions throw an exception if the device is not opened.
  void testFailIfClosed();

 private:
  devPCIE _pcieDevice;
  std::string _deviceFileName;
  unsigned int _slot;

  // Internal function for better code readablility.
  // Returns an error message. If the message is empty the test succeeded.
  std::string checkDmaValues( int32_t * dmaBuffer, size_t nWords );
};

class PcieDeviceTestSuite : public test_suite {
public:
  PcieDeviceTestSuite(std::string const & deviceFileName, unsigned int slot) 
    : test_suite("devPCIE test suite") {
        // add member function test cases to a test suite
    boost::shared_ptr<PcieDeviceTest> pcieDeviceTest( new PcieDeviceTest( deviceFileName, slot ) );

        test_case* openTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testOpen, pcieDeviceTest );

        test_case* readRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadRegister, pcieDeviceTest );
        test_case* writeRegisterTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteRegister, pcieDeviceTest );

        test_case* readAreaTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadArea, pcieDeviceTest );
        test_case* writeAreaTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteArea, pcieDeviceTest );

        test_case* readDMATestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadDMA, pcieDeviceTest );
        test_case* writeDMATestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testWriteDMA, pcieDeviceTest );

        test_case* readDeviceInfoTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testReadDeviceInfo, pcieDeviceTest );

        test_case* closeTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testClose, pcieDeviceTest );
        test_case* failIfClosedTestCase = BOOST_CLASS_TEST_CASE( &PcieDeviceTest::testFailIfClosed, pcieDeviceTest );

	readRegisterTestCase->depends_on( openTestCase );
	writeRegisterTestCase->depends_on( readRegisterTestCase );
	readAreaTestCase->depends_on( openTestCase );
	writeAreaTestCase->depends_on( readAreaTestCase );
	readDMATestCase->depends_on( openTestCase );
	writeDMATestCase->depends_on( readDMATestCase );
	closeTestCase->depends_on( openTestCase ); 

	add( BOOST_TEST_CASE( &PcieDeviceTest::testConstructor ) );
        add( openTestCase );

        add( readRegisterTestCase );
        add( writeRegisterTestCase );

        add( readAreaTestCase );
        add( writeAreaTestCase );

        add( readDMATestCase );
        add( writeDMATestCase );

	add( readDeviceInfoTestCase );

	add( closeTestCase );
	add( failIfClosedTestCase );
  }
private:
  
};

// Although the compiler complains that argc and argv are not used they 
// cannot be commented out. This somehow changes the signature and linking fails.
test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
  framework::master_test_suite().p_name.value = "devPCIE test suite";

  std::stringstream llrfdummyFileName;
  llrfdummyFileName << "/dev/llrfdummys" << LLRFDRV_TEST_SLOT;
  framework::master_test_suite().add( new PcieDeviceTestSuite(llrfdummyFileName.str(), LLRFDRV_TEST_SLOT) );

  std::stringstream mtcadummyFileName;
  mtcadummyFileName << "/dev/mtcadummys" << PCIEDEV_TEST_SLOT;
  framework::master_test_suite().add( new PcieDeviceTestSuite(mtcadummyFileName.str(), PCIEDEV_TEST_SLOT) );

  return NULL;
}

// The implementations of the individual tests

void PcieDeviceTest::testConstructor() {
  devPCIE pcieDevice;
  BOOST_CHECK( pcieDevice.isOpen() == false );
}

PcieDeviceTest::PcieDeviceTest(std::string const & deviceFileName, unsigned int slot)
  : _deviceFileName(deviceFileName), _slot(slot)
{}

void PcieDeviceTest::testOpen() {
  BOOST_CHECK_THROW( _pcieDevice.openDev("/invalid/FileName") ,
		     exDevPCIE );
  try{
    _pcieDevice.openDev("/invalid/FileName");
  }catch(exDevPCIE & e){
    BOOST_CHECK( e.getID() == exDevPCIE::EX_CANNOT_OPEN_DEVICE );
  }

  // test opening an unsupported file
  BOOST_CHECK_THROW( _pcieDevice.openDev("/dev/noioctldummys5") ,
		     exDevPCIE );
  try{
    _pcieDevice.openDev("/dev/noioctldummys5");
  }catch(exDevPCIE & e){
    BOOST_CHECK( e.getID() == exDevPCIE::EX_UNSUPPORTED_DRIVER );
  }

  // after the failed open attempt the device should still be closed.
  BOOST_CHECK( _pcieDevice.isOpen() == false );

  try{
    _pcieDevice.openDev(_deviceFileName);
  }
  catch(exDevPCIE & e )
  {
    std::string errorMessage("Opening the dummy device failed. You need to load the mtcadummy driver to run the devPCIE tests.\n");
    errorMessage+= std::string("exception exDevPCIE: ") + e.what();
    BOOST_FAIL(errorMessage);
    throw;
  }
  BOOST_CHECK( _pcieDevice.isOpen() );

  // The device cannot be reopened. Check that an exception is thrown.
  BOOST_CHECK_THROW( _pcieDevice.openDev(_deviceFileName) ,
		     exDevPCIE );

}

void PcieDeviceTest::testReadRegister()
{
  //FIXME: Change the driver to have the standard register set and adapt this code
  
  // read the WORD_COMPILATION register in bar 0. It's value is not 0.
  int32_t dataWord = 0; // initialise with 0 so we can check if reading the content works.

  //check that the exception is thrown if the device is not opened
  _pcieDevice.closeDev();
  BOOST_CHECK_THROW( _pcieDevice.readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0),
		     exDevPCIE );

  _pcieDevice.openDev(_deviceFileName);// no need to check if this works because we did the open test first 
  _pcieDevice.readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0);
  BOOST_CHECK_EQUAL( dataWord, DMMY_AS_ASCII );
}

void PcieDeviceTest::testWriteRegister()
{
  //FIXME: Change the driver to have the standard register set and adapt this code
  
  // We read the user register, increment it by one, write it and reread it.
  // As we checked that reading work, this is a reliable test that writing is ok.
  int32_t originalUserWord, newUserWord;
  _pcieDevice.readReg(WORD_USER_OFFSET, &originalUserWord, /*bar*/ 0);
  _pcieDevice.writeReg(WORD_USER_OFFSET, originalUserWord +1 , /*bar*/ 0);
  _pcieDevice.readReg(WORD_USER_OFFSET, &newUserWord, /*bar*/ 0);
 
  BOOST_CHECK_EQUAL( originalUserWord +1, newUserWord );
}

void PcieDeviceTest::testReadArea(){
  //FIXME: Change the driver to have the standard register set and adapt this code
  
  // Read the first two words, which are WORD_FIRMWARE and WORD_COMPILATION
  // We checked that single reading worked, so we use it to create the reference.
  int32_t firmwareContent;
  _pcieDevice.readReg(WORD_FIRMWARE_OFFSET, &firmwareContent, /*bar*/ 0);
  int32_t compilationContent;
  _pcieDevice.readReg(WORD_COMPILATION_OFFSET, &compilationContent, /*bar*/ 0);

  // Now try reading them as area
  int32_t twoWords[2];
  twoWords[0]=0xFFFFFFFF;
  twoWords[1]=0xFFFFFFFF;
  
  _pcieDevice.readArea( WORD_FIRMWARE_OFFSET, twoWords, 2*sizeof(int32_t), /*bar*/ 0 );
  BOOST_CHECK( (twoWords[0] == firmwareContent) && (twoWords[1] ==compilationContent) );
  
  // now try to read only six of the eight bytes. This should throw an exception because it is
  // not a multiple of 4.
  BOOST_CHECK_THROW( _pcieDevice.readArea( /*offset*/ 0, twoWords, /*nBytes*/ 6, /*bar*/ 0 ),
		     exDevPCIE );

}


void PcieDeviceTest::testWriteArea(){
  //FIXME: Change the driver to have the standard register set and adapt this code
  
  // Read the two WORD_CLK_CNT words, write them and read them back
  int32_t originalClockCounts[2];
  int32_t increasedClockCounts[2];
  int32_t readbackClockCounts[2];
  
  _pcieDevice.readArea( WORD_CLK_CNT_OFFSET, originalClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
  increasedClockCounts[0] = originalClockCounts[0]+1;
  increasedClockCounts[1] = originalClockCounts[1]+1;
  _pcieDevice.writeArea( WORD_CLK_CNT_OFFSET, increasedClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
  _pcieDevice.readArea( WORD_CLK_CNT_OFFSET, readbackClockCounts, 2*sizeof(int32_t), /*bar*/ 0 );
  BOOST_CHECK( (increasedClockCounts[0] == readbackClockCounts[0]) && 
	       (increasedClockCounts[1] == readbackClockCounts[1]) );
  
  // now try to write only six of the eight bytes. This should throw an exception because it is
  // not a multiple of 4.
  BOOST_CHECK_THROW( _pcieDevice.writeArea(  WORD_CLK_CNT_OFFSET, originalClockCounts, /*nBytes*/ 6, /*bar*/ 0 ),
		     exDevPCIE );
}

void PcieDeviceTest::testReadDMA(){
  // Start the ADC on the dummy device. This will fill the "DMA" buffer with the default values (index^2) in the first 25 words.
  _pcieDevice.writeReg(WORD_ADC_ENA_OFFSET, 1 , /*bar*/ 0);
  
  int32_t dmaUserBuffer[N_WORDS_DMA];
  for (size_t i = 0; i < N_WORDS_DMA; ++i)
  {
    dmaUserBuffer[i] = -1;
  }
  
  _pcieDevice.readDMA( /*offset*/ 0, dmaUserBuffer, N_WORDS_DMA*sizeof(int32_t), /*the dma bar*/ 2 );

  std::string errorMessage = checkDmaValues( dmaUserBuffer, N_WORDS_DMA );
  BOOST_CHECK_MESSAGE( errorMessage.empty(), errorMessage  );
  
}

void PcieDeviceTest::testWriteDMA(){
  
}

void PcieDeviceTest::testReadDeviceInfo(){
  // The device info returns slot and driver version (major and minor).
  // For the dummy major and minor are the same as firmware and compilation, respectively.
  int32_t major;
  _pcieDevice.readReg(WORD_FIRMWARE_OFFSET, &major, /*bar*/ 0);
  int32_t minor;
  _pcieDevice.readReg(WORD_COMPILATION_OFFSET, &minor, /*bar*/ 0);
  std::stringstream referenceInfo;
  referenceInfo << "SLOT: "<< _slot << " DRV VER: " 
		<< major << "." << minor;

  std::string deviceInfo;
  _pcieDevice.readDeviceInfo( &deviceInfo );

  BOOST_CHECK( referenceInfo.str() == deviceInfo );
}

void PcieDeviceTest::testClose(){
  _pcieDevice.closeDev();
  BOOST_CHECK( _pcieDevice.isOpen() == false );
}

//check that the exception is thrown if the device is not opened
void PcieDeviceTest::testFailIfClosed()
{
  //FIXME: Change the driver to have the standard register set and adapt this code
  
  // We use the  WORD_USER_OFFSET register in bar 0 for all operations. It is read/write.
  int32_t dataWord = 0; // Just use one single word, even for dma. nothing should be executed anyway.

  _pcieDevice.closeDev();
  BOOST_CHECK_THROW( _pcieDevice.readReg(WORD_USER_OFFSET, &dataWord, /*bar*/ 0),
		     exDevPCIE );
  BOOST_CHECK_THROW( _pcieDevice.readArea(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
		     exDevPCIE );
  BOOST_CHECK_THROW( _pcieDevice.readDMA(0, &dataWord, sizeof(dataWord), /*bar*/ 0),
		     exDevPCIE );
  BOOST_CHECK_THROW( _pcieDevice.writeReg(WORD_USER_OFFSET, 0, /*bar*/ 0),
		     exDevPCIE );
  BOOST_CHECK_THROW( _pcieDevice.writeArea(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
		     exDevPCIE );
  BOOST_CHECK_THROW( _pcieDevice.writeDMA(WORD_USER_OFFSET, &dataWord, sizeof(dataWord), /*bar*/ 0),
		     exDevPCIE );

  std::string deviceInfo;
  BOOST_CHECK_THROW( _pcieDevice.readDeviceInfo(&deviceInfo),
		     exDevPCIE );

}

std::string PcieDeviceTest::checkDmaValues( int32_t * dmaBuffer, size_t nWords ) {
  bool dmaValuesOK = true;
  size_t i; // we need this after the loop
  for ( i=0; i < nWords; ++i)
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
