#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LargeBarNumberTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

/* This test is checking that 64-bit BARs are working as expected
 */

#include "Device.h"
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE(testLargeBarNumber) {
  Device d;
  d.open("(dummy?map=goodMapFile.map)");

  // this is a smoke test that reading and writing works with a large BAR map file
  auto intAccessor = d.getScalarRegisterAccessor<int32_t>("LARGE_BAR/NUMBER");
  intAccessor.read();

  intAccessor = 42;
  intAccessor.write();
}
