///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include "Device.h"
#include "BufferingRegisterAccessor.h"

using namespace boost::unit_test_framework;
namespace mtca4u{
  using namespace ChimeraTK;
}
using namespace mtca4u;


class LMapBackendTest {
  public:
    void testExceptions();
    void testCatalogue();
    void testReadWriteConstant();
    void testReadWriteVariable();
    void testReadWriteRegister();
    void testReadWriteRange();
    void testRegisterAccessorForRegister();
    void testRegisterAccessorForRange();
    void testRegisterAccessorForChannel();
    void testRegisterAccessorForBit();
    void testNonBufferingAccessor();
    void testOther();
};

class LMapBackendTestSuite : public test_suite {
  public:
    LMapBackendTestSuite() : test_suite("LogicalNameMappingBackend test suite") {
      boost::shared_ptr<LMapBackendTest> lMapBackendTest(new LMapBackendTest);

      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testExceptions, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testCatalogue, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteConstant, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteVariable, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testReadWriteRange, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRegister, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForRange, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForChannel, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testRegisterAccessorForBit, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testNonBufferingAccessor, lMapBackendTest) );
      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testOther, lMapBackendTest) );
    }
};

bool init_unit_test(){
  framework::master_test_suite().p_name.value = "LogicalNameMappingBackend test suite";
  framework::master_test_suite().add(new LMapBackendTestSuite());

  return true;
}

/********************************************************************************************************************/

void LMapBackendTest::testExceptions() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;
  BOOST_CHECK(device.isOpened() == false);
  device.open("LMAP0");
  BOOST_CHECK(device.isOpened() == true);
  int data = 0;

  BOOST_CHECK_THROW(device.getRegisterMap(), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.writeReg("Channel3", &data), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.readReg("Channel3", &data), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.getRegistersInModule("MODULE"), ChimeraTK::logic_error);

  BOOST_CHECK_THROW(device.getRegisterAccessorsInModule("MODULE"), ChimeraTK::logic_error);
  BOOST_CHECK(device.isOpened() == true);
  device.close();
  BOOST_CHECK(device.isOpened() == false);

}

