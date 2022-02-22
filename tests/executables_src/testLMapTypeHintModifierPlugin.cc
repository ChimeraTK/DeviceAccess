#  define BOOST_TEST_DYN_LINK
#  define BOOST_TEST_MODULE LMapTypeHintModifierPluginTest
#  include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#  include "BackendFactory.h"
#  include "DeviceAccessVersion.h"
#  include "DummyBackend.h"
#  include "BufferingRegisterAccessor.h"
#  include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapForceReadOnlyPluginTestSuite)

/********************************************************************************************************************/
#if 0

BOOST_AUTO_TEST_CASE(test) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=typeHintModifierPlugin.xlmap)");

  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("test");
  auto descriptor = info->getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 11);

  // Basically check if integer aliases to int32
  info = cat.getRegister("test2");
  descriptor = info->getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 11);

  info = cat.getRegister("test3");
  descriptor = info->getDataDescriptor();
  BOOST_CHECK(descriptor.isIntegral());
  BOOST_CHECK(not descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 20);

  info = cat.getRegister("test4");
  descriptor = info->getDataDescriptor();
  BOOST_CHECK(not descriptor.isIntegral());
  BOOST_CHECK(descriptor.isSigned());
  BOOST_CHECK_EQUAL(descriptor.nFractionalDigits(), 325);
  BOOST_CHECK_EQUAL(descriptor.nDigits(), 328);
}

/********************************************************************************************************************/

#endif

BOOST_AUTO_TEST_SUITE_END()
