#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BufferingRegisterAccessor.h"
#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapMathPluginTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSimpleReadWriteScalar) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto accMath = device.getScalarRegisterAccessor<double>("SimpleScalarReadWrite");

  accTarget = 45;
  accTarget.write();
  accMath.read();
  BOOST_CHECK_CLOSE(double(accMath), 45. / 7. + 13., 0.00001);

  accTarget = -666;
  accTarget.write();
  accMath.read();
  BOOST_CHECK_CLOSE(double(accMath), -666. / 7. + 13., 0.00001);

  accMath = 77.;
  accMath.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 24); // 77/7 + 13

  accMath = -140.;
  accMath.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), -7); // -140/7 + 13
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReturnedReadWriteScalar) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  // ReturnedScalarReadWrite uses a return statement, but is otherwise identical to the "simple" version
  auto accMath = device.getScalarRegisterAccessor<double>("ReturnedScalarReadWrite");

  accTarget = 45;
  accTarget.write();
  accMath.read();
  BOOST_CHECK_CLOSE(double(accMath), 45. / 7. + 13., 0.00001);

  accTarget = -666;
  accTarget.write();
  accMath.read();
  BOOST_CHECK_CLOSE(double(accMath), -666. / 7. + 13., 0.00001);

  accMath = 77.;
  accMath.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 24); // 77/7 + 13

  accMath = -140.;
  accMath.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), -7); // -140/7 + 13
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteArray) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getOneDRegisterAccessor<int>("SimpleArray");
  auto accMath = device.getOneDRegisterAccessor<double>("ArrayReadWrite");
  BOOST_CHECK_EQUAL(accMath.getNElements(), 6);

  accTarget = {11, 22, 33, 44, 55, 66};
  accTarget.write();
  accMath.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_CLOSE(double(accMath[i]), accTarget[i] / 7. + 13., 0.00001);

  accTarget = {-120, 123456, -18, 9999, -999999999, 0};
  accTarget.write();
  accMath.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_CLOSE(double(accMath[i]), accTarget[i] / 7. + 13., 0.00001);

  accMath = {-120, 123456, -18, 9999, -999999999, 0};
  accMath.write();
  accTarget.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_EQUAL(double(accTarget[i]), std::round(accMath[i] / 7. + 13.));

  accMath = {0, 1, 2, 3, 4, 5};
  accMath.write();
  accTarget.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_EQUAL(double(accTarget[i]), std::round(accMath[i] / 7. + 13.));
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptions) {}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