/********************************************************************************************************************/

  void LMapBackendTest::testCatalogue() {

    BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
    mtca4u::Device device;

    device.open("LMAP0");

    const RegisterCatalogue &catalogue = device.getRegisterCatalogue();

    boost::shared_ptr<RegisterInfo> info = catalogue.getRegister("SingleWord");
    BOOST_CHECK( info->getRegisterName() == "/SingleWord" );
    BOOST_CHECK( info->getNumberOfElements() == 1 );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 0 );

    info = catalogue.getRegister("FullArea");
    BOOST_CHECK( info->getRegisterName() == "/FullArea" );
    BOOST_CHECK( info->getNumberOfElements() == 0x400 );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 1 );

    info = catalogue.getRegister("PartOfArea");
    BOOST_CHECK( info->getRegisterName() == "/PartOfArea" );
    BOOST_CHECK( info->getNumberOfElements() == 20 );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 1 );

    mtca4u::Device target1;
    target1.open("PCIE3");
    mtca4u::TwoDRegisterAccessor<int32_t> accTarget = target1.getTwoDRegisterAccessor<int32_t>("TEST","NODMA");
    unsigned int nSamples = accTarget[3].size();

    info = catalogue.getRegister("Channel3");
    BOOST_CHECK( info->getRegisterName() == "/Channel3" );
    BOOST_CHECK( info->getNumberOfElements() == nSamples );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 1 );

    info = catalogue.getRegister("Constant2");
    BOOST_CHECK( info->getRegisterName() == "/Constant2" );
    BOOST_CHECK( info->getNumberOfElements() == 1 );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 0 );

    info = catalogue.getRegister("/MyModule/SomeSubmodule/Variable");
    BOOST_CHECK( info->getRegisterName() == "/MyModule/SomeSubmodule/Variable" );
    BOOST_CHECK( info->getNumberOfElements() == 1 );
    BOOST_CHECK( info->getNumberOfChannels() == 1 );
    BOOST_CHECK( info->getNumberOfDimensions() == 0 );
    device.close();

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
    BOOST_CHECK_THROW( device.writeReg("Constant",&res), ChimeraTK::logic_error );

    device.readReg("Constant",&res, 4);
    BOOST_CHECK( res == 42 );

    // test with buffering register accessor
    mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","Constant");
    BOOST_CHECK( acc.getNumberOfElements() == 1 );
    BOOST_CHECK( acc[0] == 42 );
    acc.read();
    BOOST_CHECK( acc[0] == 42 );
    BOOST_CHECK_THROW( acc.write(), ChimeraTK::logic_error );

    mtca4u::BufferingRegisterAccessor<int32_t> acc2 = device.getBufferingRegisterAccessor<int32_t>("","Constant");
    mtca4u::BufferingRegisterAccessor<int32_t> acc3 = device.getBufferingRegisterAccessor<int32_t>("","Constant2");

    boost::shared_ptr< NDRegisterAccessor<int32_t> > impl,impl2, impl3;
    impl = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc.getHighLevelImplElement());
    impl2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc2.getHighLevelImplElement());
    impl3 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3.getHighLevelImplElement());

    //BOOST_CHECK( impl->mayReplaceOther(impl2) == true );    // this is currently always set to false, since it doesn't really make any difference...
    BOOST_CHECK( impl->mayReplaceOther(impl3) == false );

    auto arrayConstant = device.getOneDRegisterAccessor<int>("/ArrayConstant");
    BOOST_CHECK_EQUAL( arrayConstant.getNElements(), 5 );
    BOOST_CHECK_EQUAL( arrayConstant[0], 1111 );
    BOOST_CHECK_EQUAL( arrayConstant[1], 2222 );
    BOOST_CHECK_EQUAL( arrayConstant[2], 3333 );
    BOOST_CHECK_EQUAL( arrayConstant[3], 4444 );
    BOOST_CHECK_EQUAL( arrayConstant[4], 5555 );
    arrayConstant.read();
    BOOST_CHECK_EQUAL( arrayConstant[0], 1111 );
    BOOST_CHECK_EQUAL( arrayConstant[1], 2222 );
    BOOST_CHECK_EQUAL( arrayConstant[2], 3333 );
    BOOST_CHECK_EQUAL( arrayConstant[3], 4444 );
    BOOST_CHECK_EQUAL( arrayConstant[4], 5555 );
    BOOST_CHECK_THROW( acc.write(), ChimeraTK::logic_error );

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

}

/********************************************************************************************************************/

