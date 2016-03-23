/*
 * testLMapBackend.cpp
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "Device.h"
#include "BufferingRegisterAccessor.h"

#include "accessPrivateData.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

// we need to access the private implementation of the accessor (see accessPrivateData.h)
struct BufferingRegisterAccessor_int32t_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<int32_t> >(NDRegisterAccessorBridge<int32_t>::*type);
};
template class accessPrivateData::stow_private<BufferingRegisterAccessor_int32t_impl, &mtca4u::NDRegisterAccessorBridge<int32_t>::_impl>;


class LMapBackendTest {
  public:
    void testExceptions();
    void testReadWriteConstant();
    void testReadWriteVariable();
    void testReadWriteRegister();
    void testReadWriteRange();
    void testRegisterAccessorForRegister();
    void testRegisterAccessorForRange();
    void testRegisterAccessorForChannel();
    void testNonBufferingAccessor();
    void testVariableChannelNumber();
    void testPlugin();  // TODO @todo move this test to its own executable
    void testOther();
};

class LMapBackendTestSuite : public test_suite {
  public:
    LMapBackendTestSuite() : test_suite("LogicalNameMappingBackend test suite") {
      boost::shared_ptr<LMapBackendTest> lMapBackendTest(new LMapBackendTest);

      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testExceptions, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteConstant, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteVariable, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRange, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRange, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForChannel, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testNonBufferingAccessor, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testVariableChannelNumber, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testPlugin, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testOther, lMapBackendTest) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "LogicalNameMappingBackend test suite";
  framework::master_test_suite().add(new LMapBackendTestSuite());

  return NULL;
}

/********************************************************************************************************************/

