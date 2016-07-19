// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendRegisterAccessorTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
using namespace mtca4u;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( NumericAddressedBackendRegisterAccessorTestSuite )

// Test the creation by using all possible options in Device
BOOST_AUTO_TEST_CASE( testCreation ){
  // it is always a 1D-type register (for scalar it's just 1x1)
  BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
  Device device;
  device.open("DUMMYD1");

  // we only check the size. That writing/reading from the offsets is ok is checked elsewere
  // FIXME: Should it be moved here? seems really scattered around at the moment.

  // the full register
  auto accessor1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA");
  BOOST_CHECK(accessor1.getNElements() == 10);
  // just a part
  auto accessor2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA",5);
  BOOST_CHECK(accessor2.getNElements() == 5);
  // A part with offset
  auto accessor3 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA",3,4);
  BOOST_CHECK(accessor3.getNElements() == 3);
  // The rest of the register from an offset
  auto accessor4 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA",0 ,2);
  BOOST_CHECK(accessor4.getNElements() == 8);

  // some error cases:
  // too many elements requested
  BOOST_CHECK_THROW( device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 11), DeviceException );
  // offset exceeds range (or would result in accessor with 0 elements)
  BOOST_CHECK_THROW( device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 10), DeviceException );
  BOOST_CHECK_THROW( device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 11), DeviceException );
  // sum of requested elements and offset too large
  BOOST_CHECK_THROW( device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 5, 6), DeviceException );
  
  // get accessor in raw mode
  // FIXME: This was never used, so raw mode is never tested anywhere
  auto accessor5 = device.getOneDRegisterAccessor<int32_t>("MODULE1/TEST_AREA",0 ,0, {AccessMode::raw});
  BOOST_CHECK(accessor5.getNElements() == 10);
  // only int32_t works, other types fail
  BOOST_CHECK_THROW( device.getOneDRegisterAccessor<double>("MODULE1/TEST_AREA",0 ,0, {AccessMode::raw} ), DeviceException );
  
}


// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
