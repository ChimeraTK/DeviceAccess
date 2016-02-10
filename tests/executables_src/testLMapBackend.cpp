/*
 * testLMapBackend.cpp
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "Device.h"
#include "BufferingRegisterAccessor.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

class LMapBackendTest {
  public:
    void testReadWriteConstant();
    void testReadWriteRegister();
    void testReadWriteRange();
    void testRegisterAccessorForRegister();
    void testRegisterAccessorForRange();
};

class LMapBackendTestSuite : public test_suite {
  public:
    LMapBackendTestSuite() : test_suite("LogicalNameMappingBackend test suite") {
      boost::shared_ptr<LMapBackendTest> lMapBackendTest(new LMapBackendTest);

      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteConstant, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRange, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRange, lMapBackendTest) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "LogicalNameMappingBackend test suite";
  framework::master_test_suite().add(new LMapBackendTestSuite());

  return NULL;
}

/********************************************************************************************************************/

void LMapBackendTest::testReadWriteConstant() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;

  device.open("LMAP0");
  int res = 0;
  device.readReg("Constant",&res, 4);
  BOOST_CHECK( res == 42 );

  res = 0;
  device.readReg("Constant",&res, 3);
  BOOST_CHECK( res == 0 );

  res = 0;
  device.readReg("Constant",&res, 100); // this would be wrong with any other backend or register type...
  BOOST_CHECK( res == 42 );

  res = 10;
  device.writeReg("Constant",&res);     // should have no effect

  device.readReg("Constant",&res, 4);
  BOOST_CHECK( res == 42 );

  device.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testReadWriteRegister() {
  int res;
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  // singla word
  res = 120;
  target1.writeReg("BOARD.WORD_USER", &res, 4);
  res = 0;
  device.readReg("SingleWord",&res, 4);
  BOOST_CHECK( res == 120 );

  res = 66;
  target1.writeReg("BOARD.WORD_USER", &res, 4);
  res = 0;
  device.readReg("SingleWord",&res, 4);
  BOOST_CHECK( res == 66 );

  res = 42;
  device.writeReg("SingleWord",&res, 4);
  res = 0;
  target1.readReg("BOARD.WORD_USER", &res, 4);
  BOOST_CHECK( res == 42 );

  res = 12;
  device.writeReg("SingleWord",&res, 4);
  res = 0;
  target1.readReg("BOARD.WORD_USER", &res, 4);
  BOOST_CHECK( res == 12 );

  // area
  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  device.readReg("FullArea",area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  device.readReg("FullArea",area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  device.writeReg("FullArea",area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  device.writeReg("FullArea",area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  device.close();
  target1.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testReadWriteRange() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  for(int i=0; i<1024; i++) area[i] = 0;
  for(int i=0; i<20; i++) area[i+10] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  device.readReg("PartOfArea",area.data(), 4*20);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = 0;
  for(int i=0; i<20; i++) area[i+10] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  device.readReg("PartOfArea",area.data(), 4*20);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  for(int i=0; i<1024; i++) area[i] = 0;
  for(int i=0; i<20; i++) area[i] = 12345+3*i;
  device.writeReg("PartOfArea",area.data(), 4*20);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = 0;
  for(int i=0; i<20; i++) area[i] = -876543210+42*i;
  device.writeReg("PartOfArea",area.data(), 4*20);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] == -876543210+42*i );

  device.close();
  target1.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForRegister() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","FullArea");

  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<1024; i++) BOOST_CHECK( acc[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<1024; i++) BOOST_CHECK( acc[i] == -876543210+42*i );

  for(int i=0; i<1024; i++) acc[i] = 12345+3*i;
  acc.write();
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) acc[i] = -876543210+42*i;
  acc.write();
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  device.close();
  target1.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForRange() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","PartOfArea");

  for(int i=0; i<20; i++) area[i+10] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == 12345+3*i );

  for(int i=0; i<20; i++) area[i+10] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == -876543210+42*i );

  for(int i=0; i<20; i++) acc[i] = 12345+3*i;
  acc.write();
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] == 12345+3*i );

  for(int i=0; i<20; i++) acc[i] = -876543210+42*i;
  acc.write();
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] == -876543210+42*i );

  device.close();
  target1.close();

}
