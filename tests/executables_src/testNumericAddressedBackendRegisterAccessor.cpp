// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE NumericAddressedBackendRegisterAccessorTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "accessPrivateData.h"
#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "TransferGroup.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// we need to access some private data of the low level transfer element
// FIXME: redesign test so this is not necessary!
struct NumericAddressedLowLevelTransferElement_startAddress {
  typedef uint64_t NumericAddressedLowLevelTransferElement::* type;
};
template struct accessPrivateData::stow_private<NumericAddressedLowLevelTransferElement_startAddress,
    &ChimeraTK::NumericAddressedLowLevelTransferElement::_startAddress>;

struct NumericAddressedLowLevelTransferElement_numberOfBytes {
  typedef size_t NumericAddressedLowLevelTransferElement::* type;
};
template struct accessPrivateData::stow_private<NumericAddressedLowLevelTransferElement_numberOfBytes,
    &ChimeraTK::NumericAddressedLowLevelTransferElement::_numberOfBytes>;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(NumericAddressedBackendRegisterAccessorTestSuite)

/**********************************************************************************************************************/

// Test the creation by using all possible options in Device
BOOST_AUTO_TEST_CASE(testCreation) {
  // it is always a 1D-type register (for scalar it's just 1x1)
  BackendFactory::getInstance().setDMapFilePath("./dummies.dmap");
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

  // some error cases:
  // too many elements requested
  BOOST_CHECK_THROW(std::ignore = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 11), ChimeraTK::logic_error);
  // offset exceeds range (or would result in accessor with 0 elements)
  BOOST_CHECK_THROW(
      std::ignore = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 10), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      std::ignore = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 11), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      std::ignore = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 0, 2), ChimeraTK::logic_error);
  // sum of requested elements and offset too large
  BOOST_CHECK_THROW(
      std::ignore = device.getOneDRegisterAccessor<int>("MODULE1/TEST_AREA", 5, 6), ChimeraTK::logic_error);

  // get accessor in raw mode
  // FIXME: This was never used, so raw mode is never tested anywhere
  auto accessor5 = device.getOneDRegisterAccessor<int32_t>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw});
  BOOST_CHECK(accessor5.getNElements() == 10);
  // only int32_t works, other types fail
  BOOST_CHECK_THROW(std::ignore = device.getOneDRegisterAccessor<double>("MODULE1/TEST_AREA", 0, 0, {AccessMode::raw}),
      ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWrite) {
  Device device;
  device.open("(dummy?map=goodMapFile.map)");

  auto accessor = device.getScalarRegisterAccessor<int>("MODULE0/WORD_USER1");

  // FIXME: systematically test reading and writing. Currently is scattered all
  // over the place...
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadOnly) {
  Device device;
  device.open("(dummy?map=goodMapFile.map)");

  auto accToReadOnly = device.getScalarRegisterAccessor<int>("MODULE1/WORD_USER3");

  BOOST_CHECK(accToReadOnly.isReadOnly());
  BOOST_CHECK(!accToReadOnly.isWriteable());

  BOOST_CHECK_THROW(accToReadOnly.write(), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRawWrite) {
  Device device;
  device.open("(dummy?map=goodMapFile.map)");

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

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRawWithTransferGroup) {
  Device device;
  device.open("(dummy?map=goodMapFile.map)");

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

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testConversionTypes) {
  // The bit interpretation can either have a fixed number of fractional bits or be IEEE754 single precision.
  // We test those scenarios, and that raw and coocked accessors are working for all of them.

  // As we cannot rely on any NumericAddressedRegisterAccessor at the moment we use the
  // DummyRegisterRawAccessor to monitor what is going on in the target memory space on
  // the device
  const char* deviceDescriptor = "(dummy?map=goodMapFile.map)";

  auto dummyBackend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend(deviceDescriptor));

  Device device;
  device.open(deviceDescriptor);

  // Fixed point convertion, raw and coocked accessors
  // MODULE0.WORD_USER1 is fixed point, 16 bit, 3 fractional, signed
  auto user1Dummy = dummyBackend->getRawAccessor("MODULE0", "WORD_USER1");
  // Demonstrate the correct usage of the raw accessor, with using the lock. It would not be strictly needed here
  // because this is single threaded, but in general it is required.
  {
    auto bufferLock = user1Dummy.getBufferLock();
    user1Dummy = 0x4321;
  }

  auto user1Coocked = device.getScalarRegisterAccessor<float>("MODULE0/WORD_USER1");
  user1Coocked.read();

  BOOST_CHECK_CLOSE(float(user1Coocked), 2148.125, 0.0001);

  user1Coocked = -1;
  user1Coocked.write();

  {
    auto bufferLock = user1Dummy.getBufferLock();
    BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0xfff8);
  }

  auto user1Raw = device.getScalarRegisterAccessor<int32_t>("MODULE0/WORD_USER1", 0, {AccessMode::raw});
  user1Raw.read();

  BOOST_CHECK_EQUAL(int32_t(user1Raw), 0xfff8);
  BOOST_CHECK_CLOSE(user1Raw.getAsCooked<float>(), -1, 0.0001);

  user1Raw.setAsCooked(-2.5);

  user1Raw.write();

  {
    auto bufferLock = user1Dummy.getBufferLock();
    BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0xffec);
  }

  // special case: int32 does not necessarily mean raw. There is also a cooked version:
  auto user1CoockedInt = device.getScalarRegisterAccessor<int32_t>("MODULE0/WORD_USER1");
  user1CoockedInt.read();

  BOOST_CHECK_EQUAL(int(user1CoockedInt), -3);

  user1CoockedInt = 16;
  user1CoockedInt.write();

  {
    auto bufferLock = user1Dummy.getBufferLock();
    BOOST_CHECK_EQUAL(int32_t(user1Dummy), 0x80);
  }

  // IEEE754 conversion, raw and coocked accessors
  // FLOAT_TEST.ARRAY is IEEE754. We use the 1 D version in contrast to fixed point case, where we use scalar (just
  // because we can)
  auto floatTestDummy = dummyBackend->getRawAccessor("FLOAT_TEST", "ARRAY");

  float testValue = 1.1;
  {
    auto bufferLock = floatTestDummy.getBufferLock();
    floatTestDummy[0] = std::bit_cast<int32_t>(testValue);
    testValue = 2.2;
    floatTestDummy[1] = std::bit_cast<int32_t>(testValue);
    testValue = 3.3;
    floatTestDummy[2] = std::bit_cast<int32_t>(testValue);
    testValue = 4.4;
    floatTestDummy[3] = std::bit_cast<int32_t>(testValue);
  } // release buffer lock

  auto floatTestCoocked = device.getOneDRegisterAccessor<float>("FLOAT_TEST/ARRAY");
  floatTestCoocked.read();

  BOOST_CHECK_CLOSE(floatTestCoocked[0], 1.1, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[1], 2.2, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[2], 3.3, 0.0001);
  BOOST_CHECK_CLOSE(floatTestCoocked[3], 4.4, 0.0001);

  floatTestCoocked[3] = 44.4;
  floatTestCoocked.write();

  {
    auto bufferLock = floatTestDummy.getBufferLock();
    testValue = std::bit_cast<float>(floatTestDummy[3]);
  }
  BOOST_CHECK_CLOSE(testValue, 44.4, 0.0001);

  auto floatTestRaw = device.getOneDRegisterAccessor<int32_t>("FLOAT_TEST/ARRAY", 0, 0, {AccessMode::raw});
  floatTestRaw.read();

  testValue = std::bit_cast<float>(floatTestRaw[2]);

  BOOST_CHECK_CLOSE(testValue, 3.3, 0.0001);
  BOOST_CHECK_CLOSE(floatTestRaw.getAsCooked<float>(0), 1.1, 0.0001);

  floatTestRaw.setAsCooked(0, -2.5);

  floatTestRaw.write();

  {
    auto bufferLock = floatTestDummy.getBufferLock();
    testValue = std::bit_cast<float>(floatTestDummy[0]);
  }
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

  {
    auto bufferLock = floatTestDummy.getBufferLock();
    testValue = std::bit_cast<float>(floatTestDummy[1]);
  }
  BOOST_CHECK_CLOSE(testValue, 16.0, 0.001);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(registerCatalogueCreation) {
  Device d("(dummy?map=goodMapFile.map)");
  auto catalogue = d.getRegisterCatalogue();
  BOOST_CHECK_NO_THROW((void)catalogue.getRegister("MODULE0/WORD_USER1"));

  BOOST_CHECK(d.isOpened() == false);
  BOOST_CHECK_NO_THROW(d.open());
  BOOST_CHECK(d.isOpened() == true);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMergeNumericRegisters) {
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux1 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_1");
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // create the same register accessors again, so we have a second set not part
  // of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux1b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_1");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // obtain the private pointers to the implementation of the accessor
  auto mux0i = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(mux0.getHighLevelImplElement());
  auto mux1i = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(mux1.getHighLevelImplElement());
  auto mux2i = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(mux2.getHighLevelImplElement());
  auto mux3i = boost::dynamic_pointer_cast<NDRegisterAccessor<int>>(mux3.getHighLevelImplElement());

  // check that all underlying raw accessors are still different
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux1i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux1i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux1i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux2i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);

  // check that the underlying raw accessors have the right address range
  NumericAddressedLowLevelTransferElement* llelem; // operator ->* does not work on a shared_ptr
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux0i->getHardwareAccessingElements()[0])
               .get();
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x20);
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux1i->getHardwareAccessingElements()[0])
               .get();
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x24);
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux2i->getHardwareAccessingElements()[0])
               .get();
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x28);
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux3i->getHardwareAccessingElements()[0])
               .get();
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x2C);
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);

  // add accessors to the transfer group. The accessors are intentionally added
  // out of order to check if the behaviour is also correct in that case
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux2);
  group.addAccessor(mux1);
  group.addAccessor(mux3);

  // check that all underlying raw accessors are now all the same
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux1i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux3i->getHardwareAccessingElements()[0]);

  // check that the underlying raw accessor have the right address range
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux0i->getHardwareAccessingElements()[0])
               .get();
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x20);
  BOOST_CHECK(llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 16);

  // check that reading and writing works
  mux0 = 42;
  mux1 = 120;
  mux2 = 84;
  mux3 = 240;
  group.write();

  mux0b.read();
  BOOST_CHECK(mux0b == 42);
  mux1b.read();
  BOOST_CHECK(mux1b == 120);
  mux2b.read();
  BOOST_CHECK(mux2b == 84);
  mux3b.read();
  BOOST_CHECK(mux3b == 240);

  mux0b = 123;
  mux0b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 120);
  BOOST_CHECK(mux2 == 84);
  BOOST_CHECK(mux3 == 240);

  mux1b = 234;
  mux1b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 84);
  BOOST_CHECK(mux3 == 240);

  mux2b = 345;
  mux2b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 345);
  BOOST_CHECK(mux3 == 240);

  mux3b = 456;
  mux3b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 345);
  BOOST_CHECK(mux3 == 456);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMergeNumericRegistersDifferentTypes) {
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses
  auto mux0 = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_0");
  auto mux1 = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_1");
  auto mux2 = device.getScalarRegisterAccessor<int32_t>("/ADC/WORD_CLK_MUX_2", 0, {AccessMode::raw});
  auto mux3 = device.getScalarRegisterAccessor<int64_t>("/ADC/WORD_CLK_MUX_3");

  // create the same register accessors again, so we have a second set not part
  // of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_0");
  auto mux1b = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_1");
  auto mux2b = device.getScalarRegisterAccessor<int32_t>("/ADC/WORD_CLK_MUX_2", 0, {AccessMode::raw});
  auto mux3b = device.getScalarRegisterAccessor<int64_t>("/ADC/WORD_CLK_MUX_3");

  // obtain the private pointers to the implementation of the accessor
  auto mux0i = boost::dynamic_pointer_cast<NDRegisterAccessor<uint16_t>>(mux0.getHighLevelImplElement());
  auto mux1i = boost::dynamic_pointer_cast<NDRegisterAccessor<uint16_t>>(mux1.getHighLevelImplElement());
  auto mux2i = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(mux2.getHighLevelImplElement());
  auto mux3i = boost::dynamic_pointer_cast<NDRegisterAccessor<int64_t>>(mux3.getHighLevelImplElement());

  // check that all underlying raw accessors are still different
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux1i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux1i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux1i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux2i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0]);

  // add accessors to the transfer group. The accessors are intentionally added
  // out of order to check if the behaviour is also correct in that case
  TransferGroup group;
  group.addAccessor(mux2);
  group.addAccessor(mux1);
  group.addAccessor(mux3);
  group.addAccessor(mux0);

  // check that all underlying raw accessors are now all the same
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux1i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux2i->getHardwareAccessingElements()[0]);
  BOOST_CHECK(mux0i->getHardwareAccessingElements()[0] == mux3i->getHardwareAccessingElements()[0]);

  // also check that all high-level implementations are still the same as
  // previously
  BOOST_CHECK(mux0i == boost::dynamic_pointer_cast<NDRegisterAccessor<uint16_t>>(mux0.getHighLevelImplElement()));
  BOOST_CHECK(mux1i == boost::dynamic_pointer_cast<NDRegisterAccessor<uint16_t>>(mux1.getHighLevelImplElement()));
  BOOST_CHECK(mux2i == boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(mux2.getHighLevelImplElement()));
  BOOST_CHECK(mux3i == boost::dynamic_pointer_cast<NDRegisterAccessor<int64_t>>(mux3.getHighLevelImplElement()));

  // check that reading and writing works
  mux0 = 42;
  mux1 = 120;
  mux2 = 84;
  mux3 = 240;
  group.write();

  mux0b.read();
  BOOST_CHECK(mux0b == 42);
  mux1b.read();
  BOOST_CHECK(mux1b == 120);
  mux2b.read();
  BOOST_CHECK(mux2b == 84);
  mux3b.read();
  BOOST_CHECK(mux3b == 240);

  mux0b = 123;
  mux0b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 120);
  BOOST_CHECK(mux2 == 84);
  BOOST_CHECK(mux3 == 240);

  mux1b = 234;
  mux1b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 84);
  BOOST_CHECK(mux3 == 240);

  mux2b = 345;
  mux2b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 345);
  BOOST_CHECK(mux3 == 240);

  mux3b = 456;
  mux3b.write();
  group.read();
  BOOST_CHECK(mux0 == 123);
  BOOST_CHECK(mux1 == 234);
  BOOST_CHECK(mux2 == 345);
  BOOST_CHECK(mux3 == 456);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestMergeStridedChannelSlices) {
  // Open a device on the multiplexed-data map so the named channel slices of the
  // 2D register TEST/NODMA are constructed as strided channel accessors.
  ChimeraTK::Device device;
  device.open("(ExceptionDummy:1?map=muxedDataAccessor.jmap)");

  // Create strided channel accessors for a non-trivial subset of channels.
  auto ch0 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.0");
  auto ch5 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.5");
  auto ch12 = device.getOneDRegisterAccessor<int16_t>("/TEST/NODMA.12");

  // A full 2D accessor, used exclusively to write the values expected to be read
  // back through the channel accessors. It is never added to the group.
  auto full = device.getTwoDRegisterAccessor<int16_t>("/TEST/NODMA");

  // Obtain the high-level impl elements to inspect the underlying low-level elements.
  auto ch0i = boost::dynamic_pointer_cast<NDRegisterAccessor<int16_t>>(ch0.getHighLevelImplElement());
  auto ch5i = boost::dynamic_pointer_cast<NDRegisterAccessor<int16_t>>(ch5.getHighLevelImplElement());
  auto ch12i = boost::dynamic_pointer_cast<NDRegisterAccessor<int16_t>>(ch12.getHighLevelImplElement());

  // Before adding to the group, all underlying low-level elements are distinct...
  BOOST_TEST((ch0i->getHardwareAccessingElements()[0] != ch5i->getHardwareAccessingElements()[0]));
  BOOST_TEST((ch0i->getHardwareAccessingElements()[0] != ch12i->getHardwareAccessingElements()[0]));
  BOOST_TEST((ch5i->getHardwareAccessingElements()[0] != ch12i->getHardwareAccessingElements()[0]));

  // ...and after adding them to the group (intentionally out of order) they all
  // become one shared low-level transfer element, because the interleaved channel
  // slices overlap in memory.
  TransferGroup group;
  group.addAccessor(ch12);
  group.addAccessor(ch0);
  group.addAccessor(ch5);

  BOOST_TEST((ch0i->getHardwareAccessingElements()[0] == ch5i->getHardwareAccessingElements()[0]));
  BOOST_TEST((ch0i->getHardwareAccessingElements()[0] == ch12i->getHardwareAccessingElements()[0]));
  BOOST_TEST((ch5i->getHardwareAccessingElements()[0] == ch12i->getHardwareAccessingElements()[0]));

  // Functional read-back verification: write per-channel values through the full 2D
  // accessor, then read via the group and confirm each channel accessor decodes its
  // own channel's strided data through the now-shared low-level element.
  for(size_t e = 0; e < 4; ++e) {
    full[0][e] = static_cast<int16_t>(100 + e);
    full[5][e] = static_cast<int16_t>(200 + e);
    full[12][e] = static_cast<int16_t>(300 + e);
  }
  full.write();

  group.read();
  for(size_t e = 0; e < 4; ++e) {
    BOOST_TEST(ch0[static_cast<int>(e)] == static_cast<int16_t>(100 + e));
    BOOST_TEST(ch5[static_cast<int>(e)] == static_cast<int16_t>(200 + e));
    BOOST_TEST(ch12[static_cast<int>(e)] == static_cast<int16_t>(300 + e));
  }

  // Overwrite the values once more through the 2D accessor and re-read through the
  // group, confirming the shared low-level element still decodes each channel
  // independently after the merge.
  for(size_t e = 0; e < 4; ++e) {
    full[0][e] = static_cast<int16_t>(400 - e);
    full[5][e] = static_cast<int16_t>(500 - e);
    full[12][e] = static_cast<int16_t>(600 - e);
  }
  full.write();

  group.read();
  for(size_t e = 0; e < 4; ++e) {
    BOOST_TEST(ch0[static_cast<int>(e)] == static_cast<int16_t>(400 - e));
    BOOST_TEST(ch5[static_cast<int>(e)] == static_cast<int16_t>(500 - e));
    BOOST_TEST(ch12[static_cast<int>(e)] == static_cast<int16_t>(600 - e));
  }
}

