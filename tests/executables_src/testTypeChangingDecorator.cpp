// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/unit_test.hpp>

#include <limits>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
using namespace ChimeraTK;

#include "Device.h"
#include "ScalarRegisterAccessor.h"
#include "TransferGroup.h"

#include <boost/shared_ptr.hpp>

/**********************************************************************************************************************/

// helper function to convert numeric or string to double
template<class UserType>
double toDouble(UserType input) {
  return userTypeToNumeric<double>(input);
}

template<typename T>
bool test_equal_or_close(T a, T b) {
  if(a == b) {
    return true;
  }
  else {
    std::cout << "checking for equality failed: " << a << " != " << b << std::endl;
    return false;
  }
}

template<>
bool test_equal_or_close<double>(double a, double b) {
  if(std::fabs(a - b) < 0.0001) {
    return true;
  }
  else {
    std::cout << "checking for being close failed: " << a << " - " << b << " >= 0.0001" << std::endl;
    return false;
  }
}

template<>
bool test_equal_or_close<float>(float a, float b) {
  if(std::fabs(a - b) < 0.0001) {
    return true;
  }
  else {
    std::cout << "checking for being close failed: " << a << " - " << b << " >= 0.0001" << std::endl;
    return false;
  }
}

template<typename T>
bool test_not_close(T a, T b, double tolerance = 0.0001) {
  if(std::fabs(a - b) > tolerance) {
    return true;
  }
  else {
    std::cout << a << " - " << b << " is not > " << tolerance << std::endl;
    return false;
  }
}

template<>
bool test_not_close<std::string>(std::string a, std::string b, double /*tolerance*/) {
  return (a != b);
}

template<typename T, typename IMPL_T>
struct Adder {
  static T add(T startVal, int increment) { return startVal + increment; }
};

template<typename IMPL_T>
struct Adder<std::string, IMPL_T> {
  static std::string add(std::string startVal, int increment) {
    std::stringstream s_in, s_out;
    s_in << startVal;
    IMPL_T startValImplT;
    s_in >> startValImplT;

    s_out << startValImplT + increment;

    return s_out.str();
  }
};

// we need a special implementation for int8, which otherwise is treated as a character/letter
template<>
struct Adder<std::string, int8_t> {
  static std::string add(std::string startVal, int increment) {
    std::stringstream s_in, s_out;
    s_in << startVal;
    int32_t startValImplT;
    s_in >> startValImplT;

    s_out << startValImplT + increment;

    return s_out.str();
  }
};

