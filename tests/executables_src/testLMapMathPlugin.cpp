// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMathPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "LogicalNameMappingBackend.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapMathPluginTestSuite)

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testSimpleReadWriteScalar) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto accMathRead = device.getScalarRegisterAccessor<double>("SimpleScalarRead");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("SimpleScalarWrite");

  accTarget = 45;
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), 45. / 7. + 13., 0.00001);

  accTarget = -666;
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), -666. / 7. + 13., 0.00001);

  accMathWrite = 77.;
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 24); // 77/7 + 13

  accMathWrite = -140.;
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), -7); // -140/7 + 13
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReturnedReadWriteScalar) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  // ReturnedScalarReadWrite uses a return statement, but is otherwise identical to the "simple" version
  auto accMathRead = device.getScalarRegisterAccessor<double>("ReturnedScalarRead");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("ReturnedScalarWrite");

  accTarget = 45;
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), 45. / 7. + 13., 0.00001);

  accTarget = -666;
  accTarget.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), -666. / 7. + 13., 0.00001);

  accMathWrite = 77.;
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 24); // 77/7 + 13

  accMathWrite = -140.;
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), -7); // -140/7 + 13
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadWriteArray) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getOneDRegisterAccessor<int>("SimpleArray");
  auto accMathRead = device.getOneDRegisterAccessor<double>("ArrayRead");
  auto accMathWrite = device.getOneDRegisterAccessor<double>("ArrayWrite");
  BOOST_CHECK_EQUAL(accMathRead.getNElements(), 6);
  BOOST_CHECK_EQUAL(accMathWrite.getNElements(), 6);

  accTarget = {11, 22, 33, 44, 55, 66};
  accTarget.write();
  accMathRead.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_CLOSE(double(accMathRead[i]), accTarget[i] / 7. + 13., 0.00001);

  accTarget = {-120, 123456, -18, 9999, -999999999, 0};
  accTarget.write();
  accMathRead.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_CLOSE(double(accMathRead[i]), accTarget[i] / 7. + 13., 0.00001);

  accMathWrite = {-120, 123456, -18, 9999, -999999999, 0};
  accMathWrite.write();
  accTarget.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_EQUAL(double(accTarget[i]), std::round(accMathWrite[i] / 7. + 13.));

  accMathWrite = {0, 1, 2, 3, 4, 5};
  accMathWrite.write();
  accTarget.read();
  for(size_t i = 0; i < 6; ++i) BOOST_CHECK_EQUAL(double(accTarget[i]), std::round(accMathWrite[i] / 7. + 13.));
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testParameters) {
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto accTarget = device.getScalarRegisterAccessor<int>("SimpleScalar");
  auto scalarPar = device.getScalarRegisterAccessor<int>("ScalarParameter");
  auto arrayPar = device.getOneDRegisterAccessor<int>("SimpleArray");
  auto accMathRead = device.getScalarRegisterAccessor<double>("ScalarWithParametersRead");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("ScalarWithParametersWrite");

  accTarget = 42;
  accTarget.write();
  scalarPar = 6;
  scalarPar.write();
  arrayPar = {2, 3, 4, 5, 6, 7};
  arrayPar.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), 42. / 6. + (2 + 3 + 4 + 5 + 6 + 7), 0.00001);

  scalarPar = 7;
  scalarPar.write();
  arrayPar = {1, -1, 1, -1, 1, -1};
  arrayPar.write();
  accMathRead.read();
  BOOST_CHECK_CLOSE(double(accMathRead), 42. / 7., 0.00001);

  accMathWrite = 56;
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 8);

  scalarPar = 4;
  scalarPar.write();
  accMathWrite.write();
  accTarget.read();
  BOOST_CHECK_EQUAL(int(accTarget), 14);
}

/********************************************************************************************************************/

/////**
// * dummy backend used for testing the double buffering handshake.
// * a double-buffer read consists of (write ctrl, read buffernumber, read other buffer, write ctrl)
// * The overwritten functions of this class refer to the inner protocol
// */
// struct DummyForCleanupCheck : public LogicalNameMappingBackend {
//  using LogicalNameMappingBackend::LogicalNameMappingBackend;

//  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
//    return returnInstance<DummyForCleanupCheck>(
//        parameters.at("map"), DummyBackend::convertPathRelativeToDmapToAbs(parameters.at("map")));
//  }
//  ~DummyForCleanupCheck() override { // TODO check called
//    cleanupCalled = true;
//  }

