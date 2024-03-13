// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE INTCHandlerTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "TransferGroup.h"
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(INTCHandlerTestSuite)


/*
 create a map file with multiple primary interrupts to cover all the combinations in the tests.
 * use example for asyncread
 */ 

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testBlockingRead) {
}
/********************************************************************************************************************/
   
 




BOOST_AUTO_TEST_CASE(testMER) {
  
   ChimeraTK::Device device;
  
  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_CHECK(device.isOpened() == true);
  auto accInterrput = device.getScalarRegisterAccessor<int>("!0:N",0, {ChimeraTK::AccessMode::wait_for_new_data}); 
  
  
  auto accMER = device.getScalarRegisterAccessor<int>("APP.MER");
  
  // MER should not be set
  BOOST_CHECK(0 = accMER.read())
  
  device.activateAsyncRead();

  // MER should be set now
  
  BOOST_CHECK(1 = accMER.read())

  
  device.close();
  
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMIE) {
  
}


/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
