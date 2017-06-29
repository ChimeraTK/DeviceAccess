// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendRegisterAccessorTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "TransferGroup.h"
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

BOOST_AUTO_TEST_CASE( testReadWrite ){
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");
  
  auto accessor = device.getScalarRegisterAccessor<int>("MODULE0/WORD_USER1");

  // FIXME: systematically test reading and writing. Currently is scattered all over the place...
}

BOOST_AUTO_TEST_CASE( testRawWrite ){
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");

  auto accessor1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0,  {AccessMode::raw});
  for (auto & value : accessor1){
    value = 0xFF;
  }
  accessor1.write();

  // another accessor for reading the same register
  auto accessor2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0,  {AccessMode::raw});
  accessor2.read();
  for (auto & value : accessor2){
    BOOST_CHECK(value == 0xFF);
  }

  for (auto & value : accessor1){
    value = 0x77;
  }
  accessor1.write();
  for (auto & value : accessor1){
    BOOST_CHECK(value == 0x77);
  } 

  accessor2.read();
  for (auto & value : accessor2){
    BOOST_CHECK(value == 0x77);
  } 

  // do not change the content of accessor1. suspicion: it has old, swapped data
  accessor1.write();
  accessor2.read();
  for (auto & value : accessor2){
    BOOST_CHECK(value == 0x77);
  } 
  
}

BOOST_AUTO_TEST_CASE( testRawWithTransferGroup ){
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");

  // the whole register
  auto a1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 2, 0,  {AccessMode::raw});
  auto a2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 2, 2,  {AccessMode::raw});

  // the whole register in a separate accessor which is not in the group
  auto standalone = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0,  {AccessMode::raw});

  // start with a single accessor so the low level transfer element is not shared
  TransferGroup group;
  group.addAccessor(a1);

  for (auto & value : a1){
    value = 0x77;
  }
  group.write();
  
  standalone.read();
  BOOST_CHECK(standalone[0] == 0x77);
  BOOST_CHECK(standalone[1] == 0x77);

  // check that the swapping works as intended
  for (auto & value : a1){
    value = 0xFF;
  }

  //writing twice without modifying the buffer certainly has to work
  //In case the old values have accidentally been swapped out and not back in this is not the case, which would be a bug
  for (int i=0; i<2; ++i){
    group.write();
    // writing must not swap away the buffer
    for (auto & value : a1){
      BOOST_CHECK(value == 0xFF);
    }
    standalone.read();
    BOOST_CHECK(standalone[0] == 0xFF);
    BOOST_CHECK(standalone[1] == 0xFF);
  }

  //test reading and mixed reading/writing
  standalone[0] = 0xAA;
  standalone[1] = 0xAA;
  standalone.write();
  
  for (int i=0; i<2; ++i){
    group.read();
    for (auto & value : a1){
      BOOST_CHECK(value == 0xAA);
    }
  }

  standalone[0] = 0xAB;
  standalone[1] = 0xAB;
  standalone.write();

  group.read();
  group.write();
  for (auto & value : a1){
    BOOST_CHECK(value == 0xAB);
  }
    
  standalone.read();
  BOOST_CHECK(standalone[0] == 0xAB);
  BOOST_CHECK(standalone[1] == 0xAB);

  //initialise the words pointed to by a2
  for (auto & value : a2){
    value = 0x77;
  }
  a2.write();
  
  // Now add the second accessor of the same register to the group and repeat the tests
  // They will share the same low level transfer element.
  group.addAccessor(a2);
  for (auto & value : a1){
    value = 0xFD;
  }
  for (auto & value : a2){
    value = 0xFE;
  }

  for (int i=0; i<2; ++i){
    group.write();
    for (auto & value : a1){
      BOOST_CHECK(value == 0xFD);
    }
    for (auto & value : a2){
      BOOST_CHECK(value == 0xFE);
    }
    standalone.read();
    BOOST_CHECK(standalone[0] == 0xFD);
    BOOST_CHECK(standalone[1] == 0xFD);
    BOOST_CHECK(standalone[2] == 0xFE);
    BOOST_CHECK(standalone[3] == 0xFE);
  }

  standalone[0] = 0xA1;
  standalone[1] = 0xA2;
  standalone[2] = 0xA3;
  standalone[3] = 0xA4;
  standalone.write();

  group.read();
  group.write();
  BOOST_CHECK(a1[0] == 0xA1);
  BOOST_CHECK(a1[1] == 0xA2); 
  BOOST_CHECK(a2[0] == 0xA3);
  BOOST_CHECK(a2[1] == 0xA4);
   
  standalone.read();
  BOOST_CHECK(standalone[0] == 0xA1);
  BOOST_CHECK(standalone[1] == 0xA2);
  BOOST_CHECK(standalone[2] == 0xA3);
  BOOST_CHECK(standalone[3] == 0xA4);

}


// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
