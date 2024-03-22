// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE INTCHandlerTest
#include <boost/test/unit_test.hpp>
#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "TransferGroup.h"
using namespace ChimeraTK;
using namespace boost::unit_test_framework;

BOOST_AUTO_TEST_SUITE(INTCHandlerTestSuite)


/*
 create a map file with multiple primary interrupts to cover all the combinations in the tests.
 * use example for asyncread
 */

/********************************************************************************************************************/

//BOOST_AUTO_TEST_CASE(testBlockingRead) {
//}
/********************************************************************************************************************/


/* probably this is the first one to be implemented
 * if neither (SIE and CIE) nor MIR are present, or only IER is there, it writes 1<<N to IER and clears with 1<<N
 * if neither ICR nor IAR are present: INTC writes the according bit mask to ISR
*/
BOOST_AUTO_TEST_CASE(testIERwithISR) {

  ChimeraTK::Device device;
  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:4",0, {ChimeraTK::AccessMode::wait_for_new_data});

  device.activateAsyncRead();

  auto testIER = device.getScalarRegisterAccessor<int>("APP/IER");

 // BOOST_CHECK(0x8 == testIER.read());

  auto accICR = device.getScalarRegisterAccessor<int>("APP/ISR");

 // BOOST_CHECK((accICR >> 3) & 1) == true);

  device.close();
}
/**********************************************************************************************************************/
#ifdef _en
/*if IAR is present: INTC writes 1<<n the according bit mask to IAR and not to ISR*/

BOOST_AUTO_TEST_CASE(testIERwithIAR) {

  ChimeraTK::Device device;
  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!1:4",0, {ChimeraTK::AccessMode::wait_for_new_data});

  device.activateAsyncRead();

  //auto testIER = device.getScalarRegisterAccessor<int>("APP/IER");

  //BOOST_CHECK(0x8  == testIER.read()):

  auto accIAR = device.getScalarRegisterAccessor<int>("APP/IAR");

  //BOOST_CHECK((accIAR >> 3) & 1) == true);

  device.close();
}
/**********************************************************************************************************************/
/* if ICR is present: INTC writes 1<<n the according bit maskto ICR and not to ISR */

BOOST_AUTO_TEST_CASE(testIERwithICR) {

  ChimeraTK::Device device;
  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!2:4",0, {ChimeraTK::AccessMode::wait_for_new_data});

  device.activateAsyncRead();

  //auto testIER = device.getScalarRegisterAccessor<int>("APP/IER");

  //BOOST_CHECK(0x8 == testIER.read());

  auto accICR = device.getScalarRegisterAccessor<int>("APP/ICR");

  //BOOST_CHECK((accICR >> 3) & 1) == true);

  device.close();
}

BOOST_AUTO_TEST_CASE(testMER) {

  ChimeraTK::Device device;

  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:1",0, {ChimeraTK::AccessMode::wait_for_new_data});

  auto accMER = device.getScalarRegisterAccessor<int>("APP/MER");

  // MER should not be set
  BOOST_CHECK(0x0 == accMER.read());

  device.activateAsyncRead();

  // MER should be set now

  BOOST_CHECK(0x1 == accMER.read());

  device.close();

}

/********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testMIE) {
  ChimeraTK::Device device;

  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:1",0, {ChimeraTK::AccessMode::wait_for_new_data});

  auto accMIE = device.getScalarRegisterAccessor<int>("APP.MIE");

  // MIE should not be set
  BOOST_CHECK(0x0 == accMIE.read());

  device.activateAsyncRead();

  // MIE should be set now

  BOOST_CHECK(0x1 == accMIE.read());

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testGIE) {
  ChimeraTK::Device device;

  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:1",0, {ChimeraTK::AccessMode::wait_for_new_data});

  auto accGIE = device.getScalarRegisterAccessor<int>("APP/GIE");

  // MIE should not be set
  BOOST_CHECK(0 == accGIE.read());

  device.activateAsyncRead();

  // MER should be set now

  BOOST_CHECK(0x2 == accGIE.read());

  device.close();
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSIE) {
  ChimeraTK::Device device;

  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:2",0, {ChimeraTK::AccessMode::wait_for_new_data});

  device.activateAsyncRead();

  auto accSIE = device.getScalarRegisterAccessor<int>("APP/SIE");

  BOOST_CHECK(0x4 == accSIE.read());

  /*auto accCIE = device.getScalarRegisterAccessor<int>("APP/CIE");

  BOOST_CHECK(false = (accGIE >> 2) & 1))*/

  device.close();
}

/**********************************************************************************************************************/

#endif
/**********************************************************************************************************************/





BOOST_AUTO_TEST_SUITE_END()
