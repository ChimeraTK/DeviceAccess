#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BufferingRegisterAccessor.h"
#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapMathPluginTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadScalar) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  ChimeraTK::Device target;
  target.open("(dummy?map=mtcadummy.map)");

  auto accTarget = target.getScalarRegisterAccessor<int>("BOARD.WORD_USER");
  auto accMath = device.getScalarRegisterAccessor<double>("SimpleScalarRead");

  accTarget = 42;
  accTarget.write();
  accMath.read();
  BOOST_CHECK_CLOSE(double(accMath), 42. / 7. + 13., 0.00001);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptions) {}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
