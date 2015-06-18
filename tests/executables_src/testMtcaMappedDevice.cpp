#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "devMap.h"
#include <cmath>

using namespace mtca4u;

typedef devMap<devPCIE> MtcaMappedDevice;

#define VALID_MAPPING_FILE_NAME "mtcadummy_withoutModules.map"
#define DUMMY_DEVICE_FILE_NAME "/dev/mtcadummys0"

#define FXPNT_ERROR_1_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint1.map"
#define FXPNT_ERROR_2_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint2.map"
#define FXPNT_ERROR_3_MAPPING_FILE_NAME "mtcadummy_bad_fxpoint3.map"

class MtcaMappedDeviceTest
{
 public:
  MtcaMappedDeviceTest();
  
  void testOpenClose();
  static void testThrowIfNeverOpened();
  
  static void testMapFileParser_parse();

  void testRegObject_getRegisterInfo();
  /** Read reading more than one word and working with offset. Check with all different data types.
   */
  void testRegObject_readBlock();

  /** Check that the default arguments work, which means reading of one word, and check the corner case 
   *  nWord==0;
   *  This is only checked for int and double, not all types.
   *  Also checks the read convenience function.
   */
  void testRegObject_readSimple();

  /** Testing the write function with a multiple registers.
   */
  void testRegObject_writeBlock();

    /** Check that the default arguments work, which means writing of one word, and check the corner case 
   *  nWord==0;
   *  This is only checked for int and doubel, not all types.
   *  Also checks the write convenience function.
   */
  void testRegObject_writeSimple();

  void getRegistersInModule();
  void getRegisterAccerrossInModule();

 private:
  MtcaMappedDevice _mappedDevice;
  
  template<typename DataType>
  void testRegObject_typedWriteBlock(DataType offsetValue);
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
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testRegObject_writeBlock, mtcaMappedDeviceTest ) );
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testRegObject_writeSimple, mtcaMappedDeviceTest ) );
	
  add( BOOST_TEST_CASE( &MtcaMappedDeviceTest::testMapFileParser_parse) );
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
  BOOST_CHECK_NO_THROW( _mappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, VALID_MAPPING_FILE_NAME ) );
  BOOST_CHECK_NO_THROW( _mappedDevice.closeDev() );

  BOOST_CHECK_NO_THROW( _mappedDevice.openDev( std::make_pair(DUMMY_DEVICE_FILE_NAME, VALID_MAPPING_FILE_NAME ) ) );
  BOOST_CHECK_NO_THROW( _mappedDevice.closeDev() );

  devMap<devBase> mappedDeviceAsBase;
  // you cannot directly open a devMap of devBase. devBase is purely virtual and cannot be instantiated.
  BOOST_CHECK_THROW( mappedDeviceAsBase.openDev( DUMMY_DEVICE_FILE_NAME, VALID_MAPPING_FILE_NAME ),
		     exdevMap );

  // you have to create an instance of an implementation like devPCIe and pass it as a devBase pointer
  boost::shared_ptr<devBase> dummyDevice( new devPCIE );
  dummyDevice->openDev( DUMMY_DEVICE_FILE_NAME );

  mapFileParser fileParser;
  boost::shared_ptr<mapFile> registerMapping = fileParser.parse(VALID_MAPPING_FILE_NAME);
			
  BOOST_CHECK_NO_THROW( mappedDeviceAsBase.openDev( dummyDevice, registerMapping ) );

  // get of a smart pointer gives a raw pointer of the object it points to
  BOOST_CHECK( registerMapping.get() == mappedDeviceAsBase.getRegisterMap().get() );

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
    
    
  BOOST_CHECK_THROW( MtcaMappedDevice::regObject  myRegObject = virginMappedDevice.getRegObject("irrelevant"), 
		     exdevMap );
  BOOST_CHECK_THROW(
      boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
          myRegisterAccessor =
              virginMappedDevice.getRegisterAccessor("irrelevant"),
      exdevMap);
  BOOST_CHECK_THROW(
      boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
          myRegisterAccessor =
              virginMappedDevice.getRegisterAccessor("irrelevant"),
      exdevMap);
  BOOST_CHECK_THROW( virginMappedDevice.getRegistersInModule("irrelevant"), exdevMap );
  BOOST_CHECK_THROW( virginMappedDevice.getRegisterAccessorsInModule("irrelevant"), exdevMap );
}

void MtcaMappedDeviceTest::testMapFileParser_parse() {
  MtcaMappedDevice virginMappedDevice;
  BOOST_CHECK_THROW( virginMappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, FXPNT_ERROR_1_MAPPING_FILE_NAME ), exMapFileParser );
  BOOST_CHECK_THROW( virginMappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, FXPNT_ERROR_2_MAPPING_FILE_NAME ), exMapFileParser );
  BOOST_CHECK_THROW( virginMappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, FXPNT_ERROR_3_MAPPING_FILE_NAME ), exMapFileParser );
}

