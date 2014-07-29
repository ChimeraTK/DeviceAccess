#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "devMap.h"
#include <cmath>

using namespace mtca4u;

typedef devMap<devPCIE> MtcaMappedDevice;

#define MAPPING_FILE_NAME "mtcadummy.map"
#define DUMMY_DEVICE_FILE_NAME "/dev/mtcadummys0"

class MtcaMappedDeviceTest
{
 public:
  MtcaMappedDeviceTest();
  
  void testOpenClose();
  static void testThrowIfNeverOpened();

  void testRegObject_getRegisterInfo();
  /** Read reading more than one word and working with offset. Check with all different data types.
   */
  void testRegObject_readBlock();

  /** Check that the default arguments work, which means reading of one word, and check the corner case 
   *  nWord==0;
   *  This is only checked for int and float, not all types.
   */
  void testRegObject_readSimple();

 private:
  MtcaMappedDevice _mappedDevice;
};

class MtcaMappedDeviceTestSuite : public test_suite {
public:
  MtcaMappedDeviceTestSuite() 
    : test_suite("devPCIE test suite") {
        // add member function test cases to a test suite
        boost::shared_ptr<MtcaMappedDeviceTest> mtcaMappedDeviceTest( new MtcaMappedDeviceTest );

	// add member functions using BOOST_CLASS_TEST_CASE
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testOpenClose, mtcaMappedDeviceTest ) );
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testRegObject_getRegisterInfo, mtcaMappedDeviceTest ) );
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testRegObject_readBlock, mtcaMappedDeviceTest ) );
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testRegObject_readSimple, mtcaMappedDeviceTest ) );
	
	add( BOOST_TEST_CASE( &MtcaMappedDeviceTest::testThrowIfNeverOpened ) );
	//        test_case* writeTestCase = BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testWrite, mtcaMappedDeviceTest );

	//	writeTestCase->depends_on( readTestCase );

	//        add( readTestCase );  
	//        add( writeTestCase );
  }
};

// Register the test suite with the testing system so it is executed.
test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "MtcaMappedDevice test suite";

  return new MtcaMappedDeviceTestSuite;
}

// The implementations of the individual tests

void MtcaMappedDeviceTest::testOpenClose() {
  // test all tree open functions
  BOOST_CHECK_NO_THROW( _mappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, MAPPING_FILE_NAME ) );
  BOOST_CHECK_NO_THROW( _mappedDevice.closeDev() );

  BOOST_CHECK_NO_THROW( _mappedDevice.openDev( std::make_pair(DUMMY_DEVICE_FILE_NAME, MAPPING_FILE_NAME ) ) );
  BOOST_CHECK_NO_THROW( _mappedDevice.closeDev() );

  devMap<devBase> mappedDeviceAsBase;

  boost::shared_ptr<devBase> dummyDevice( new devPCIE );
  dummyDevice->openDev( DUMMY_DEVICE_FILE_NAME );

  mapFileParser fileParser;
  boost::shared_ptr<mapFile> registerMapping = fileParser.parse(MAPPING_FILE_NAME);
			
  BOOST_CHECK_NO_THROW( mappedDeviceAsBase.openDev( dummyDevice, registerMapping ) );
  BOOST_CHECK_NO_THROW( mappedDeviceAsBase.closeDev() );
}

MtcaMappedDeviceTest::MtcaMappedDeviceTest()
{}

void MtcaMappedDeviceTest::testThrowIfNeverOpened() {
  MtcaMappedDevice virginMappedDevice;
  
  int32_t dataWord;
  BOOST_CHECK_THROW( virginMappedDevice.closeDev(), exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.readReg(0 /*regOffset*/, &dataWord, 0 /*bar*/), exdevMap ) ;
  BOOST_CHECK_THROW( virginMappedDevice.writeReg(0 /*regOffset*/, dataWord, 0 /*bar*/), exdevMap ); 
  BOOST_CHECK_THROW( virginMappedDevice.readArea(0 /*regOffset*/, &dataWord, 4 /*size*/, 0 /*bar*/), exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.writeArea(0 /*regOffset*/, &dataWord, 4 /*size*/, 0 /*bar*/), exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.readDMA(0 /*regOffset*/, &dataWord, 4 /*size*/, 0 /*bar*/), exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.writeDMA(0 /*regOffset*/, &dataWord, 4 /*size*/, 0 /*bar*/), exdevMap );

  std::string deviceInfo;
  BOOST_CHECK_THROW( virginMappedDevice.readDeviceInfo(&deviceInfo), exdevMap );

  BOOST_CHECK_THROW( virginMappedDevice.readReg("irrelevant", &dataWord),  exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.writeReg("irrelevant", &dataWord),  exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.readDMA("irrelevant", &dataWord),  exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.writeDMA("irrelevant", &dataWord),  exdevMap );
    
    
  BOOST_CHECK_THROW( MtcaMappedDevice::regObject  myRegObject = virginMappedDevice.getRegObject("irrelevant"),  exdevMap );
}

