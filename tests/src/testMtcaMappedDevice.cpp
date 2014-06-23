#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "devMap.h"

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

  void testGetRegObjectGetRegisterInfo();

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
	add( BOOST_CLASS_TEST_CASE( &MtcaMappedDeviceTest::testGetRegObjectGetRegisterInfo, mtcaMappedDeviceTest ) );
	
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

void MtcaMappedDeviceTest::testGetRegObjectGetRegisterInfo(){
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
