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
    void testRegisterAccessorForChannel();
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
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForChannel, lMapBackendTest) );
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
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","FullArea");
  BOOST_CHECK( !acc.isReadOnly() );

  const mtca4u::BufferingRegisterAccessor<int32_t> acc_const = acc;

  // reading via [] operator
  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<1024; i++) BOOST_CHECK( acc[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<1024; i++) BOOST_CHECK( acc[i] == -876543210+42*i );

  // writing via [] operator
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

  // reading via iterator
  index = 0;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::iterator it = acc.begin(); it != acc.end(); ++it) {
    BOOST_CHECK( *it == -876543210+42*index );
    ++index;
  }
  BOOST_CHECK( index == 1024 );

  // reading via const_iterator
  index = 0;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::const_iterator it = acc_const.begin(); it != acc_const.end(); ++it) {
    BOOST_CHECK( *it == -876543210+42*index );
    ++index;
  }
  BOOST_CHECK( index == 1024 );

  // reading via reverse_iterator
  index = 1024;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::reverse_iterator it = acc.rbegin(); it != acc.rend(); ++it) {
    --index;
    BOOST_CHECK( *it == -876543210+42*index );
  }
  BOOST_CHECK( index == 0 );

  // reading via const_reverse_iterator
  index = 1024;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::const_reverse_iterator it = acc_const.rbegin(); it != acc_const.rend(); ++it) {
    --index;
    BOOST_CHECK( *it == -876543210+42*index );
  }
  BOOST_CHECK( index == 0 );

  device.close();
  target1.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForRange() {
  std::vector<int> area(1024);
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","PartOfArea");

  const mtca4u::BufferingRegisterAccessor<int32_t> acc_const = acc;

  for(int i=0; i<20; i++) area[i+10] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == 12345+3*i );

  for(int i=0; i<20; i++) area[i+10] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == -876543210+42*i );

  // writing range registers fails
  BOOST_CHECK( acc.isReadOnly() );
  BOOST_CHECK_THROW( acc.write(), DeviceException );
  try {
    acc.write();
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::REGISTER_IS_READ_ONLY);
  }

  // reading via iterator
  index = 0;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::iterator it = acc.begin(); it != acc.end(); ++it) {
    BOOST_CHECK( *it == -876543210+42*index );
    ++index;
  }
  BOOST_CHECK( index == 20 );

  // reading via const_iterator
  index = 0;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::const_iterator it = acc_const.begin(); it != acc_const.end(); ++it) {
    BOOST_CHECK( *it == -876543210+42*index );
    ++index;
  }
  BOOST_CHECK( index == 20 );

  // reading via reverse_iterator
  index = 20;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::reverse_iterator it = acc.rbegin(); it != acc.rend(); ++it) {
    --index;
    BOOST_CHECK( *it == -876543210+42*index );
  }
  BOOST_CHECK( index == 0 );

  // reading via const_reverse_iterator
  index = 20;
  for(mtca4u::BufferingRegisterAccessor<int32_t>::const_reverse_iterator it = acc_const.rbegin(); it != acc_const.rend(); ++it) {
    --index;
    BOOST_CHECK( *it == -876543210+42*index );
  }
  BOOST_CHECK( index == 0 );

  device.close();
  target1.close();

}

  /********************************************************************************************************************/

  void LMapBackendTest::testRegisterAccessorForChannel() {
    std::vector<int> area(1024);

    BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
    mtca4u::Device device, target1;

    target1.open("PCIE3");
    device.open("LMAP0");

    mtca4u::BufferingRegisterAccessor<int32_t> acc3 = device.getBufferingRegisterAccessor<int32_t>("","Channel3");
    mtca4u::BufferingRegisterAccessor<int32_t> acc4 = device.getBufferingRegisterAccessor<int32_t>("","Channel4");

    mtca4u::TwoDRegisterAccessor<int32_t> accTarget = target1.getTwoDRegisterAccessor<int32_t>("TEST","NODMA");
    unsigned int nSamples = accTarget[3].size();
    BOOST_CHECK( accTarget[4].size() == nSamples );
    BOOST_CHECK( acc3.getNumberOfElements() == nSamples );
    BOOST_CHECK( acc4.getNumberOfElements() == nSamples );

    // fill target register
    for(unsigned int i=0; i<nSamples; i++) {
      accTarget[3][i] = 3000+i;
      accTarget[4][i] = 4000-i;
    }
    accTarget.write();

    // clear channel accessor buffers
    for(unsigned int i=0; i<nSamples; i++) {
      acc3[i] = 0;
      acc4[i] = 0;
    }

    // read channel accessors
    acc3.read();
    for(unsigned int i=0; i<nSamples; i++) {
      BOOST_CHECK( acc3[i] == (signed) (3000+i) );
      BOOST_CHECK( acc4[i] == 0 );
    }
    acc4.read();
    for(unsigned int i=0; i<nSamples; i++) {
      BOOST_CHECK( acc3[i] == (signed) (3000+i) );
      BOOST_CHECK( acc4[i] == (signed) (4000-i) );
    }

    // write channel registers fails
    BOOST_CHECK( acc3.isReadOnly() );
    BOOST_CHECK( acc4.isReadOnly() );
    BOOST_CHECK_THROW( acc3.write(), DeviceException );
    try {
      acc3.write();
    }
    catch(DeviceException &e) {
      BOOST_CHECK(e.getID() == DeviceException::REGISTER_IS_READ_ONLY);
    }
    BOOST_CHECK_THROW( acc4.write(), DeviceException );
    try {
      acc4.write();
    }
    catch(DeviceException &e) {
      BOOST_CHECK(e.getID() == DeviceException::REGISTER_IS_READ_ONLY);
    }

    device.close();
    target1.close();

}
