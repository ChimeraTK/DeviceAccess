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

BOOST_AUTO_TEST_CASE(test2) {
  // this xlmap was causing a logic_error although it should not.
  // See ticket https://redmine.msktools.desy.de/issues/9551
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=forceReadOnlyPlugin2.xlmap)");

  auto cat = device.getRegisterCatalogue();
  /*
   * I have following suspicion here:
   * - forceReadonly plugin sets up proper catalogue with correct entries only in getRegisterCatalogue
   *     by call to plugin->updateRegisterInfo(_catalogue_mutable);
   * - however, math plugin uses catalogue already before.
   *    where? in MathPlugin::updateRegisterInfo() called from MathPlugin::openhook()
   *    but: openhook does call backend->getRegisterCatalogue!!
   *    so ForceReadOnlyPlugin::updateRegisterInfo is actually called before MathPlugin::updateRegisterInfo,
   *    only the wrong information (meaning A would be writeable ) has before (both calls of updateregisterInfo)
   *    already propagated to the _info on the register B
   *    This copying happens in LogicalNameMappingBackend::getRegisterCatalogue()
   *    This shows that late-correcting of catalogue entries is really bad.
   * - math plugin sets up proper catalogue already in openHook
   * - doing the same for forceReadOnly plugin would solve the problem?
   *   I guess so.
   */
  auto infoA = cat.getRegister("Test/A");
  BOOST_CHECK(!infoA.isWriteable());
  BOOST_CHECK(infoA.isReadable());

  auto accA = device.getScalarRegisterAccessor<double>("Test/A");
  BOOST_CHECK(!accA.isWriteable());
  BOOST_CHECK(accA.isReadable());

  auto infoB = cat.getRegister("Test/B");
  BOOST_CHECK(!infoB.isWriteable());
  BOOST_CHECK(infoB.isReadable());

  auto accB = device.getScalarRegisterAccessor<double>("Test/B");
  BOOST_CHECK(!accB.isWriteable());
  BOOST_CHECK(accB.isReadable());
  BOOST_CHECK_NO_THROW(accB.read());
}

/********************************************************************************************************************/
