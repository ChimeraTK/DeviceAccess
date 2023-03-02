// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "ExceptionDummyBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMathPluginDataValidityTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadSync) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto accMathRead = device.getScalarRegisterAccessor<double>("SimpleScalarRead");

  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);

  accTarget.setDataValidity(ChimeraTK::DataValidity::faulty);
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::faulty);

  accTarget.setDataValidity(ChimeraTK::DataValidity::ok);
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWrite) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("SimpleScalarWrite");

  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);

  accMathWrite.setDataValidity(ChimeraTK::DataValidity::faulty);
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);

  accMathWrite.setDataValidity(ChimeraTK::DataValidity::ok);
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadSyncWithParameters) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto scalarPar = device.getScalarRegisterAccessor<int>("ScalarParameter");
  auto accMathRead = device.getScalarRegisterAccessor<double>("ScalarWithParametersRead");
  auto arrayPar = device.getOneDRegisterAccessor<int>("SimpleArray");

  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
  scalarPar.read();
  BOOST_CHECK(scalarPar.dataValidity() == ChimeraTK::DataValidity::ok);
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  // set a parameter to faulty.
  scalarPar.setDataValidity(ChimeraTK::DataValidity::faulty);
  scalarPar.write();

  // should become faulty
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::faulty);

  // It's readonly so no change is expected in target.
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);

  // other parameters should be ok.
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  // set a parameter to ok.
  scalarPar.setDataValidity(ChimeraTK::DataValidity::ok);
  scalarPar.write();

  // should be ok now.
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);

  // set target to faulty.
  accTarget.setDataValidity(ChimeraTK::DataValidity::faulty);
  accTarget.write();

  // parameter should be ok.
  scalarPar.read();
  scalarPar.setDataValidity(ChimeraTK::DataValidity::ok);
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  // It should become faulty
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::faulty);

  // set target to ok.
  accTarget.setDataValidity(ChimeraTK::DataValidity::ok);
  accTarget.write();

  // All should be ok now.
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteWithParameters) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto scalarPar = device.getScalarRegisterAccessor<int>("ScalarParameter");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("ScalarWithParametersWrite");
  auto arrayPar = device.getOneDRegisterAccessor<int>("SimpleArray");

  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
  scalarPar.read();
  BOOST_CHECK(scalarPar.dataValidity() == ChimeraTK::DataValidity::ok);
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  accMathWrite.setDataValidity(ChimeraTK::DataValidity::faulty);
  accMathWrite.write();

  // target should become faulty.
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);

  // parameters should be ok.
  scalarPar.read();
  BOOST_CHECK(scalarPar.dataValidity() == ChimeraTK::DataValidity::ok);
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  // set it back to ok
  accMathWrite.setDataValidity(ChimeraTK::DataValidity::ok);
  accMathWrite.write();

  // should be ok.
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);

  // set parameter to faulty
  scalarPar.setDataValidity(ChimeraTK::DataValidity::faulty);
  scalarPar.write();

  // other parameter should be ok
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);

  // update
  accMathWrite.write();

  // target should become faulty.
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);

  // set parameter to ok
  scalarPar.setDataValidity(ChimeraTK::DataValidity::ok);
  scalarPar.write();

  // update
  accMathWrite.write();

  // target should be ok now.
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
}

BOOST_AUTO_TEST_SUITE_END()