BOOST_AUTO_TEST_CASE(TestGatedByScalarPolled) {
  // A scalar register with a gatedBy reports valid data while its gating register holds the configured value and
  // faulty data while it does not. The payload through a closed gate is unspecified and not asserted.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  auto plain = device.getScalarRegisterAccessor<uint32_t>("DATA/PLAIN");

  // Open gate: CTRL.SEL == 1 is the configured value.
  sel = 1;
  sel.write();
  plain.read();
  BOOST_TEST(plain.dataValidity() == ChimeraTK::DataValidity::ok);

  // Closed gate: any other value of the gating register.
  sel = 0;
  sel.write();
  plain.read();
  BOOST_TEST(plain.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Reopening restores good data.
  sel = 1;
  sel.write();
  plain.read();
  BOOST_TEST(plain.dataValidity() == ChimeraTK::DataValidity::ok);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByAlternativesPolled) {
  // Two registers share the same address and differ only in their gatedBy value, so changing the gating register
  // returns the other alternative.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  auto plain = device.getScalarRegisterAccessor<uint32_t>("DATA/PLAIN");
  auto plainAlt = device.getScalarRegisterAccessor<uint32_t>("DATA/PLAIN_ALT");

  // CTRL.SEL == 1 opens PLAIN and closes PLAIN_ALT.
  sel = 1;
  sel.write();
  plain.read();
  plainAlt.read();
  BOOST_TEST(plain.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(plainAlt.dataValidity() == ChimeraTK::DataValidity::faulty);

  // CTRL.SEL == 2 swaps the roles.
  sel = 2;
  sel.write();
  plain.read();
  plainAlt.read();
  BOOST_TEST(plain.dataValidity() == ChimeraTK::DataValidity::faulty);
  BOOST_TEST(plainAlt.dataValidity() == ChimeraTK::DataValidity::ok);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByOneDArrayPolled) {
  // A 1D register carries the same whole-register gate as a scalar one.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  auto array = device.getOneDRegisterAccessor<uint32_t>("DATA/ARRAY");

  sel = 1;
  sel.write();
  array.read();
  BOOST_TEST(array.dataValidity() == ChimeraTK::DataValidity::ok);

  sel = 0;
  sel.write();
  array.read();
  BOOST_TEST(array.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByNonScalarGatingRegisterRejected) {
  // A gating register that is not scalar (multi-element or 2D) is rejected with a clear error when the gated
  // accessor is created.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  BOOST_CHECK_THROW(
      std::ignore = device.getScalarRegisterAccessor<uint32_t>("DATA/GATED_BY_ARR"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(
      std::ignore = device.getScalarRegisterAccessor<uint32_t>("DATA/GATED_BY_2D"), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByBitRangeGatingRegisterPolled) {
  // A bit-range gating register is accepted and gates correctly.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto brSel = device.getScalarRegisterAccessor<uint32_t>("CTRL/BR_SEL");
  auto gated = device.getScalarRegisterAccessor<uint32_t>("DATA/GATED_BY_BR");

  // CTRL.BR_SEL == 3 is the configured value.
  brSel = 3;
  brSel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);

  brSel = 1;
  brSel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByBitRangeAccessorPolled) {
  // A gated register whose own representation has a non-zero bitShift is a bit-range accessor itself. It reports
  // faulty data while its gate is closed and the data while it is open.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  auto gated = device.getScalarRegisterAccessor<uint32_t>("DATA/GATED_BIT_RANGE");

  // CTRL.SEL == 1 is the configured value.
  sel = 1;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);

  // Any other value closes the gate.
  sel = 0;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Reopening restores good data.
  sel = 1;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByBitRangeChildSlicePolled) {
  // A bit-field child slice of a gated channel carries the channel's gate. It reports faulty data while the gate is
  // closed and the data while it is open.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedBy2D.jmap)");

  auto chSel = device.getScalarRegisterAccessor<uint32_t>("CTRL/CH_SEL");
  auto flag = device.getScalarRegisterAccessor<uint32_t>("DAQ/GATED_STATUS/StatusCh0/Flag");

  // The channel is gated by CTRL.CH_SEL == 1.
  chSel = 1;
  chSel.write();
  flag.read();
  BOOST_TEST(flag.dataValidity() == ChimeraTK::DataValidity::ok);

  chSel = 0;
  chSel.write();
  flag.read();
  BOOST_TEST(flag.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByUnsignedTopBitPolled) {
  // An unsigned gate value with the top bit set gates correctly.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto top = device.getScalarRegisterAccessor<uint32_t>("CTRL/TOP");
  auto gated = device.getScalarRegisterAccessor<uint32_t>("DATA/GATED_BY_TOP");

  // The configured value is 2^31, i.e. only the top bit set.
  top = 0x80000000U;
  top.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);

  // The same register value with the top bit cleared closes the gate.
  top = 0x7fffffffU;
  top.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByAsciiPolled) {
  // A gated ASCII ("string") register is gated like a numeric one: a polled read reports faulty data while its gate
  // is closed and the data while it is open. The payload through a closed gate is unspecified and not asserted.
  ChimeraTK::Device device;
  device.open("(dummy?map=gatedByScalar.jmap)");

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  // A writable register at the payload address lets the test place the raw bytes the ASCII accessor reads.
  auto scratch = device.getOneDRegisterAccessor<uint32_t>("DATA/SCRATCH_STR");
  auto gated = device.getOneDRegisterAccessor<std::string>("DATA/STRING_GATED");

  // "AB" in the first character slot, "CD" in the second.
  scratch[0] = 0x4241U;
  scratch[1] = 0x4443U;
  scratch.write();

  // Open gate: CTRL.SEL == 1 is the configured value.
  sel = 1;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(gated[0] == "AB");
  BOOST_TEST(gated[1] == "CD");

  // Closed gate: any other value of the gating register.
  sel = 0;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Reopening restores good data.
  sel = 1;
  sel.write();
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(gated[0] == "AB");
}

/**********************************************************************************************************************/

// A backend that counts the reads of each hardware address, so a test can check that one gating register is read once
// per transfer however many consumers it gates.
class GatedReadCountBackend : public DummyBackend {
 public:
  explicit GatedReadCountBackend(const std::string& mapFileName) : DummyBackend(mapFileName) {}

  void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override {
    ++_readCount[address];
    DummyBackend::read(bar, address, data, sizeInBytes);
  }

  /** Number of reads of the given hardware address since construction. */
  size_t readsOf(uint64_t address) const {
    auto it = _readCount.find(address);
    return it == _readCount.end() ? 0 : it->second;
  }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::make_shared<GatedReadCountBackend>(parameters.at("map"));
  }

 private:
  std::map<uint64_t, size_t> _readCount;

  struct Registerer {
    Registerer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "GatedReadCountBackend", &GatedReadCountBackend::createInstance, {"map"});
    }
  };
  static Registerer _registerer;
};