void MtcaMappedDeviceTest::testRegObject_getRegisterInfo(){
  _mappedDevice.openDev( DUMMY_DEVICE_FILE_NAME, VALID_MAPPING_FILE_NAME );
  // Sorry, this test is hard coded against the mtcadummy implementation.
  // PP: Is there a different way of testing it?
  MtcaMappedDevice::regObject registerAccessor = 
    _mappedDevice.getRegObject("AREA_DMAABLE");
  mapFile::mapElem registerInfo = registerAccessor.getRegisterInfo();
  BOOST_CHECK( registerInfo.reg_address == 0x0 );
  BOOST_CHECK( registerInfo.reg_elem_nr == 0x400);
  BOOST_CHECK( registerInfo.reg_size = 0x1000 );
  BOOST_CHECK( registerInfo.reg_bar == 2 );
  // the dmaable area has no fixed point settings, which has to result in
  // the default 32 0 true (like raw words)
  BOOST_CHECK( registerInfo.reg_width == 32 );
  BOOST_CHECK( registerInfo.reg_frac_bits == 0 );
  BOOST_CHECK( registerInfo.reg_signed == true );
  BOOST_CHECK( registerInfo.reg_name == "AREA_DMAABLE");
  
  registerAccessor = _mappedDevice.getRegObject("WORD_FIRMWARE");
  registerInfo = registerAccessor.getRegisterInfo();
  BOOST_CHECK( registerInfo.reg_name == "WORD_FIRMWARE");
  BOOST_CHECK( registerInfo.reg_address == 0x0 );
  BOOST_CHECK( registerInfo.reg_elem_nr == 0x1);
  BOOST_CHECK( registerInfo.reg_size = 0x4 );
  BOOST_CHECK( registerInfo.reg_bar == 0 );
  BOOST_CHECK( registerInfo.reg_width == 32 );
  BOOST_CHECK( registerInfo.reg_frac_bits == 0 );
  BOOST_CHECK( registerInfo.reg_signed == false );
  
  registerAccessor = _mappedDevice.getRegObject("WORD_INCOMPLETE_1");
  registerInfo = registerAccessor.getRegisterInfo();
  BOOST_CHECK( registerInfo.reg_name == "WORD_INCOMPLETE_1");
  BOOST_CHECK( registerInfo.reg_address == 0x60 );
  BOOST_CHECK( registerInfo.reg_elem_nr == 0x1);
  BOOST_CHECK( registerInfo.reg_size = 0x4 );
  BOOST_CHECK( registerInfo.reg_bar == 0 );
  BOOST_CHECK( registerInfo.reg_width == 13 );
  BOOST_CHECK( registerInfo.reg_frac_bits == 0 );
  BOOST_CHECK( registerInfo.reg_signed == true );
  
  registerAccessor = _mappedDevice.getRegObject("WORD_INCOMPLETE_2");
  registerInfo = registerAccessor.getRegisterInfo();
  BOOST_CHECK( registerInfo.reg_name == "WORD_INCOMPLETE_2");
  BOOST_CHECK( registerInfo.reg_address == 0x64 );
  BOOST_CHECK( registerInfo.reg_elem_nr == 0x1);
  BOOST_CHECK( registerInfo.reg_size = 0x4 );
  BOOST_CHECK( registerInfo.reg_bar == 0 );
  BOOST_CHECK( registerInfo.reg_width == 13 );
  BOOST_CHECK( registerInfo.reg_frac_bits == 8 );
  BOOST_CHECK( registerInfo.reg_signed == true );

}