void LMapBackendTest::testExceptions() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;
  device.open("LMAP0");
  int data = 0;

  BOOST_CHECK_THROW(device.getRegisterMap(), mtca4u::DeviceException);
  try {
    device.getRegisterMap();
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK( e.getID() == mtca4u::DeviceException::NOT_IMPLEMENTED);
  }

  BOOST_CHECK_THROW(device.writeReg("Channel3", &data), mtca4u::DeviceException);
  try {
    device.writeReg("Channel3", &data);
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK( e.getID() == mtca4u::DeviceException::NOT_IMPLEMENTED);
  }

  BOOST_CHECK_THROW(device.readReg("Channel3", &data), mtca4u::DeviceException);
  try {
    device.readReg("Channel3", &data);
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK( e.getID() == mtca4u::DeviceException::NOT_IMPLEMENTED);
  }

  BOOST_CHECK_THROW(device.getRegistersInModule("MODULE"), mtca4u::DeviceException);
  try {
    device.getRegistersInModule("MODULE");
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK( e.getID() == mtca4u::DeviceException::NOT_IMPLEMENTED);
  }

  BOOST_CHECK_THROW(device.getRegisterAccessorsInModule("MODULE"), mtca4u::DeviceException);
  try {
    device.getRegisterAccessorsInModule("MODULE");
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK( e.getID() == mtca4u::DeviceException::NOT_IMPLEMENTED);
  }

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
  BOOST_CHECK_THROW( device.writeReg("Constant",&res), mtca4u::DeviceException );

  device.readReg("Constant",&res, 4);
  BOOST_CHECK( res == 42 );

  // test with buffering register accessor
  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","Constant");
  BOOST_CHECK( acc.getNumberOfElements() == 1 );
  BOOST_CHECK( acc[0] == 42 );
  acc.read();
  BOOST_CHECK( acc[0] == 42 );
  BOOST_CHECK_THROW( acc.write(), DeviceException );
  try {
    acc.write();
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::REGISTER_IS_READ_ONLY);
  }

  mtca4u::BufferingRegisterAccessor<int32_t> acc2 = device.getBufferingRegisterAccessor<int32_t>("","Constant");
  mtca4u::BufferingRegisterAccessor<int32_t> acc3 = device.getBufferingRegisterAccessor<int32_t>("","Constant2");

  boost::shared_ptr< NDRegisterAccessor<int32_t> > impl,impl2, impl3;
  impl = acc.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  impl2 = acc2.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  impl3 = acc3.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;

  BOOST_CHECK( impl->isSameRegister(impl2) == true );
  BOOST_CHECK( impl->isSameRegister(impl3) == false );

  device.close();

}

  /********************************************************************************************************************/

    void LMapBackendTest::testReadWriteVariable() {

    BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
    mtca4u::Device device;

    device.open("LMAP0");

    // test with buffering register accessor
    mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","/MyModule/SomeSubmodule/Variable");
    mtca4u::BufferingRegisterAccessor<int32_t> acc2 = device.getBufferingRegisterAccessor<int32_t>("","/MyModule/SomeSubmodule/Variable");
    BOOST_CHECK( acc.getNumberOfElements() == 1 );
    BOOST_CHECK( acc[0] == 2 );
    BOOST_CHECK( acc2[0] == 2 );
    acc.read();
    BOOST_CHECK( acc[0] == 2 );
    acc[0] = 3;
    BOOST_CHECK( acc[0] == 3 );
    BOOST_CHECK( acc2[0] == 2 );
    acc.write();
    acc2.read();
    BOOST_CHECK( acc[0] == 3 );
    BOOST_CHECK( acc2[0] == 3 );

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

  // single word
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

/*  for(int i=0; i<1024; i++) area[i] = 0;
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
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] == -876543210+42*i ); */

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

  mtca4u::BufferingRegisterAccessor<int32_t> acc2 = device.getBufferingRegisterAccessor<int32_t>("","PartOfArea");

  boost::shared_ptr< NDRegisterAccessor<int32_t> > impl,impl2;
  impl = acc.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  impl2 = acc2.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;

  BOOST_CHECK( impl->isSameRegister( impl ) == true );
  BOOST_CHECK( impl2->isSameRegister( impl ) == false );
  BOOST_CHECK( impl->isSameRegister( impl2 ) == false );

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

  // swap with std::vector
  std::vector<int32_t> vec(1024);
  acc.swap(vec);
  for(unsigned int i=0; i<vec.size(); i++) {
    BOOST_CHECK( vec[i] == -876543210+42*(signed)i );
  }

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

  mtca4u::BufferingRegisterAccessor<int32_t> acc3_2 = device.getBufferingRegisterAccessor<int32_t>("","Channel3");

  boost::shared_ptr< NDRegisterAccessor<int32_t> > impl3,impl4,impl3_2;
  impl3 = acc3.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  impl4 = acc4.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  impl3_2 = acc3_2.*accessPrivateData::stowed< BufferingRegisterAccessor_int32t_impl >::value;
  BOOST_CHECK( impl3->isSameRegister( impl3_2 ) == true );
  BOOST_CHECK( impl3->isSameRegister( impl4 ) == false );

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

  // read via iterators
  unsigned int idx=0;
  for(BufferingRegisterAccessor<int>::iterator it = acc3.begin(); it != acc3.end(); ++it) {
    BOOST_CHECK( *it == (signed) (3000+idx) );
    ++idx;
  }
  BOOST_CHECK(idx == nSamples);

  // read via const iterators
  const mtca4u::BufferingRegisterAccessor<int32_t> &acc3_const = acc3;
  idx=0;
  for(BufferingRegisterAccessor<int>::const_iterator it = acc3_const.begin(); it != acc3_const.end(); ++it) {
    BOOST_CHECK( *it == (signed) (3000+idx) );
    ++idx;
  }
  BOOST_CHECK(idx == nSamples);

  // read via reverse iterators
  idx=nSamples;
  for(BufferingRegisterAccessor<int>::reverse_iterator it = acc3.rbegin(); it != acc3.rend(); ++it) {
    --idx;
    BOOST_CHECK( *it == (signed) (3000+idx) );
  }
  BOOST_CHECK(idx == 0);

  // read via reverse const iterators
  idx=nSamples;
  for(BufferingRegisterAccessor<int>::const_reverse_iterator it = acc3_const.rbegin(); it != acc3_const.rend(); ++it) {
    --idx;
    BOOST_CHECK( *it == (signed) (3000+idx) );
  }
  BOOST_CHECK(idx == 0);

  // swap into other vector
  std::vector<int> someVector(nSamples);
  acc3.swap(someVector);
  for(unsigned int i=0; i<nSamples; i++) {
    BOOST_CHECK( someVector[i] == (signed) (3000+i) );
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

/********************************************************************************************************************/

void LMapBackendTest::testNonBufferingAccessor() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  target1.open("PCIE2");
  device.open("LMAP0");

  boost::shared_ptr<mtca4u::RegisterAccessor> acc;

  // full area (pass-through a standard BackendBufferingRegisterAccessor)
  acc = device.getRegisterAccessor("FullArea");

  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  acc->read(area.data(), 1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  acc->read(area.data(), 1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  acc->write(area.data(), 1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  acc->write(area.data(), 1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) BOOST_CHECK( area[i] == -876543210+42*i );

  // range (special LNMBackendRegisterAccessor)
  acc = device.getRegisterAccessor("PartOfArea");

  for(int i=0; i<1024; i++) area[i] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  acc->read(area.data(), 20);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i] == 12345+3*(i+10) );

  for(int i=0; i<1024; i++) area[i] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<1024; i++) area[i] = 0;
  acc->read(area.data(), 20);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i] == -876543210+42*(i+10) );

  /* no longer supported
  for(int i=0; i<20; i++) area[i] = 12345+3*(i+10);
  acc->write(area.data(), 20);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=10; i<30; i++) BOOST_CHECK( area[i] == 12345+3*i );

  for(int i=0; i<20; i++) area[i] = -876543210+42*(i+10);
  acc->write(area.data(), 20);
  for(int i=0; i<1024; i++) area[i] = 0;
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=10; i<30; i++) BOOST_CHECK( area[i] == -876543210+42*i );
  */

  device.close();
  target1.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testVariableChannelNumber() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target;
  device.open("LMAP0");
  target.open("PCIE3");

  mtca4u::TwoDRegisterAccessor<int32_t> acc2D = target.getTwoDRegisterAccessor<int32_t>("TEST","NODMA");
  mtca4u::BufferingRegisterAccessor<int32_t> accVar = device.getBufferingRegisterAccessor<int32_t>("","/MyModule/ConfigurableChannel");
  mtca4u::BufferingRegisterAccessor<int32_t> accChannel = device.getBufferingRegisterAccessor<int32_t>("","/MyModule/SomeSubmodule/Variable");

  // fill channels
  for(unsigned int i=0; i<acc2D.getNumberOfDataSequences(); i++) {
    for(unsigned int k=0; k<acc2D[i].size(); k++) {
      acc2D[i][k] = i*1000 + k;
    }
  }
  acc2D.write();

  // read back all channels
  for(unsigned int i=0; i<acc2D.getNumberOfDataSequences(); i++) {

      // configure "ConfigurableChannel" to use channel i
    accChannel = i;
    accChannel.write();

    // read back via ConfigurableChannel
    accVar.read();
    BOOST_CHECK( accVar.getNumberOfElements() == acc2D[i].size() );
    for(unsigned int k=0; k<accVar.getNumberOfElements(); k++) {
      BOOST_CHECK( accVar[k] == (signed)i*1000 + (signed)k );
    }

  }

}