GatedReadCountBackend::Registerer GatedReadCountBackend::_registerer;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestGatedByGatingRegisterReadOncePerTransfer) {
  // Two gated consumers share one gating register. The gate accessors merge, so the gating register is read exactly
  // once per transfer even though two gated consumers are served. This also holds after a transfer-group merge of the
  // payload accessors.
  ChimeraTK::Device device;
  device.open("(GatedReadCountBackend?map=gatedByScalar.jmap)");

  auto backend = boost::dynamic_pointer_cast<GatedReadCountBackend>(device.getBackend());
  BOOST_REQUIRE(backend != nullptr);

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  sel = 1;
  sel.write();

  // DATA/PLAIN and DATA/PLAIN_ALT share the gating register CTRL.SEL but sit at one address, so their payload
  // elements merge in the transfer group.
  auto plain = device.getScalarRegisterAccessor<uint32_t>("DATA/PLAIN");
  auto plainAlt = device.getScalarRegisterAccessor<uint32_t>("DATA/PLAIN_ALT");

  ChimeraTK::TransferGroup group;
  group.addAccessor(plain);
  group.addAccessor(plainAlt);

  const uint64_t selAddress = 0; // CTRL.SEL is the gating register at offset 0.
  auto before = backend->readsOf(selAddress);
  group.read();
  // One gating register read for the whole transfer, however many consumers it gates.
  BOOST_TEST(backend->readsOf(selAddress) == before + 1);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