void MtcaMappedDeviceTest::testRegObject_readBlock(){
  // trigger the "DAQ" sequence which writes i*i into the first 25 registers, so we know what we have
  int32_t tempWord=0;
  _mappedDevice.writeReg("WORD_ADC_ENA", &tempWord);
  tempWord=1;
  _mappedDevice.writeReg("WORD_ADC_ENA", &tempWord);

  MtcaMappedDevice::regObject registerAccessor = 
    _mappedDevice.getRegObject("AREA_DMAABLE");

  // there are 25 elements with value i*i. ignore the first 2
  static const size_t N_ELEMENTS = 23;
  static const size_t OFFSET_ELEMENTS = 2;
  
  std::vector<int32_t> int32Buffer(N_ELEMENTS,0);
  registerAccessor.read(&int32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  // pre-check: make sure we know what we get
  for (size_t i=0; i < N_ELEMENTS; ++i){
    BOOST_CHECK(int32Buffer[i] == static_cast<int>((i+OFFSET_ELEMENTS) * (i+OFFSET_ELEMENTS)) );
  }
  
  // Change the fractional parameters and test the read again. 
  // We use another accessor which accesses the same area with different fractioanal
  // settings (1 fractional bit, 10 bits, signed)
  MtcaMappedDevice::regObject registerAccessor10_1 =
    _mappedDevice.getRegObject("AREA_DMAABLE_FIXEDPOINT10_1");

  registerAccessor10_1.read(&int32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<uint32_t> uint32Buffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&uint32Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<int16_t> int16Buffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&int16Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<uint16_t> uint16Buffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&uint16Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<int8_t> int8Buffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&int8Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<uint8_t> uint8Buffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&uint8Buffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<float> floatBuffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&floatBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS );

  std::vector<double> doubleBuffer(N_ELEMENTS,0);
  registerAccessor10_1.read(&doubleBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS );


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

  boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
  registerAccessor = _mappedDevice.getRegisterAccessor("WORD_USER");
  // 3 fractional bits, 12 bits, signed (from the map file)

  static const int inputValue = 0xFA5;
  registerAccessor->writeReg(&inputValue);

  int32_t myInt=0;
  registerAccessor->read(&myInt);

  BOOST_CHECK( myInt == -11 );
  
  myInt=17;
  registerAccessor->read(&myInt,0);

  // the int has to be untouched
  BOOST_CHECK( myInt == 17);

  myInt = registerAccessor->read<int>();
  BOOST_CHECK( myInt == static_cast<int>(0xFFFFFFF5) );
 
  double myDouble=0;
  registerAccessor->read(&myDouble);
  BOOST_CHECK( myDouble == -11.375 );

  myDouble=0;
  myDouble = registerAccessor->read<double>();
  BOOST_CHECK( myDouble == -11.375 );
}

template<typename DataType>
void MtcaMappedDeviceTest::testRegObject_typedWriteBlock(DataType offsetValue){
  // there are 25 elements. ignore the first 2
  static const size_t N_ELEMENTS = 23;
  static const size_t N_BYTES = N_ELEMENTS * sizeof(int32_t);
  static const size_t OFFSET_ELEMENTS = 2;
 
  std::vector<DataType> writeBuffer(N_ELEMENTS);
  
  //write i+offset to all arrays 
  for (size_t i=0; i < N_ELEMENTS; ++i){
    writeBuffer[i]=i+offsetValue;
  }

  boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
  registerAccessor =
      _mappedDevice.getRegisterAccessor("AREA_DMAABLE_FIXEDPOINT16_3");
  // 16 bits, 3 fractional signed

  // use raw write to zero the registers
  static const std::vector<int32_t> zeroedBuffer(N_ELEMENTS, 0);
  registerAccessor->writeReg(&zeroedBuffer[0], N_BYTES, OFFSET_ELEMENTS * sizeof(int32_t));

  registerAccessor->write( &writeBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);

  // we already tested that read works, so just read back and compare that we get what we wrote
  std::vector<DataType> readBuffer(N_ELEMENTS,0);
  registerAccessor->read( &readBuffer[0], N_ELEMENTS, OFFSET_ELEMENTS);
   for (size_t i=0; i < N_ELEMENTS; ++i){
     BOOST_CHECK( writeBuffer[i] == readBuffer[i] );
  }      
}

void MtcaMappedDeviceTest::testRegObject_writeBlock(){
  // the tested values run from startValue to startValue+23, so small negative numbers test positive and 
  // negative values
  testRegObject_typedWriteBlock(static_cast<uint32_t>(14));
  testRegObject_typedWriteBlock(static_cast<int32_t>(-14)); // also test negative values with signed ints
  testRegObject_typedWriteBlock(static_cast<uint16_t>(14));
  testRegObject_typedWriteBlock(static_cast<int16_t>(-14)); // also test negative values with signed ints
  testRegObject_typedWriteBlock(static_cast<uint8_t>(14));
  testRegObject_typedWriteBlock(static_cast<int8_t>(-14)); // also test negative values with signed ints
  testRegObject_typedWriteBlock(static_cast<double>(-13.75));
  testRegObject_typedWriteBlock(static_cast<float>(-13.75));
}

void MtcaMappedDeviceTest::testRegObject_writeSimple(){
  boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
  registerAccessor = _mappedDevice.getRegisterAccessor("WORD_USER");
  // the word has 3 fractional bits, 12 bits, signed, just to be different from the
  // other setting. Values coming from the map file.

  static const int startValue = 0;
  // write something we are going to change
  registerAccessor->writeReg(&startValue);


  int32_t myInt=-14;
  // write and read back
  registerAccessor->write(&myInt,1);

  int32_t readbackValue=0;
  registerAccessor->readReg(&readbackValue);
  BOOST_CHECK( static_cast<uint32_t>(readbackValue) == 0xF90 );
  
  myInt=17;
  registerAccessor->write(&myInt,0);
  // nothing should be written, should still read the same
  readbackValue=0;
  registerAccessor->readReg(&readbackValue);
  BOOST_CHECK( static_cast<uint32_t>(readbackValue) == 0xF90 );
 
  registerAccessor->write(-17);
  BOOST_CHECK( registerAccessor->read<int>() == -17 );

  double myDouble=-13.75;
  registerAccessor->write(&myDouble,1);
  readbackValue=0;
  registerAccessor->readReg(&readbackValue);
  BOOST_CHECK( static_cast<uint32_t>(readbackValue) == 0xF92 );

  registerAccessor->write(-17.25);
  BOOST_CHECK( registerAccessor->read<double>() == -17.25 );
}