void MtcaMappedDeviceTest::testRegObject_getRegisterInfo(){
  _mappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, MAPPING_FILE_NAME );
  // Sorry, this test is hard coded against the mtcadummy implementation.
  MtcaMappedDevice::regObject registerAccessor = _mappedDevice.getRegObject("AREA_DMA");
  mapFile::mapElem registerInfo = registerAccessor.getRegisterInfo();
  BOOST_CHECK( registerInfo.reg_address == 0x0 );
  BOOST_CHECK( registerInfo.reg_elem_nr == 0x400);
  BOOST_CHECK( registerInfo.reg_size = 0x1000 );
  BOOST_CHECK( registerInfo.reg_bar == 2 );
  BOOST_CHECK( registerInfo.reg_name == "AREA_DMA");
}

void MtcaMappedDeviceTest::testRegObject_readBlock(){
  // trigger the "DAQ" sequence which writes i*i into the first 25 registers, so we know what we have
  int32_t tempWord=0;
  _mappedDevice.writeReg("WORD_ADC_ENA", &tempWord);
  tempWord=1;
  _mappedDevice.writeReg("WORD_ADC_ENA", &tempWord);

  MtcaMappedDevice::regObject registerAccessor = _mappedDevice.getRegObject("AREA_DMA");

  // there are 25 elements with value i*i. ignore the first 2
  static const size_t N_ELEMENTS = 23;
  static const size_t OFFSET_ELEMENTS = 2;
  static const size_t OFFSET_BYTES = OFFSET_ELEMENTS * sizeof(int32_t);
  
  std::vector<int32_t> int32Buffer(N_ELEMENTS);
  registerAccessor.read(&int32Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  // pre-check: make sure we know what we get
  for (size_t i=0; i < N_ELEMENTS; ++i){
    BOOST_CHECK(int32Buffer[i] == static_cast<int>((i+OFFSET_ELEMENTS) * (i+OFFSET_ELEMENTS)) );
  }
  
  // change the fractional parameters and test the read
  // We go for 1 fractional bit, 10 bits, signed
  registerAccessor.setFixedPointConversion(10, 1, true);

  registerAccessor.read(&int32Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<uint32_t> uint32Buffer(N_ELEMENTS);
  registerAccessor.read(&uint32Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<int16_t> int16Buffer(N_ELEMENTS);
  registerAccessor.read(&int16Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<uint16_t> uint16Buffer(N_ELEMENTS);
  registerAccessor.read(&uint16Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<int8_t> int8Buffer(N_ELEMENTS);
  registerAccessor.read(&int8Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<uint8_t> uint8Buffer(N_ELEMENTS);
  registerAccessor.read(&uint8Buffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<float> floatBuffer(N_ELEMENTS);
  registerAccessor.read(&floatBuffer[0], N_ELEMENTS, OFFSET_BYTES );

  std::vector<double> doubleBuffer(N_ELEMENTS);
  registerAccessor.read(&doubleBuffer[0], N_ELEMENTS, OFFSET_BYTES );


  // now test different template types:
  for (size_t i=0; i < N_ELEMENTS; ++i){
    int rawValue = (i+OFFSET_ELEMENTS) * (i+OFFSET_ELEMENTS);
    double value;
    if ( rawValue & 0x200 ){ // the sign bit for a 10 bit integer
      value = static_cast<double>(static_cast<int>(rawValue|0xFFFFFE00))/2.; //negative, 1 fractional bit
    }else{
      value = static_cast<double>(0x1FF&rawValue)/2.; // positive, 1 fractional bit
    }
    
    std::stringstream errorMessage;
    errorMessage << "Index " << i <<", expected " << static_cast<int>(round(value)) << "("
    << value <<") and read " << int32Buffer[i]; 
    BOOST_CHECK_MESSAGE(int32Buffer[i] == static_cast<int>(round(value)), errorMessage.str() );
    BOOST_CHECK(uint32Buffer[i] == static_cast<uint32_t>(round(value)) );
    BOOST_CHECK(int16Buffer[i] == static_cast<int16_t>(round(value)) );
    BOOST_CHECK(uint16Buffer[i] == static_cast<uint16_t>(round(value)) );
    BOOST_CHECK(int8Buffer[i] == static_cast<int8_t>(round(value)) );
    BOOST_CHECK(uint8Buffer[i] == static_cast<uint8_t>(round(value)) );

    BOOST_CHECK(floatBuffer[i] == value ); 
    BOOST_CHECK(doubleBuffer[i] == value ); 
  }
}

void MtcaMappedDeviceTest::testRegObject_readSimple(){

  MtcaMappedDevice::regObject registerAccessor = _mappedDevice.getRegisterAccessor("WORD_USER");
  static const int inputValue = 0xFA5;
  registerAccessor.writeReg(&inputValue);

  // change the fractional parameters and test the read
  // We go for 3 fractional bits, 12 bits, signed, just to be different from the other setting
  registerAccessor.setFixedPointConversion(12, 3, true);

  int32_t myInt=0;
  registerAccessor.read(&myInt);

  BOOST_CHECK( myInt == static_cast<int>(0xFFFFFFF5) );
  
  myInt=17;
  registerAccessor.read(&myInt,0);
 
  // the int has to be untouched
  BOOST_CHECK( myInt == 17);

  double myDouble=0;
  registerAccessor.read(&myDouble);
  BOOST_CHECK( myDouble == -11.375 );

}
