// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapForceReadOnlyPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"

using namespace ChimeraTK;

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(test) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=forceReadOnlyPlugin.xlmap)");

  auto cat = device.getRegisterCatalogue();
  auto info = cat.getRegister("test");
  BOOST_CHECK(!info.isWriteable());
  BOOST_CHECK(info.isReadable());

  auto acc = device.getScalarRegisterAccessor<double>("test");
  BOOST_CHECK(!acc.isWriteable());
  BOOST_CHECK(acc.isReadable());

  BOOST_CHECK_THROW(acc.write(), ChimeraTK::logic_error);
  BOOST_CHECK_NO_THROW(acc.read());
}

/********************************************************************************************************************/