// we need a special implementation for uint8, which otherwise is treated as a character/letter
template<>
struct Adder<std::string, uint8_t> {
  static std::string add(std::string startVal, int increment) {
    std::stringstream s_in, s_out;
    s_in << startVal;
    uint32_t startValImplT;
    s_in >> startValImplT;

    s_out << startValImplT + increment;

    return s_out.str();
  }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(TypeChangingDecoratorTestSuite)

// the startReadValue and expectedWriteValue are from the register in the dummy
// device, which is 32 bit fixed point singed with 16 fractional bits, thus we
// talk to it as double from the test
template<class T, class IMPL_T, template<typename, typename> class DECORATOR_TYPE = TypeChangingRangeCheckingDecorator>
void testDecorator(double startReadValue, T expectedReadValue, T startWriteValue, double expectedWriteValue) {
  ChimeraTK::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");
  auto anotherScalarAccessor = d.getScalarRegisterAccessor<double>("/SOME/SCALAR");
  auto anotherImplTAccessor = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");

  auto ndAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor<IMPL_T>>(scalar.getHighLevelImplElement());
  DECORATOR_TYPE<T, IMPL_T> decoratedScalar(ndAccessor);

  BOOST_REQUIRE(decoratedScalar.getNumberOfChannels() == 1);
  BOOST_REQUIRE(decoratedScalar.getNumberOfSamples() == 1);

  BOOST_CHECK(decoratedScalar.getName() == "/SOME/SCALAR");

  BOOST_CHECK(decoratedScalar.isReadable());
  BOOST_CHECK(decoratedScalar.isWriteable());
  BOOST_CHECK(!decoratedScalar.isReadOnly());

  anotherScalarAccessor = startReadValue;
  anotherScalarAccessor.write();
  // check that the values are different at start so we know the test is
  // sensitive
  BOOST_CHECK(test_not_close(decoratedScalar.accessData(0), expectedReadValue));
  decoratedScalar.read();
  // internal precision of the register is 16 fractional bits fixed point
  BOOST_CHECK(test_equal_or_close<T>(decoratedScalar.accessData(0), expectedReadValue));

  decoratedScalar.accessData(0) = startWriteValue;
  decoratedScalar.write();
  anotherScalarAccessor.read();
  BOOST_CHECK_CLOSE(toDouble(anotherScalarAccessor), expectedWriteValue, 0.0001);

  // repeat the read / write tests with all different functions

  // test asynchronous reading. Check that the modification is arriving in the
  // decorator's buffer

  // just to check that the test is not producing false positives by accident
  assert(fabs(startReadValue + 2 - (expectedWriteValue + 1)) > 0.001);
  anotherScalarAccessor = startReadValue + 2;
  anotherScalarAccessor.write();

  // FIXME: We cannot test that the decorator is relaying doReadTransfer,
  // doReadTransferLatest and do readTransferLatest correctly with the dummy
  // backend because they all point to the same implementation. Thus we
  // intentionally do not call them to indicate them uncovered. We would have to
  // use the control system adapter implementations with the queues to test it.

  // Check that the result for mayReplaceOther is consistent
  BOOST_CHECK(!decoratedScalar.mayReplaceOther(anotherImplTAccessor.getHighLevelImplElement()));
  auto anotherNdAccessor =
      boost::dynamic_pointer_cast<NDRegisterAccessor<IMPL_T>>(anotherImplTAccessor.getHighLevelImplElement());
  BOOST_CHECK(anotherNdAccessor->mayReplaceOther(ndAccessor)); // unrelated check just to make sure the test is doing
                                                               // the right thing...
  auto anotherDecoratedScalar = boost::make_shared<DECORATOR_TYPE<T, IMPL_T>>(anotherNdAccessor);
  BOOST_CHECK(decoratedScalar.mayReplaceOther(anotherDecoratedScalar));

  // OK, I give up. I would have to repeat all tests ever written for a
  // decorator, incl. transfer group(added below), persistentDataStorage and everything. All
  // I can do is leave the stuff intentionally uncovered so a reviewer can find
  // the places.

  // Test with transfer group
  assert(fabs(startReadValue + 3 - (expectedWriteValue + 1)) > 0.001);
  anotherScalarAccessor = startReadValue + 3;
  anotherScalarAccessor.write();

  TransferGroup transferGroup;
  auto decoratedScalarInGroup = boost::make_shared<DECORATOR_TYPE<T, IMPL_T>>(ndAccessor);
  transferGroup.addAccessor(decoratedScalarInGroup);

  transferGroup.read();
  BOOST_CHECK(
      test_equal_or_close<T>(decoratedScalarInGroup->accessData(0), Adder<T, IMPL_T>::add(expectedReadValue, 3)));

  decoratedScalarInGroup->accessData(0) = Adder<T, IMPL_T>::add(startWriteValue, 1);
  transferGroup.write();
  anotherScalarAccessor.read();
  BOOST_CHECK_CLOSE(toDouble(anotherScalarAccessor), expectedWriteValue + 1, 0.0001);

  // Test pre/postRead
  anotherScalarAccessor = startReadValue + 4;
  anotherScalarAccessor.write();

  decoratedScalar.preRead(ChimeraTK::TransferType::read);

  // still nothing has changed on the user buffer
  BOOST_CHECK(test_equal_or_close<T>(decoratedScalar.accessData(0), startWriteValue));

  decoratedScalar.readTransfer();
  // Pass hasNewData as false, user buffer should still not have changed
  decoratedScalar.postRead(ChimeraTK::TransferType::read, false);
  BOOST_CHECK(test_equal_or_close<T>(decoratedScalar.accessData(0), startWriteValue));

  decoratedScalar.preRead(ChimeraTK::TransferType::read);
  decoratedScalar.readTransfer();
  // This time we except an update of the buffer
  decoratedScalar.postRead(ChimeraTK::TransferType::read, true);
  BOOST_CHECK(test_equal_or_close<T>(decoratedScalar.accessData(0), Adder<T, IMPL_T>::add(expectedReadValue, 4)));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAllDecoratorConversions) {
  testDecorator<int, int8_t>(12, 12, 22, 22);
  testDecorator<int, uint8_t>(13, 13, 23, 23);
  testDecorator<int, int16_t>(14, 14, 24, 24);
  testDecorator<int, uint16_t>(15, 15, 25, 25);
  testDecorator<int, int32_t>(16, 16, 26, 26);
  testDecorator<int, uint32_t>(17, 17, 27, 27);
  testDecorator<int, int64_t>(36, 36, -46, -46);
  testDecorator<int, uint64_t>(37, 37, 47, 47);
  testDecorator<int, float>(18.5, 19, 28, 28);
  testDecorator<int, float>(18.4, 18, 28, 28);
  testDecorator<int, double>(19.5, 20, 29, 29);
  testDecorator<int, double>(19.4, 19, 29, 29);
  testDecorator<int, std::string>(101, 101, 112, 112);

  testDecorator<float, int8_t>(112, 112, -122.4, -122);
  testDecorator<float, int8_t>(112, 112, -122.5, -123);
  testDecorator<float, uint8_t>(113, 113, 123.4, 123);
  testDecorator<float, uint8_t>(113, 113, 123.5, 124);
  testDecorator<float, int16_t>(114, 114, -124.4, -124);
  testDecorator<float, int16_t>(114, 114, -124.5, -125);
  testDecorator<float, uint16_t>(115, 115, 125.4, 125);
  testDecorator<float, uint16_t>(115, 115, 125.5, 126);
  testDecorator<float, int32_t>(116, 116, -126.4, -126);
  testDecorator<float, int32_t>(116, 116, -126.5, -127);
  testDecorator<float, uint32_t>(117, 117, 127.4, 127);
  testDecorator<float, uint32_t>(117, 117, 127.5, 128);
  testDecorator<float, int64_t>(136, 136, -146.4, -146);
  testDecorator<float, int64_t>(136, 136, -146.5, -147);
  testDecorator<float, uint64_t>(137, 137, 147.4, 147);
  testDecorator<float, uint64_t>(137, 137, 147.5, 148);
  testDecorator<float, float>(118.5, 118.5, 128.6, 128.6);
  testDecorator<float, double>(119.5, 119.5, 129.6, 129.6);
  testDecorator<float, std::string>(101.1, 101.1, 112.2, 112.2);
  testDecorator<double, std::string>(201.1, 201.1, 212.2, 212.2);

  testDecorator<std::string, int8_t>(112, "112", "-122.4", -122);
  testDecorator<std::string, int8_t>(112, "112", "-122.5", -122); // no proper rounding for strings
  testDecorator<std::string, uint8_t>(113, "113", "123.4", 123);
  testDecorator<std::string, uint8_t>(113, "113", "123.5", 123); // no proper rounding for strings
  testDecorator<std::string, int16_t>(114, "114", "-124.4", -124);
  testDecorator<std::string, int16_t>(114, "114", "-124.5", -124); // no proper rounding for strings
  testDecorator<std::string, uint16_t>(115, "115", "125.4", 125);
  testDecorator<std::string, uint16_t>(115, "115", "125.5", 125); // no proper rounding for strings
  testDecorator<std::string, int32_t>(116, "116", "-126.4", -126);
  testDecorator<std::string, int32_t>(116, "116", "-126.5", -126); // no proper rounding for strings
  testDecorator<std::string, uint32_t>(117, "117", "127.4", 127);
  testDecorator<std::string, uint32_t>(117, "117", "127.5", 127); // no proper rounding for strings
  testDecorator<std::string, int64_t>(136, "136", "-146.4", -146);
  testDecorator<std::string, int64_t>(136, "136", "-146.5", -146); // no proper rounding for strings
  testDecorator<std::string, uint64_t>(137, "137", "147.4", 147);
  testDecorator<std::string, uint64_t>(137, "137", "147.5", 147); // no proper rounding for strings
  testDecorator<std::string, float>(118.5, "118.5", "128.6", 128.6);
  testDecorator<std::string, double>(119.5, "119.5", "129.6", 129.6);

  testDecorator<int, std::string, TypeChangingDirectCastDecorator>(201, 201, 212, 212);
  testDecorator<float, std::string, TypeChangingDirectCastDecorator>(202, 202, 213, 213);
  testDecorator<double, std::string, TypeChangingDirectCastDecorator>(203, 203, 214, 214);
  testDecorator<int, float, TypeChangingDirectCastDecorator>(218.5, 218, 228, 228);
  testDecorator<float, int, TypeChangingDirectCastDecorator>(228, 228, 239.6, 239);
}

/**********************************************************************************************************************/

template<template<typename, typename> class DECORATOR_TYPE>
void loopTest() {
  // Test loops for numeric data types. One type is enough because it's template
  // code We don't have to test the string specialisations, arrays of strings
  // are not allowed
  ChimeraTK::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto twoD = d.getTwoDRegisterAccessor<double>("/SOME/TWO_D");
  auto anotherAccessor = d.getTwoDRegisterAccessor<double>("/SOME/TWO_D");

  anotherAccessor[0][0] = 100.0;
  anotherAccessor[0][1] = 101.1;
  anotherAccessor[1][0] = 110.0;
  anotherAccessor[1][1] = 111.6;
  anotherAccessor[2][0] = 120.0;
  anotherAccessor[2][1] = 121.6;
  anotherAccessor.write();

  // device under test (dut)
  auto impl = boost::dynamic_pointer_cast<NDRegisterAccessor<double>>(twoD.getHighLevelImplElement());
  DECORATOR_TYPE<int, double> dut(impl);

  BOOST_REQUIRE(dut.getNumberOfChannels() == 3);
  BOOST_REQUIRE(dut.getNumberOfSamples() == 2);

  dut.read();

  BOOST_CHECK_EQUAL(dut.accessData(0, 0), 100);
  BOOST_CHECK_EQUAL(dut.accessData(0, 1), 101);
  BOOST_CHECK_EQUAL(dut.accessData(1, 0), 110);
  BOOST_CHECK_EQUAL(dut.accessData(1, 1), 112);
  BOOST_CHECK_EQUAL(dut.accessData(2, 0), 120);
  BOOST_CHECK_EQUAL(dut.accessData(2, 1), 122);

  dut.accessData(0, 0) = 200;
  dut.accessData(0, 1) = 201;
  dut.accessData(1, 0) = 210;
  dut.accessData(1, 1) = 212;
  dut.accessData(2, 0) = 220;
  dut.accessData(2, 1) = 222;
  dut.write();

  anotherAccessor.read();

  BOOST_CHECK_CLOSE(anotherAccessor[0][0], 200., 0.0001);
  BOOST_CHECK_CLOSE(anotherAccessor[0][1], 201., 0.0001);
  BOOST_CHECK_CLOSE(anotherAccessor[1][0], 210., 0.0001);
  BOOST_CHECK_CLOSE(anotherAccessor[1][1], 212., 0.0001);
  BOOST_CHECK_CLOSE(anotherAccessor[2][0], 220., 0.0001);
  BOOST_CHECK_CLOSE(anotherAccessor[2][1], 222., 0.0001);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testLoops) {
  loopTest<TypeChangingRangeCheckingDecorator>();
  loopTest<TypeChangingDirectCastDecorator>();
}

/**********************************************************************************************************************/

#define CHECK_THROW_PRINT(command, exception_type)                                                                     \
  try {                                                                                                                \
    command;                                                                                                           \
    BOOST_ERROR(std::string(#command) + " did not throw as excepted.");                                                \
  }                                                                                                                    \
  catch(exception_type & e) {                                                                                          \
    std::cout << "** For manually checking the exeption message of " << #command << ":\n"                              \
              << "   " << e.what() << std::endl;                                                                       \
  }

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeChecks) {
  // Just a few tests where the type changing decorator with conversion should
  // throw where the direct cast decorator changes the interpretation
  ChimeraTK::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto myInt = d.getScalarRegisterAccessor<int32_t>("/SOME/INT");
  auto myIntDummy = d.getScalarRegisterAccessor<int32_t>("/SOME/INT"); // the second accessor for the test
  auto myUInt = d.getScalarRegisterAccessor<uint32_t>("/SOME/UINT");
  auto myUIntDummy = d.getScalarRegisterAccessor<uint32_t>("/SOME/UINT");

  auto intNDAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(myInt.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<uint32_t, int32_t> u2i(intNDAccessor);
  TypeChangingDirectCastDecorator<uint32_t, int32_t> directU2i(intNDAccessor);

  auto uintNDAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor<uint32_t>>(myUInt.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<int32_t, uint32_t> i2u(uintNDAccessor);
  TypeChangingDirectCastDecorator<int32_t, uint32_t> directI2u(uintNDAccessor);

  myIntDummy = -1;
  myIntDummy.write();
  BOOST_CHECK_NO_THROW(u2i.read());
  BOOST_CHECK_EQUAL(u2i.accessData(0), 0);
  BOOST_CHECK_NO_THROW(directU2i.read());
  BOOST_CHECK(directU2i.accessData(0) == 0xFFFFFFFF);

  myUIntDummy = std::numeric_limits<uint32_t>::max();
  myUIntDummy.write();
  BOOST_CHECK_NO_THROW(i2u.read());
  BOOST_CHECK_EQUAL(i2u.accessData(0), std::numeric_limits<int32_t>::max());
  BOOST_CHECK_NO_THROW(directI2u.read());
  BOOST_CHECK(directI2u.accessData(0) == -1);

  u2i.accessData(0) = static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U;
  BOOST_CHECK_NO_THROW(u2i.write());
  myIntDummy.read();
  BOOST_CHECK_EQUAL(myIntDummy, std::numeric_limits<int32_t>::max());

  i2u.accessData(0) = -1;
  BOOST_CHECK_NO_THROW(i2u.write());
  myUIntDummy.read();
  BOOST_CHECK_EQUAL(myUIntDummy, 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testTransferGroup) {
  ChimeraTK::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto partial0 = d.getScalarRegisterAccessor<double>("/SOME/ARRAY");
  auto partial1 = d.getScalarRegisterAccessor<double>("/SOME/ARRAY", 1);

  auto wholeArray = d.getOneDRegisterAccessor<double>("/SOME/ARRAY");
  wholeArray[0] = 12345;
  wholeArray[1] = 12346;
  wholeArray.write();

  ChimeraTK::ScalarRegisterAccessor<int> decorated0(getTypeChangingDecorator<int>(partial0));
  ChimeraTK::ScalarRegisterAccessor<int> decorated1(getTypeChangingDecorator<int>(partial1));

  TransferGroup group;
  group.addAccessor(decorated0);
  group.addAccessor(decorated1);

  group.read();
  BOOST_CHECK(decorated0 == 12345);
  BOOST_CHECK(decorated1 == 12346);

  decorated0 = 4321;
  decorated1 = 4322;
  group.write();

  wholeArray.read();
  BOOST_CHECK_CLOSE(wholeArray[0], 4321., 0.0001);
  BOOST_CHECK_CLOSE(wholeArray[1], 4322., 0.0001);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFactory) {
  ChimeraTK::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<double>("/SOME/SCALAR");
  ChimeraTK::TransferElementAbstractor& transferElement = scalar;

  auto decoratedScalar = getTypeChangingDecorator<int>(transferElement);
  BOOST_CHECK(decoratedScalar);
  // factory must detect the type of scalar and create the correct decorator
  auto castedScalar = boost::dynamic_pointer_cast<TypeChangingRangeCheckingDecorator<int, double>>(decoratedScalar);
  BOOST_CHECK(castedScalar);

  // if there already is a decorator, creating another one with a different type is still possible.
  auto cstyleScalar = getTypeChangingDecorator<int>(transferElement, DecoratorType::C_style_conversion);
  BOOST_CHECK(cstyleScalar.get() != decoratedScalar.get());
  auto shortScalar = getTypeChangingDecorator<short>(transferElement);
  // you can also get the same decorator again if you ask for it
  auto sameDecorator = getTypeChangingDecorator<int>(transferElement);
  BOOST_CHECK(sameDecorator.get() == decoratedScalar.get());
  auto sameCstyleDecorator = getTypeChangingDecorator<int>(transferElement, DecoratorType::C_style_conversion);
  BOOST_CHECK(sameCstyleDecorator.get() == cstyleScalar.get());
  auto sameShortDecorator = getTypeChangingDecorator<short>(transferElement);
  BOOST_CHECK(sameShortDecorator.get() == shortScalar.get());

  // Test for direct convertion decorator type
  // we have to use a different transfer element for this to work
  auto scalar2 = d.getScalarRegisterAccessor<double>("/SOME/SCALAR");
  auto decoratedDirectConvertingScalar = getTypeChangingDecorator<int>(scalar2, DecoratorType::C_style_conversion);
  BOOST_CHECK(decoratedDirectConvertingScalar);
  auto castedDCScalar =
      boost::dynamic_pointer_cast<TypeChangingDirectCastDecorator<int, double>>(decoratedDirectConvertingScalar);
  BOOST_CHECK(castedDCScalar);
}

BOOST_AUTO_TEST_SUITE_END()
