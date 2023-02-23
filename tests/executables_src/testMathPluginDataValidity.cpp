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

  //set a parameter to faulty.
  scalarPar.setDataValidity(ChimeraTK::DataValidity::faulty);
  scalarPar.write();
  
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::faulty);
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);
  //other parameters should be ok.
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);
  
  scalarPar.setDataValidity(ChimeraTK::DataValidity::ok);
  scalarPar.write();
  
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
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
    
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);

  //parameters should be ok.  
  scalarPar.read();
  BOOST_CHECK(scalarPar.dataValidity() == ChimeraTK::DataValidity::ok);
  arrayPar.read();
  BOOST_CHECK(arrayPar.dataValidity() == ChimeraTK::DataValidity::ok);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadPush ) {
  ChimeraTK::Device device;
  setDMapFilePath("pushWithData.dmap");
  device.open("ExcepLogical");
    
  ChimeraTK::Device directDevice;
  directDevice.open("(ExceptionDummy:1?map=pushWithData.mapp)");
  auto accTarget = directDevice.getScalarRegisterAccessor<int>("pushcontent");
  auto accTargetASync = directDevice.getScalarRegisterAccessor<int>("pushcontent/PUSH_READ",0, {AccessMode::wait_for_new_data});
  auto pushable = directDevice.getScalarRegisterAccessor<int>("pushable");
  
  auto accTargetRead = directDevice.getScalarRegisterAccessor<int>("pushcontent");


  
  auto accMathRead = device.getScalarRegisterAccessor<double>("SimplePushRead");
  
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);

  accTarget.setDataValidity(ChimeraTK::DataValidity::faulty);
  accTarget = 999;
  accTarget.write();
  
  accTargetRead.read();
  std::cout<<"accTargetRead:"<<accTargetRead<<std::endl;
  if ( accTargetRead.dataValidity() == ChimeraTK::DataValidity::faulty) {
      std::cout<<"accTargetRead ChimeraTK::DataValidity::faulty"<<std::endl;

  }
  accTargetASync.readLatest();
  
  std::cout<<"accTargetASync:"<<accTargetASync<<std::endl;
  BOOST_CHECK(accTargetASync.dataValidity() == ChimeraTK::DataValidity::faulty);

  
  //auto eDummy = boost::dynamic_pointer_cast<ExceptionDummy>(directDevice.getBackend());
  //eDummy->triggerInterrupt(0, 0);
  //usleep(50000);
  accMathRead.read();
  std::cout<<"accMathRead:"<<accMathRead<<std::endl;
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::faulty);
  
  accTarget.setDataValidity(ChimeraTK::DataValidity::ok);
  accTarget.write();
  //eDummy->triggerInterrupt(0, 0);
  accMathRead.read();
  BOOST_CHECK(accMathRead.dataValidity() == ChimeraTK::DataValidity::ok);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteExternalDevice ) {
  ChimeraTK::Device device;
  setDMapFilePath("pushWithData.dmap");
  device.open("ExcepLogical");
    
  ChimeraTK::Device directDevice;
  directDevice.open("(ExceptionDummy:1?map=pushWithData.mapp)");
  auto accTarget = directDevice.getScalarRegisterAccessor<int>("pushcontent");
  auto pushable = directDevice.getScalarRegisterAccessor<int>("pushable");

  
  auto accMathWrite = device.getScalarRegisterAccessor<double>("SimpleWrite");
  
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
  

  accMathWrite.setDataValidity(ChimeraTK::DataValidity::faulty);
  accMathWrite = 999;
  accMathWrite.write();
  
  //auto eDummy = boost::dynamic_pointer_cast<ExceptionDummy>(directDevice.getBackend());
  //eDummy->triggerInterrupt(0, 0);
  //usleep(50000);
  accTarget.read();
  std::cout<<"accTarget:"<<accTarget<<std::endl;
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::faulty);
  
  accMathWrite.setDataValidity(ChimeraTK::DataValidity::ok);
  accMathWrite.write();
  //eDummy->triggerInterrupt(0, 0);
  accTarget.read();
  BOOST_CHECK(accTarget.dataValidity() == ChimeraTK::DataValidity::ok);
}
/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
