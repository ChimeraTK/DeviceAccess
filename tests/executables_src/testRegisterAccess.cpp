// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RegisterAccessSpecifierTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(RegisterAccessSpecifierTestSuite)

BOOST_AUTO_TEST_CASE(testRegisterAccess) {
  Device dev;
  dev.open("(pci:pcieunidummys6?map=registerAccess.map)");
  BOOST_CHECK(dev.isOpened());

  // Check RO
  {
    auto accessor = dev.getScalarRegisterAccessor<int>("BOARD.WORD_FIRMWARE");
    BOOST_CHECK(accessor.isReadOnly());
    BOOST_CHECK(!accessor.isWriteable());
    BOOST_CHECK(accessor.isReadable());
  }

  // Check RW
  {
    auto accessor = dev.getScalarRegisterAccessor<int>("ADC.WORD_CLK_DUMMY");
    BOOST_CHECK(!accessor.isReadOnly());
    BOOST_CHECK(accessor.isWriteable());
    BOOST_CHECK(accessor.isReadable());
  }

  // Check WO
  {
    auto accessor = dev.getScalarRegisterAccessor<int>("ADC.WORD_ADC_ENA");
    BOOST_CHECK(!accessor.isReadOnly());
    BOOST_CHECK(accessor.isWriteable());
    BOOST_CHECK(!accessor.isReadable());
  }

  // Check default: Access should be RW
  {
    auto accessor = dev.getScalarRegisterAccessor<int>("ADC.WORD_CLK_RST");
    BOOST_CHECK(!accessor.isReadOnly());
    BOOST_CHECK(accessor.isWriteable());
    BOOST_CHECK(accessor.isReadable());
  }
}

BOOST_AUTO_TEST_SUITE_END()
