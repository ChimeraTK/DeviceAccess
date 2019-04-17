#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendRegisterAccessorTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "TransferGroup.h"
#include "BackendFactory.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(NumericAddressedBackendRegisterAccessorTestSuite)

// Test the creation by using all possible options in Device
BOOST_AUTO_TEST_CASE(testCreation) {
  // it is always a 1D-type register (for scalar it's just 1x1)
  BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
  Device device;
  device.open("DUMMYD1");

  // we only check the size. That writing/reading from the offsets is ok is
  // checked elsewere
  // FIXME: Should it be moved here? seems really scattered around at the
  // moment.

  // the full register
  auto accessor1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA");
  BOOST_CHECK(accessor1.getNElements() == 10);
  // just a part
  auto accessor2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 5);
  BOOST_CHECK(accessor2.getNElements() == 5);
  // A part with offset
  auto accessor3 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 3, 4);
  BOOST_CHECK(accessor3.getNElements() == 3);
  // The rest of the register from an offset
  auto accessor4 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 2);
  BOOST_CHECK(accessor4.getNElements() == 8);

  // some error cases:
  // too many elements requested
  BOOST_CHECK_THROW(device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 11), ChimeraTK::logic_error);
  // offset exceeds range (or would result in accessor with 0 elements)
  BOOST_CHECK_THROW(device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 10), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 11), ChimeraTK::logic_error);
  // sum of requested elements and offset too large
  BOOST_CHECK_THROW(device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 5, 6), ChimeraTK::logic_error);

  // get accessor in raw mode
  // FIXME: This was never used, so raw mode is never tested anywhere
  auto accessor5 = device.getOneDRegisterAccessor<int32_t>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw});
  BOOST_CHECK(accessor5.getNElements() == 10);
  // only int32_t works, other types fail
  BOOST_CHECK_THROW(
      device.getOneDRegisterAccessor<double>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw}), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testReadWrite) {
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");

  auto accessor = device.getScalarRegisterAccessor<int>("MODULE0/WORD_USER1");

  // FIXME: systematically test reading and writing. Currently is scattered all
  // over the place...
}

BOOST_AUTO_TEST_CASE(testRawWrite) {
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");

  auto accessor1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw});
  for(auto& value : accessor1) {
    value = 0xFF;
  }
  accessor1.write();

  // another accessor for reading the same register
  auto accessor2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw});
  accessor2.read();
  for(auto& value : accessor2) {
    BOOST_CHECK(value == 0xFF);
  }

  for(auto& value : accessor1) {
    value = 0x77;
  }
  accessor1.write();
  for(auto& value : accessor1) {
    BOOST_CHECK(value == 0x77);
  }

  accessor2.read();
  for(auto& value : accessor2) {
    BOOST_CHECK(value == 0x77);
  }

  // do not change the content of accessor1. suspicion: it has old, swapped data
  accessor1.write();
  accessor2.read();
  for(auto& value : accessor2) {
    BOOST_CHECK(value == 0x77);
  }
}