void LMapBackendTest::testReadWriteRange() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");


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

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForRegister() {

  std::vector<int> area(1024);
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");


  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","FullArea");
  BOOST_CHECK( !acc.isReadOnly() );
  BOOST_CHECK( acc.isReadable() );
  BOOST_CHECK( acc.isWriteable () );

  mtca4u::BufferingRegisterAccessor<int32_t> acc2 = device.getBufferingRegisterAccessor<int32_t>("","PartOfArea");

  boost::shared_ptr< NDRegisterAccessor<int32_t> > impl,impl2;
  impl = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc.getHighLevelImplElement());
  impl2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc2.getHighLevelImplElement());

  BOOST_CHECK( impl != impl2 );
  BOOST_CHECK( impl->mayReplaceOther( impl ) == true );
  BOOST_CHECK( impl2->mayReplaceOther( impl ) == false );
  BOOST_CHECK( impl->mayReplaceOther( impl2 ) == false );

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

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForRange() {
  std::vector<int> area(1024);
  int index;

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");


  mtca4u::BufferingRegisterAccessor<int32_t> acc = device.getBufferingRegisterAccessor<int32_t>("","PartOfArea");
  BOOST_CHECK( acc.isReadOnly() == false );
  BOOST_CHECK( acc.isReadable() );
  BOOST_CHECK( acc.isWriteable() );

  const mtca4u::BufferingRegisterAccessor<int32_t> acc_const = acc;

  for(int i=0; i<20; i++) area[i+10] = 12345+3*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == 12345+3*i );

  for(int i=0; i<20; i++) area[i+10] = -876543210+42*i;
  target1.writeReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  acc.read();
  for(int i=0; i<20; i++) BOOST_CHECK( acc[i] == -876543210+42*i );

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

  // writing
  for(int i=0; i<20; i++) acc[i] = 24507+33*i;
  acc.write();
  target1.readReg("ADC.AREA_DMAABLE", area.data(), 4*1024);
  for(int i=0; i<20; i++) BOOST_CHECK( area[i+10] ==  24507+33*i );

  device.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForChannel() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE3");

  mtca4u::BufferingRegisterAccessor<int32_t> acc3 = device.getBufferingRegisterAccessor<int32_t>("","Channel3");
  mtca4u::BufferingRegisterAccessor<int32_t> acc4 = device.getBufferingRegisterAccessor<int32_t>("","Channel4");

  mtca4u::BufferingRegisterAccessor<int32_t> acc3_2 = device.getBufferingRegisterAccessor<int32_t>("","Channel3");

  boost::shared_ptr< NDRegisterAccessor<int32_t> > impl3,impl4,impl3_2;
  impl3 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3.getHighLevelImplElement());
  impl4 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc4.getHighLevelImplElement());
  impl3_2 = boost::dynamic_pointer_cast<NDRegisterAccessor<int32_t>>(acc3_2.getHighLevelImplElement());
  BOOST_CHECK( impl3->mayReplaceOther( impl3_2 ) == true );
  BOOST_CHECK( impl3->mayReplaceOther( impl4 ) == false );

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
  BOOST_CHECK( acc3.isReadable() );
  BOOST_CHECK( acc3.isWriteable() == false);

  BOOST_CHECK( acc4.isReadOnly() );
  BOOST_CHECK( acc4.isReadable() );
  BOOST_CHECK( acc4.isWriteable() == false);

  BOOST_CHECK_THROW( acc3.write(), ChimeraTK::logic_error );
  BOOST_CHECK_THROW( acc4.write(), ChimeraTK::logic_error );

  device.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testRegisterAccessorForBit() {
  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;

  device.open("LMAP0");

  auto bitField = device.getScalarRegisterAccessor<int>("/MyModule/SomeSubmodule/Variable");
  auto bit0 = device.getScalarRegisterAccessor<uint8_t>("/Bit0ofVar");
  auto bit1 = device.getScalarRegisterAccessor<uint16_t>("/Bit1ofVar");
  auto bit2 = device.getScalarRegisterAccessor<int32_t>("/Bit2ofVar");
  auto bit3 = device.getScalarRegisterAccessor<std::string>("/Bit3ofVar");

  bitField = 0;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 0 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 1;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 1 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 2;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 0 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 1 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 3;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 1 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 1 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 4;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 0 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 1 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 8;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 0 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "1" );

  bitField = 15;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 1 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 1 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 1 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "1" );

  bitField = 16;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 0 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 17;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 1 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bitField = 1;
  bitField.write();

  bit0.read();
  BOOST_CHECK_EQUAL( static_cast<uint8_t>(bit0), 1 );
  bit1.read();
  BOOST_CHECK_EQUAL( static_cast<uint16_t>(bit1), 0 );
  bit2.read();
  BOOST_CHECK_EQUAL( static_cast<int32_t>(bit2), 0 );
  bit3.read();
  BOOST_CHECK_EQUAL( static_cast<std::string>(bit3), "0" );

  bit2 = 1;
  bit2.write();
  bitField.read();
  BOOST_CHECK_EQUAL( static_cast<int>(bitField), 5 );

  bit1 = 1;
  bit1.write();
  bitField.read();
  BOOST_CHECK_EQUAL( static_cast<int>(bitField), 7 );

  bit0 = 0;
  bit0.write();
  bitField.read();
  BOOST_CHECK_EQUAL( static_cast<int>(bitField), 6 );

  bit3 = "1";
  bit3.write();
  bitField.read();
  BOOST_CHECK_EQUAL( static_cast<int>(bitField), 14 );

  device.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testNonBufferingAccessor() {
  std::vector<int> area(1024);

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1;

  device.open("LMAP0");
  target1.open("PCIE2");


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

  device.close();

}

/********************************************************************************************************************/

void LMapBackendTest::testOther() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;
  device.open("LMAP0");

  BOOST_CHECK( device.readDeviceInfo().find("Logical name mapping file:") == 0 );
  device.close();
}