/********************************************************************************************************************/

void LMapBackendTest::testPlugin() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;
  device.open("LMAP0");

  // single word scaled
  auto accDirect = device.getBufferingRegisterAccessor<double>("","SingleWord");
  auto accScaled = device.getBufferingRegisterAccessor<double>("","SingleWord_Scaled");
  accDirect = 11;
  accDirect.write();

  accScaled.read();
  BOOST_CHECK( accScaled == 33 );

  accScaled = 66;
  accScaled.write();

  accDirect.read();
  BOOST_CHECK( accDirect == 22 );

  // scaled area
  auto accDirect2 = device.getBufferingRegisterAccessor<int>("","FullArea");
  auto accScaled2 = device.getBufferingRegisterAccessor<double>("","FullArea_Scaled");

  BOOST_CHECK( accDirect2.getNumberOfElements() == accScaled2.getNumberOfElements() );

  for(unsigned int i=0; i<accDirect2.getNumberOfElements(); i++) {
    accDirect2[i] = (signed)i-10;
  }
  accDirect2.write();

  accScaled2.read();
  for(unsigned int i=0; i<accScaled2.getNumberOfElements(); i++) {
    BOOST_CHECK( accScaled2[i] == ((signed)i-10)*11.5 );
  }

  for(unsigned int i=0; i<accScaled2.getNumberOfElements(); i++) {
    accScaled2[i] = ((signed)i+30)*11.5;
  }
  accScaled2.write();

  accDirect2.read();
  for(unsigned int i=0; i<accDirect2.getNumberOfElements(); i++) {
    BOOST_CHECK( accDirect2[i] == (signed)i+30 );
  }

  // non-buffering accessor
  auto accDirect3 = device.getRegisterAccessor("FullArea","");
  auto accScaled3 = device.getRegisterAccessor("FullArea_Scaled","");

  BOOST_CHECK( accDirect3->getNumberOfElements() == accScaled3->getNumberOfElements() );
  std::vector<int> bufferDirect(accDirect3->getNumberOfElements());
  std::vector<double> bufferScaled(accScaled3->getNumberOfElements());

  for(unsigned int i=0; i<bufferDirect.size(); i++) {
    bufferDirect[i] = (signed)i-10;
  }
  accDirect3->write(bufferDirect.data(), bufferDirect.size());

  accScaled3->read(bufferScaled.data(), bufferScaled.size());
  for(unsigned int i=0; i<bufferScaled.size(); i++) {
    BOOST_CHECK( bufferScaled[i] == ((signed)i-10)*11.5 );
  }

  for(unsigned int i=0; i<bufferScaled.size(); i++) {
    bufferScaled[i] = ((signed)i+30)*11.5;
  }
  accScaled3->write(bufferScaled.data(), bufferScaled.size());

  accDirect3->read(bufferDirect.data(), bufferDirect.size());
  for(unsigned int i=0; i<bufferDirect.size(); i++) {
    BOOST_CHECK( bufferDirect[i] == (signed)i+30 );
  }

}

/********************************************************************************************************************/

void LMapBackendTest::testOther() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;
  device.open("LMAP0");

  BOOST_CHECK( device.readDeviceInfo().find("Logical name mapping file:") == 0 );

}