//  struct BackendRegisterer {
//    BackendRegisterer() {
//      ChimeraTK::BackendFactory::getInstance().registerBackendType(
//          "DummyForCleanupCheck", &DummyForCleanupCheck::createInstance, {"map"});
//    }
//  };
//  static std::atomic_bool cleanupCalled;
//};
// static std::string rawDeviceCdd("(DummyForDoubleBuffering?map=doubleBuffer.map)");

// static DummyForCleanupCheck::BackendRegisterer gDFCCRegisterer;

BOOST_AUTO_TEST_CASE(testParametersCleanup) {
  // regression test for bug https://redmine.msktools.desy.de/issues/11506
  // (math plugin + push-parameter + shm has resource cleanup problem)
  setDMapFilePath("mathPluginWithPushPars.dmap");
  ChimeraTK::Device device;
  device.open("EOD");
  device.activateAsyncRead();
  ChimeraTK::Device targetDevice;
  targetDevice.open("HOLD");

  auto accTarget = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/WORD_G");
  auto scalarPar = device.getScalarRegisterAccessor<uint32_t>("DET/EXPOSURE");
  auto accMathWrite = device.getScalarRegisterAccessor<double>("DET/GAIN");

  scalarPar = 2;
  scalarPar.write();

  accMathWrite = 1;
  accMathWrite.write();

  usleep(1000000); // TODO improve
  accTarget.readLatest();
  BOOST_CHECK_EQUAL(int(accTarget), 10 * (int)scalarPar + (int)accMathWrite);

  // next, check that

  // TODO we should add a check that things got cleaned up - but how? check shm status? not nice!
  // or better, Exception Dummy?
  // this check is problematic - we might have to trigger cleanup from the test.
  // BOOST_CHECK(DummyForCleanupCheck::cleanupCalled);
}
BOOST_AUTO_TEST_CASE(testPushPars) {
  setDMapFilePath("mathPluginWithPushPars.dmap");
  ChimeraTK::Device targetDevice;
  targetDevice.open("HOLD");

  auto accTarget = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/WORD_G");
  auto pollPar = targetDevice.getScalarRegisterAccessor<uint32_t>("HOLD0/POLLPAR");
  pollPar = 1;
  pollPar.write();

  ChimeraTK::Device device;
  device.open("EOD");
  device.activateAsyncRead();
  auto scalarPar = device.getScalarRegisterAccessor<uint32_t>("DET/EXPOSURE");

  scalarPar = 2;
  scalarPar.write();

  // check that even if main value (x in formula) is not written, writing parameter should given proper result
  int accMathWrite0 = 0; // initial value for "x"

  usleep(1000000); // TODO improve
  accTarget.readLatest();
  BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)scalarPar + accMathWrite0);

  auto accMathWrite = device.getScalarRegisterAccessor<double>("DET/GAIN");
  accMathWrite = 3;
  accMathWrite.write();

  usleep(1000000); // TODO improve
  accTarget.readLatest();
  BOOST_CHECK_EQUAL(int(accTarget), 100 * (int)pollPar + 10 * (int)scalarPar + (int)accMathWrite);
  //
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptions) {
  // missing parameter "formula"
  ChimeraTK::Device device;
  BOOST_CHECK_THROW(device.open("(logicalNameMap?map=mathPlugin-broken.xlmap)"), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.open("(logicalNameMap?map=mathPlugin-broken2.xlmap)"), ChimeraTK::logic_error);

  // open device with map file which parses and all contained formulas compile
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto acc1 = device.getOneDRegisterAccessor<double>("WrongReturnSizeInArray");
  BOOST_CHECK_THROW(acc1.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc1.write(), ChimeraTK::logic_error);

  auto acc2 = device.getOneDRegisterAccessor<double>("ReturnScalarDespiteArray");
  BOOST_CHECK_THROW(acc2.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc2.write(), ChimeraTK::logic_error);

  auto acc3 = device.getOneDRegisterAccessor<double>("ReturnString");
  BOOST_CHECK_THROW(acc3.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc3.write(), ChimeraTK::logic_error);

  auto acc4 = device.getOneDRegisterAccessor<double>("ReturnMultipleValues");
  BOOST_CHECK_THROW(acc4.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(acc4.write(), ChimeraTK::logic_error);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCDATAFormula) {
  ChimeraTK::Device device;

  // open device with map file which parses
  device.open("(logicalNameMap?map=mathPlugin.xlmap)");

  auto acc = device.getScalarRegisterAccessor<double>("FormulaWithCdata");
  acc.read();
  BOOST_TEST(int(acc) == 24);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
