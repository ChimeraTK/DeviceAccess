#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AccessModeFlagsTest

#include "AccessMode.h"

#include <boost/test/unit_test.hpp>
#include <iostream>

namespace ctk = ChimeraTK;

BOOST_AUTO_TEST_SUITE(AccessModeFlagsTestSuite)

BOOST_AUTO_TEST_CASE(testSerialize) {
  auto flags = ctk::AccessModeFlags{ctk::AccessMode::wait_for_new_data,
                                    ctk::AccessMode::raw};
  BOOST_CHECK(flags.serialize() == "raw,wait_for_new_data");

  flags = {};
  BOOST_CHECK(flags.serialize() == "");
}

BOOST_AUTO_TEST_CASE(testDeSerialize) {
  auto flags = ctk::AccessModeFlags::deserialize("wait_for_new_data,raw");
  BOOST_CHECK(flags.has(ctk::AccessMode::raw) == true);
  BOOST_CHECK(flags.has(ctk::AccessMode::wait_for_new_data) == true);

  BOOST_CHECK_THROW(ctk::AccessModeFlags::deserialize("bogus_flag"), ctk::logic_error );
}

BOOST_AUTO_TEST_SUITE_END()