BOOST_AUTO_TEST_CASE(testRawWithTransferGroup) {
  Device device;
  device.open("sdm://./dummy=goodMapFile.map");

  // the whole register
  auto a1 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 2, 0, {AccessMode::raw});
  auto a2 = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 2, 2, {AccessMode::raw});

  // the whole register in a separate accessor which is not in the group
  auto standalone = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw});

  // start with a single accessor so the low level transfer element is not
  // shared
  TransferGroup group;
  group.addAccessor(a1);

  for(auto& value : a1) {
    value = 0x77;
  }
  group.write();

  standalone.read();
  BOOST_CHECK(standalone[0] == 0x77);
  BOOST_CHECK(standalone[1] == 0x77);

  // check that the swapping works as intended
  for(auto& value : a1) {
    value = 0xFF;
  }

  // writing twice without modifying the buffer certainly has to work
  // In case the old values have accidentally been swapped out and not back in
  // this is not the case, which would be a bug
  for(int i = 0; i < 2; ++i) {
    group.write();
    // writing must not swap away the buffer
    for(auto& value : a1) {
      BOOST_CHECK(value == 0xFF);
    }
    standalone.read();
    BOOST_CHECK(standalone[0] == 0xFF);
    BOOST_CHECK(standalone[1] == 0xFF);
  }

  // test reading and mixed reading/writing
  standalone[0] = 0xAA;
  standalone[1] = 0xAA;
  standalone.write();

  for(int i = 0; i < 2; ++i) {
    group.read();
    for(auto& value : a1) {
      BOOST_CHECK(value == 0xAA);
    }
  }

  standalone[0] = 0xAB;
  standalone[1] = 0xAB;
  standalone.write();

  group.read();
  group.write();
  for(auto& value : a1) {
    BOOST_CHECK(value == 0xAB);
  }

  standalone.read();
  BOOST_CHECK(standalone[0] == 0xAB);
  BOOST_CHECK(standalone[1] == 0xAB);

  // initialise the words pointed to by a2
  for(auto& value : a2) {
    value = 0x77;
  }
  a2.write();

  // Now add the second accessor of the same register to the group and repeat
  // the tests They will share the same low level transfer element.
  group.addAccessor(a2);
  for(auto& value : a1) {
    value = 0xFD;
  }
  for(auto& value : a2) {
    value = 0xFE;
  }

  for(int i = 0; i < 2; ++i) {
    group.write();
    for(auto& value : a1) {
      BOOST_CHECK(value == 0xFD);
    }
    for(auto& value : a2) {
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

BOOST_AUTO_TEST_CASE(testConverterTypes) {
  //After the introduction of the IEEE754 floating point converter we have to test
  //that all possible converters (two at the moment) are created when they should,
  //and that raw and coocked accessors are working for all of them.

  //As we cannot rely on any NumericAddressedRegisterAccessor at the moment we use the
  //DummyRegisterRawAccessor to monitor what is going on in the target memory space on
  //the device
  auto deviceDescriptor = "(dummy?map=goodMapFile.map)";

  auto dummyBackend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend(deviceDescriptor));

  Device device;
  device.open(deviceDescriptor);

  // FixedPointConverter, raw and coocked accessors
  // MODULE0.WORD_USER1 is fixed point, 16 bit, 3 fractional, signed
  auto user1Dummy = dummyBackend->getRawAccessor("MODULE0", "WORD_USER1");
  user1Dummy = 0x4321;

  auto user1Coocked = device.getScalarRegisterAccessor<float>("MODULE0/WORD_USER1");
  user1Coocked.read();

  BOOST_CHECK_CLOSE(float(user1Coocked), 2148.125, 0.0001);

  user1Coocked = -1;
  user1Coocked.write();

  BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0xfff8);

  auto user1Raw = device.getScalarRegisterAccessor<int32_t>("MODULE0/WORD_USER1", 0, {AccessMode::raw});
  user1Raw.read();

  BOOST_CHECK_EQUAL(int32_t(user1Raw), 0xfff8);
  BOOST_CHECK_CLOSE(user1Raw.getAsCooked<float>(), -1, 0.0001);

  user1Raw.setAsCooked(-2.5);

  user1Raw.write();

  BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0xffec);

  // special case: int32 does not necessarily mean raw. There is also a cooked version:
  auto user1CoockedInt = device.getScalarRegisterAccessor<int32_t>("MODULE0/WORD_USER1");
  user1CoockedInt.read();

  BOOST_CHECK_EQUAL(int(user1CoockedInt), -3);

  user1CoockedInt = 16;
  user1CoockedInt.write();

  BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0x80);

  // IEEE754 converter, raw and coocked accessors
  // FLOAT_TEST.ARRAY is IEEE754. We use the 1 D version in constrast to FixedPoint where we use scalar (just because we can)
  auto floatTestDummy = dummyBackend->getRawAccessor("FLOAT_TEST", "ARRAY");

  float testValue = 1.1;
  void* warningAvoider = &testValue; // directly reinterpret-casting float gives compiler warnings
  floatTestDummy[0] = *(reinterpret_cast<int32_t*>(warningAvoider));
  testValue = 2.2;
  floatTestDummy[1] = *(reinterpret_cast<int32_t*>(warningAvoider));
  testValue = 3.3;
  floatTestDummy[2] = *(reinterpret_cast<int32_t*>(warningAvoider));
  testValue = 4.4;
  floatTestDummy[3] = *(reinterpret_cast<int32_t*>(warningAvoider));

  auto floatTestCoocked = device.getOneDRegisterAccessor<float>("FLOAT_TEST/ARRAY");
  floatTestCoocked.read();

  BOOST_CHECK_CLOSE(floatTestCoocked[0], 1.1, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[1], 2.2, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[2], 3.3, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[3], 4.4, 0.0001);

  floatTestCoocked[3] = 44.4;
  floatTestCoocked.write();

  *(reinterpret_cast<int32_t*>(warningAvoider)) = floatTestDummy[3];
  BOOST_CHECK_CLOSE(testValue, 44.4, 0.0001);

  auto floatTestRaw = device.getOneDRegisterAccessor<int32_t>("FLOAT_TEST/ARRAY", 0, 0, {AccessMode::raw});
  floatTestRaw.read();

  *(reinterpret_cast<int32_t*>(warningAvoider)) = floatTestRaw[2];

  BOOST_CHECK_CLOSE(testValue, 3.3, 0.0001);
  BOOST_CHECK_CLOSE(floatTestRaw.getAsCooked<float>(0), 1.1, 0.0001);

  floatTestRaw.setAsCooked(0, -2.5);

  floatTestRaw.write();

  *(reinterpret_cast<int32_t*>(warningAvoider)) = floatTestDummy[0];

  BOOST_CHECK_CLOSE(testValue, -2.5, 0.0001);

  // special case: int32 does not necessarily mean raw. There is also a cooked version:
  auto floatTestCoockedInt = device.getOneDRegisterAccessor<int32_t>("FLOAT_TEST/ARRAY");
  floatTestCoockedInt.read();

  BOOST_CHECK_EQUAL(floatTestCoockedInt[0], -3); // was -2.5
  BOOST_CHECK_EQUAL(floatTestCoockedInt[1], 2);  // was 2.2
  BOOST_CHECK_EQUAL(floatTestCoockedInt[2], 3);  // was 3.3
  BOOST_CHECK_EQUAL(floatTestCoockedInt[3], 44); // was 44.4

  floatTestCoockedInt[1] = 16;
  floatTestCoockedInt.write();

  *(reinterpret_cast<int32_t*>(warningAvoider)) = floatTestDummy[1];
  BOOST_CHECK_CLOSE(testValue, 16.0, 0.001);
}

BOOST_AUTO_TEST_CASE(registerCatalogueCreation) {
  Device d("sdm://./dummy=goodMapFile.map");
  auto catalogue = d.getRegisterCatalogue();
  BOOST_CHECK_NO_THROW(catalogue.getRegister("MODULE0/WORD_USER1"));

  BOOST_CHECK(d.isOpened() == false);
  BOOST_CHECK_NO_THROW(d.open());
  BOOST_CHECK(d.isOpened() == true);
}

// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
