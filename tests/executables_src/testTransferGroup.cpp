///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include "TransferGroup.h"
#include "BufferingRegisterAccessor.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "Device.h"
#include "NDRegisterAccessorDecorator.h"

#include "accessPrivateData.h"

using namespace boost::unit_test_framework;
namespace mtca4u{
  using namespace ChimeraTK;
}
using namespace mtca4u;

// we need to access the private implementation of the accessor (see accessPrivateData.h)
struct BufferingRegisterAccessor_int_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<int> >(NDRegisterAccessorAbstractor<int>::*type);
};
template struct accessPrivateData::stow_private<BufferingRegisterAccessor_int_impl, &mtca4u::NDRegisterAccessorAbstractor<int>::_impl>;

struct BufferingRegisterAccessor_int32_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<int32_t> >(NDRegisterAccessorAbstractor<int32_t>::*type);
};
template struct accessPrivateData::stow_private<BufferingRegisterAccessor_int32_impl, &mtca4u::NDRegisterAccessorAbstractor<int32_t>::_impl>;

struct BufferingRegisterAccessor_uint16_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<uint16_t> >(NDRegisterAccessorAbstractor<uint16_t>::*type);
};
template struct accessPrivateData::stow_private<BufferingRegisterAccessor_uint16_impl, &mtca4u::NDRegisterAccessorAbstractor<uint16_t>::_impl>;

struct BufferingRegisterAccessor_int64_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<int64_t> >(NDRegisterAccessorAbstractor<int64_t>::*type);
};
template struct accessPrivateData::stow_private<BufferingRegisterAccessor_int64_impl, &mtca4u::NDRegisterAccessorAbstractor<int64_t>::_impl>;

struct NumericAddressedLowLevelTransferElement_startAddress {
    typedef size_t (NumericAddressedLowLevelTransferElement::*type);
};
template struct accessPrivateData::stow_private<NumericAddressedLowLevelTransferElement_startAddress,
        &mtca4u::NumericAddressedLowLevelTransferElement::_startAddress>;

struct NumericAddressedLowLevelTransferElement_numberOfBytes {
    typedef size_t (NumericAddressedLowLevelTransferElement::*type);
};
template struct accessPrivateData::stow_private<NumericAddressedLowLevelTransferElement_numberOfBytes,
        &mtca4u::NumericAddressedLowLevelTransferElement::_numberOfBytes>;


class TransferGroupTest {
  public:
    void testAdding();
    void testLogicalNameMappedRegister();
    void testMergeNumericRegisters();
    void testMergeNumericRegistersDifferentTypes();
    void testCallsToPrePostFunctionsInDecorator();
    void testCallsToPrePostFunctionsInLowLevel();
};

class TransferGroupTestSuite : public test_suite {
  public:
    TransferGroupTestSuite() : test_suite("TransferGroup class test suite") {
      boost::shared_ptr<TransferGroupTest> transferGroupTest(new TransferGroupTest);

      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testAdding, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testLogicalNameMappedRegister, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testMergeNumericRegisters, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testMergeNumericRegistersDifferentTypes, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testCallsToPrePostFunctionsInDecorator, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testCallsToPrePostFunctionsInLowLevel, transferGroupTest) );
    }
};

bool init_unit_test(){
  framework::master_test_suite().p_name.value = "TransferGroup class test suite";
  framework::master_test_suite().add(new TransferGroupTestSuite());

  return true;
}

void TransferGroupTest::testAdding() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("DUMMYD3");

  BufferingRegisterAccessor<int> a1 = device.getBufferingRegisterAccessor<int>("ADC","AREA_DMAABLE");
  BufferingRegisterAccessor<int> a2 = device.getBufferingRegisterAccessor<int>("ADC","AREA_DMAABLE");
  BufferingRegisterAccessor<int> a3 = device.getBufferingRegisterAccessor<int>("BOARD","WORD_STATUS");
  BufferingRegisterAccessor<unsigned int> a4 = device.getBufferingRegisterAccessor<unsigned int>("ADC","AREA_DMAABLE");

  // slightly redundant to do this test here, this is just a control test still independent of the TransferGroup
  a1[0] = 42;
  a2[0] = 120;
  a3[0] = 123;
  a4[0] = 456;
  BOOST_CHECK(a1[0] == 42);
  a1.write();
  a3.write();
  a3[0] = 654;
  BOOST_CHECK(a2[0] == 120);
  BOOST_CHECK(a3[0] == 654);
  BOOST_CHECK(a4[0] == 456);
  a2.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 654);
  BOOST_CHECK(a4[0] == 456);
  a3.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 456);
  a4.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(a1);
  BOOST_CHECK(!group.isReadOnly());
  group.addAccessor(a2);
  BOOST_CHECK(group.isReadOnly());
  group.addAccessor(a3);
  group.addAccessor(a4);
  BOOST_CHECK(group.isReadOnly());

  // check if adding an accessor to another group throws an exception
  TransferGroup group2;
  try {
    group2.addAccessor(a1);
    BOOST_ERROR("Exception expected!");
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::WRONG_PARAMETER);
  }

  // check that reading and writing the accessors which are part of the group throws
  try {
    a1.read();
    BOOST_ERROR("Exception expected!");
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::NOT_IMPLEMENTED);
  }
  try {
    a1.write();
    BOOST_ERROR("Exception expected!");
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::NOT_IMPLEMENTED);
  }
  try {
    a3.read();
    BOOST_ERROR("Exception expected!");
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::NOT_IMPLEMENTED);
  }
  try {
    a4.write();
    BOOST_ERROR("Exception expected!");
  }
  catch(DeviceException &e) {
    BOOST_CHECK(e.getID() == DeviceException::NOT_IMPLEMENTED);
  }

  // during the replace operation, user buffers will be reset, if a replacement
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 0);    // this one was replaced
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);

  // Writing to the register accessor (cooked) buffers should not influence the other accessors in the group.
  a1[0] = 333;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 0);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a2[0] = 666;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a3[0] = 999;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 42);
  a4[0] = 111;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 111);

  device.close();

}

void TransferGroupTest::testLogicalNameMappedRegister() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1, target2;

  device.open("LMAP0");
  BufferingRegisterAccessor<int> a[6];
  a[0].replace(device.getBufferingRegisterAccessor<int>("","SingleWord"));
  a[1].replace(device.getBufferingRegisterAccessor<int>("","FullArea"));
  a[2].replace(device.getBufferingRegisterAccessor<int>("","PartOfArea"));
  a[3].replace(device.getBufferingRegisterAccessor<int>("","Channel3"));
  a[4].replace(device.getBufferingRegisterAccessor<int>("","Channel4"));
  a[5].replace(device.getBufferingRegisterAccessor<int>("","Constant"));

  // obtain the private pointers to the implementation of the accessor
  boost::shared_ptr< NDRegisterAccessor<int> > impl[6];
  for(int i=0; i<6; i++) {
    impl[i] = a[i].*accessPrivateData::stowed<BufferingRegisterAccessor_int_impl>::value;
  }

  // somewhat redundant check: underlying hardware accessors are different for all accessors
  for(int i=0; i<6; i++) {
    BOOST_CHECK( impl[i]->getHardwareAccessingElements().size() == 1 );
    for(int k=i+1; k<6; k++) {
      BOOST_CHECK( impl[i]->getHardwareAccessingElements()[0] != impl[k]->getHardwareAccessingElements()[0] );
    }
  }

  // add accessors to the transfer group
  TransferGroup group;
  for(int i=0; i<6; i++) {
    group.addAccessor(a[i]);
  }

  // now some accessors share the same underlying accessor
  BOOST_CHECK( impl[3]->getHardwareAccessingElements()[0] == impl[4]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[1]->getHardwareAccessingElements()[0] == impl[2]->getHardwareAccessingElements()[0] );

  // the others are still different
  BOOST_CHECK( impl[0]->getHardwareAccessingElements()[0] != impl[1]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[0]->getHardwareAccessingElements()[0] != impl[3]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[0]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[1]->getHardwareAccessingElements()[0] != impl[3]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[1]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0] );
  BOOST_CHECK( impl[3]->getHardwareAccessingElements()[0] != impl[5]->getHardwareAccessingElements()[0] );

  // write some stuff to the registers via the target device
  // Note: there is only one DMA area in the PCIE dummy which are shared by the registers accessed by t2 and t3. We
  //       therefore cannot test those register at the same time!
  target1.open("PCIE2");
  target2.open("PCIE3");
  BufferingRegisterAccessor<int> t1 = target1.getBufferingRegisterAccessor<int>("","BOARD.WORD_USER");
  BufferingRegisterAccessor<int> t2 = target1.getBufferingRegisterAccessor<int>("","ADC.AREA_DMAABLE");
  TwoDRegisterAccessor<int> t3 = target2.getTwoDRegisterAccessor<int>("TEST","NODMA");

  t1 = 120;
  t1.write();
  for(unsigned int i=0; i<t2.getNumberOfElements(); i++) {
    t2[i] = 67890 + 66*(signed)i;
  }
  t2.write();

  // read it back via the transfer group
  group.read();

  BOOST_CHECK( a[0] == 120 );

  BOOST_CHECK( t2.getNumberOfElements() == a[1].getNumberOfElements() );
  for(unsigned int i=0; i<t2.getNumberOfElements(); i++) {
    BOOST_CHECK( a[1][i] == 67890 + 66*(signed)i );
  }

  BOOST_CHECK( a[2].getNumberOfElements() == 20 );
  for(unsigned int i=0; i<a[2].getNumberOfElements(); i++) {
    BOOST_CHECK( a[2][i] == 67890 + 66*(signed)(i+10) );
  }

  BOOST_CHECK( a[5] == 42 );

  // write something to the multiplexed 2d register
  for(unsigned int i=0; i<t3.getNumberOfDataSequences(); i++) {
    for(unsigned int k=0; k<t3[i].size(); k++) {
      t3[i][k] = (signed)( i*10 + k );
    }
  }
  t3.write();

  // read it back via transfer group
  group.read();

  BOOST_CHECK( a[3].getNumberOfElements() == t3[3].size() );
  for(unsigned int i=0; i<a[3].getNumberOfElements(); i++) {
    BOOST_CHECK( a[3][i] == 3*10 + (signed)i );
  }

  BOOST_CHECK( a[4].getNumberOfElements() == t3[4].size() );
  for(unsigned int i=0; i<a[4].getNumberOfElements(); i++) {
    BOOST_CHECK( a[4][i] == 4*10 + (signed)i );
  }

  // check that writing to the group fails (has read-only elements)
  BOOST_CHECK_THROW( group.write(), DeviceException );
  try {
    group.write();
  }
  catch(DeviceException &e) {
    BOOST_CHECK( e.getID() == DeviceException::REGISTER_IS_READ_ONLY );
  }

}

void TransferGroupTest::testMergeNumericRegisters() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux1 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_1");
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // create the same register accessors again, so we have a second set not part of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux1b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_1");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // obtain the private pointers to the implementation of the accessor
  auto mux0i = mux0.*accessPrivateData::stowed<BufferingRegisterAccessor_int_impl>::value;
  auto mux1i = mux1.*accessPrivateData::stowed<BufferingRegisterAccessor_int_impl>::value;
  auto mux2i = mux2.*accessPrivateData::stowed<BufferingRegisterAccessor_int_impl>::value;
  auto mux3i = mux3.*accessPrivateData::stowed<BufferingRegisterAccessor_int_impl>::value;

  // check that all underlying raw accessors are still different
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux1i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux1i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux1i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux2i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );

  // check that the underlying raw accessors have the right address range
  NumericAddressedLowLevelTransferElement *llelem;  // operator ->* does not work on a shared_ptr
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux0i->getHardwareAccessingElements()[0]).get();
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x20);
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux1i->getHardwareAccessingElements()[0]).get();
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x24);
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux2i->getHardwareAccessingElements()[0]).get();
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x28);
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux3i->getHardwareAccessingElements()[0]).get();
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x2C);
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 4);

  // add accessors to the transfer group. The accessors are intentionally added out of order to check if the behaviour
  // is also correct in that case
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux2);
  group.addAccessor(mux1);
  group.addAccessor(mux3);

  // check that all underlying raw accessors are now all the same
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux1i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux3i->getHardwareAccessingElements()[0] );

  // check that the underlying raw accessor have the right address range
  llelem = boost::static_pointer_cast<NumericAddressedLowLevelTransferElement>(mux0i->getHardwareAccessingElements()[0]).get();
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_startAddress>::value == 0x20);
  BOOST_CHECK( llelem->*accessPrivateData::stowed<NumericAddressedLowLevelTransferElement_numberOfBytes>::value == 16);

  // check that reading and writing works
  mux0 = 42;
  mux1 = 120;
  mux2 = 84;
  mux3 = 240;
  group.write();

  mux0b.read();
  BOOST_CHECK(mux0b == 42);
  mux1b.read();
  BOOST_CHECK(mux1b == 120);
  mux2b.read();
  BOOST_CHECK(mux2b == 84);
  mux3b.read();
  BOOST_CHECK(mux3b == 240);

  mux0b = 123;
  mux0b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 120 );
  BOOST_CHECK( mux2 == 84 );
  BOOST_CHECK( mux3 == 240 );

  mux1b = 234;
  mux1b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 84 );
  BOOST_CHECK( mux3 == 240 );

  mux2b = 345;
  mux2b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 345 );
  BOOST_CHECK( mux3 == 240 );

  mux3b = 456;
  mux3b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 345 );
  BOOST_CHECK( mux3 == 456 );

}

void TransferGroupTest::testMergeNumericRegistersDifferentTypes() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses
  auto mux0 = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_0");
  auto mux1 = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_1");
  auto mux2 = device.getScalarRegisterAccessor<int32_t>("/ADC/WORD_CLK_MUX_2",0,{AccessMode::raw});
  auto mux3 = device.getScalarRegisterAccessor<int64_t>("/ADC/WORD_CLK_MUX_3");

  // create the same register accessors again, so we have a second set not part of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_0");
  auto mux1b = device.getScalarRegisterAccessor<uint16_t>("/ADC/WORD_CLK_MUX_1");
  auto mux2b = device.getScalarRegisterAccessor<int32_t>("/ADC/WORD_CLK_MUX_2",0,{AccessMode::raw});
  auto mux3b = device.getScalarRegisterAccessor<int64_t>("/ADC/WORD_CLK_MUX_3");

  // obtain the private pointers to the implementation of the accessor
  auto mux0i = mux0.*accessPrivateData::stowed<BufferingRegisterAccessor_uint16_impl>::value;
  auto mux1i = mux1.*accessPrivateData::stowed<BufferingRegisterAccessor_uint16_impl>::value;
  auto mux2i = mux2.*accessPrivateData::stowed<BufferingRegisterAccessor_int32_impl>::value;
  auto mux3i = mux3.*accessPrivateData::stowed<BufferingRegisterAccessor_int64_impl>::value;

  // check that all underlying raw accessors are still different
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux1i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux1i->getHardwareAccessingElements()[0] != mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux1i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux2i->getHardwareAccessingElements()[0] != mux3i->getHardwareAccessingElements()[0] );

  // add accessors to the transfer group. The accessors are intentionally added out of order to check if the behaviour
  // is also correct in that case
  TransferGroup group;
  group.addAccessor(mux2);
  group.addAccessor(mux1);
  group.addAccessor(mux3);
  group.addAccessor(mux0);

  // check that all underlying raw accessors are now all the same
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux1i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux2i->getHardwareAccessingElements()[0] );
  BOOST_CHECK( mux0i->getHardwareAccessingElements()[0] == mux3i->getHardwareAccessingElements()[0] );

  // also check that all high-level implementations are still the same as previously
  BOOST_CHECK( mux0i == mux0.*accessPrivateData::stowed<BufferingRegisterAccessor_uint16_impl>::value );
  BOOST_CHECK( mux1i == mux1.*accessPrivateData::stowed<BufferingRegisterAccessor_uint16_impl>::value );
  BOOST_CHECK( mux2i == mux2.*accessPrivateData::stowed<BufferingRegisterAccessor_int32_impl>::value );
  BOOST_CHECK( mux3i == mux3.*accessPrivateData::stowed<BufferingRegisterAccessor_int64_impl>::value );

  // check that reading and writing works
  mux0 = 42;
  mux1 = 120;
  mux2 = 84;
  mux3 = 240;
  group.write();

  mux0b.read();
  BOOST_CHECK(mux0b == 42);
  mux1b.read();
  BOOST_CHECK(mux1b == 120);
  mux2b.read();
  BOOST_CHECK(mux2b == 84);
  mux3b.read();
  BOOST_CHECK(mux3b == 240);

  mux0b = 123;
  mux0b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 120 );
  BOOST_CHECK( mux2 == 84 );
  BOOST_CHECK( mux3 == 240 );

  mux1b = 234;
  mux1b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 84 );
  BOOST_CHECK( mux3 == 240 );

  mux2b = 345;
  mux2b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 345 );
  BOOST_CHECK( mux3 == 240 );

  mux3b = 456;
  mux3b.write();
  group.read();
  BOOST_CHECK( mux0 == 123 );
  BOOST_CHECK( mux1 == 234 );
  BOOST_CHECK( mux2 == 345 );
  BOOST_CHECK( mux3 == 456 );

}

template<typename T>
struct CountingDecorator : NDRegisterAccessorDecorator<T> {

    // if fakeLowLevel is set to true, the decorator will pretend to be the low-level TransferElement.
    CountingDecorator( const boost::shared_ptr<mtca4u::TransferElement> &target,
                       bool _fakeLowLevel = false )
    : NDRegisterAccessorDecorator<T>(boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(target)),
      fakeLowLevel(_fakeLowLevel)
    {
      assert(boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(target) != nullptr);    // a bit late but better than nothing...
      this->_name = "CD:"+this->_name;
    }

    void doPreRead() override {
      nPreRead++;
      NDRegisterAccessorDecorator<T>::doPreRead();
    }

    void doPostRead() override {
      nPostRead++;
      NDRegisterAccessorDecorator<T>::doPostRead();
    }

    void doPreWrite() override {
      nPreWrite++;
      NDRegisterAccessorDecorator<T>::doPreWrite();
    }

    void doPostWrite() override {
      nPostWrite++;
      NDRegisterAccessorDecorator<T>::doPostWrite();
    }

    void doReadTransfer() override {
      nRead++;
      NDRegisterAccessorDecorator<T>::doReadTransfer();
    }

    bool doReadTransferNonBlocking() override {
      nReadNonBlocking++;
      return NDRegisterAccessorDecorator<T>::doReadTransferNonBlocking();
    }

    bool doReadTransferLatest() override {
      nReadLatest++;
      return NDRegisterAccessorDecorator<T>::doReadTransferLatest();
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
      nWrite++;
      return NDRegisterAccessorDecorator<T>::doWriteTransfer(versionNumber);
    }

    std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
      if(fakeLowLevel) {
        return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
      }
      else {
        return NDRegisterAccessorDecorator<T>::getHardwareAccessingElements();
      }
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      if(fakeLowLevel) return;
      if(_target->mayReplaceOther(newElement)) {
        _target = boost::static_pointer_cast< NDRegisterAccessor<T> >(newElement);
      }
      else {
        _target->replaceTransferElement(newElement);
      }
    }

    std::list< boost::shared_ptr<TransferElement> > getInternalElements() override {
      if(fakeLowLevel) {
        return {};
      }
      else {
        return NDRegisterAccessorDecorator<T>::getInternalElements();
      }
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &other) const override {
      auto casted = boost::dynamic_pointer_cast<CountingDecorator<T> const>(other);
      if(!casted) return false;
      if(_target == casted->_target) return true;
      if(_target->mayReplaceOther(casted->_target)) return true;
      return false;
    }

    void resetCounters() {
      nPreRead = 0;
      nPostRead = 0;
      nPreWrite = 0;
      nPostWrite = 0;
      nRead = 0;
      nReadNonBlocking = 0;
      nReadLatest = 0;
      nWrite = 0;
    }

    bool fakeLowLevel;
    size_t nPreRead{0};
    size_t nPostRead{0};
    size_t nPreWrite{0};
    size_t nPostWrite{0};
    size_t nRead{0};
    size_t nReadNonBlocking{0};
    size_t nReadLatest{0};
    size_t nWrite{0};

    using NDRegisterAccessorDecorator<T>::_target;

};

void TransferGroupTest::testCallsToPrePostFunctionsInDecorator() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses, one of the registers is in the group two times
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux0_2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // decorate the accessors which we will put into the transfer group, so we can count how often the functions are called
  auto mux0d = boost::make_shared<CountingDecorator<int>>(mux0.getHighLevelImplElement());
  auto mux0_2d = boost::make_shared<CountingDecorator<int>>(mux0_2.getHighLevelImplElement());
  auto mux2d = boost::make_shared<CountingDecorator<int>>(mux2.getHighLevelImplElement());
  auto mux3d = boost::make_shared<CountingDecorator<int>>(mux3.getHighLevelImplElement());

  // place the decorated registers inside the abstractors
  mux0.replace(mux0d);
  mux0_2.replace(mux0_2d);
  mux2.replace(mux2d);
  mux3.replace(mux3d);

  // create the same register accessors again, so we have a second set not part of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  BOOST_CHECK( mux0d->_target != mux0_2d->_target );
  BOOST_CHECK( mux0.getHighLevelImplElement() == mux0d );
  BOOST_CHECK( mux0_2.getHighLevelImplElement() == mux0_2d );

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux0_2);
  group.addAccessor(mux2);
  group.addAccessor(mux3);

  BOOST_CHECK( mux0.getHighLevelImplElement() == mux0d );
  BOOST_CHECK( mux0_2.getHighLevelImplElement() != mux0_2d );
  BOOST_CHECK( boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(mux0_2.getHighLevelImplElement()) != nullptr );

  // write some data to the registers (without the TransferGroup)
  mux0b = 18;
  mux0b.write();
  mux2b = 22;
  mux2b.write();
  mux3b = 23;
  mux3b.write();

  // read through transfer group
  group.read();

  BOOST_CHECK_EQUAL( (int)mux0, 18 );
  BOOST_CHECK_EQUAL( (int)mux0_2, 18 );

  // we don't know which of the accessors has been eliminated (and this is actually a random choice at runtime)
  BOOST_CHECK( (mux0d->nPreRead == 1 && mux0_2d->nPreRead == 0) || (mux0d->nPreRead == 0 && mux0_2d->nPreRead == 1) );
  if(mux0d->nPreRead == 1) {
    BOOST_CHECK_EQUAL( mux0d->nPostRead, 1 );
    BOOST_CHECK_EQUAL( mux0_2d->nPreRead, 0 );
    BOOST_CHECK_EQUAL( mux0_2d->nPostRead, 0 );
  }
  else {
    BOOST_CHECK_EQUAL( mux0_2d->nPostRead, 1 );
    BOOST_CHECK_EQUAL( mux0d->nPreRead, 0 );
    BOOST_CHECK_EQUAL( mux0d->nPostRead, 0 );
  }
  BOOST_CHECK_EQUAL( mux0d->nRead, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nRead, 0 );
  BOOST_CHECK_EQUAL( mux0d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux0d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux0d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux0d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux0d->nWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nWrite, 0 );

  BOOST_CHECK_EQUAL( (int)mux2, 22 );
  BOOST_CHECK_EQUAL( mux2d->nPreRead, 1 );
  BOOST_CHECK_EQUAL( mux2d->nPostRead, 1 );
  BOOST_CHECK_EQUAL( mux2d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux2d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux2d->nRead, 0 );
  BOOST_CHECK_EQUAL( mux2d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux2d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux2d->nWrite, 0 );

  BOOST_CHECK_EQUAL( (int)mux3, 23 );
  BOOST_CHECK_EQUAL( mux3d->nPreRead, 1 );
  BOOST_CHECK_EQUAL( mux3d->nPostRead, 1 );
  BOOST_CHECK_EQUAL( mux3d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux3d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux3d->nRead, 0 );
  BOOST_CHECK_EQUAL( mux3d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux3d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux3d->nWrite, 0 );

  mux0d->resetCounters();
  mux0_2d->resetCounters();
  mux2d->resetCounters();
  mux3d->resetCounters();

  // write through transfer group is not possible, since it is read-only
  mux0 = 24;
  mux0_2 = 24;
  mux2 = 30;
  mux3 = 33;
  try {
    group.write();
    BOOST_ERROR("Exception expected.");
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK(e.getID() == mtca4u::DeviceException::REGISTER_IS_READ_ONLY);
  }
}

void TransferGroupTest::testCallsToPrePostFunctionsInLowLevel() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux0_2 = mux0;  // make duplicate of one accessor
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // decorate the accessors which we will put into the transfer group, so we can count how often the functions are called
  auto mux0d = boost::make_shared<CountingDecorator<int>>(mux0.getHighLevelImplElement(), true);
  auto mux0_2d = boost::make_shared<CountingDecorator<int>>(mux0_2.getHighLevelImplElement(), true);
  auto mux2d = boost::make_shared<CountingDecorator<int>>(mux2.getHighLevelImplElement(), true);
  auto mux3d = boost::make_shared<CountingDecorator<int>>(mux3.getHighLevelImplElement(), true);

  // decorate another time
  auto mux0d2 = boost::make_shared<CountingDecorator<int>>(mux0d);
  auto mux0_2d2 = boost::make_shared<CountingDecorator<int>>(mux0_2d);
  auto mux2d2 = boost::make_shared<CountingDecorator<int>>(mux2d);
  auto mux3d2 = boost::make_shared<CountingDecorator<int>>(mux3d);

  // place the decorated registers inside the abstractors
  mux0.replace(mux0d2);
  mux0_2.replace(mux0_2d2);
  mux2.replace(mux2d2);
  mux3.replace(mux3d2);

  // create the same register accessors again, so we have a second set not part of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  BOOST_CHECK( mux0d->_target == mux0_2d->_target );
  BOOST_CHECK( mux0d2->_target == mux0d );
  BOOST_CHECK( mux0_2d2->_target == mux0_2d );
  BOOST_CHECK( mux2d2->_target == mux2d );
  BOOST_CHECK( mux3d2->_target == mux3d );

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux0_2);
  group.addAccessor(mux2);
  group.addAccessor(mux3);

  BOOST_CHECK( mux0d->_target == mux0_2d->_target );
  BOOST_CHECK( boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(mux0_2.getHighLevelImplElement()) != nullptr );
  BOOST_CHECK( mux2d2->_target == mux2d );
  BOOST_CHECK( mux3d2->_target == mux3d );

  // write some data to the registers (without the TransferGroup)
  mux0b = 18;
  mux0b.write();
  mux2b = 22;
  mux2b.write();
  mux3b = 23;
  mux3b.write();

  // read through transfer group
  group.read();

  BOOST_CHECK_EQUAL( (int)mux0, 18 );
  BOOST_CHECK_EQUAL( (int)mux0_2, 18 );

  // we don't know which of the accessors has been eliminated (and this is actually a random choice at runtime)
  BOOST_CHECK( (mux0d->nRead == 1 && mux0_2d->nRead == 0) || (mux0d->nRead == 0 && mux0_2d->nRead == 1) );
  if(mux0d->nRead == 1) {
    BOOST_CHECK_EQUAL( mux0d->nPreRead, 1 );
    BOOST_CHECK_EQUAL( mux0d->nPostRead, 1 );
    BOOST_CHECK_EQUAL( mux0_2d->nPreRead, 0 );
    BOOST_CHECK_EQUAL( mux0_2d->nPostRead, 0 );
  }
  else {
    BOOST_CHECK_EQUAL( mux0_2d->nPreRead, 1 );
    BOOST_CHECK_EQUAL( mux0_2d->nPostRead, 1 );
    BOOST_CHECK_EQUAL( mux0d->nPreRead, 0 );
    BOOST_CHECK_EQUAL( mux0d->nPostRead, 0 );
  }
  BOOST_CHECK_EQUAL( mux0d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux0d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux0d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux0d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux0d->nWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux0_2d->nWrite, 0 );

  BOOST_CHECK_EQUAL( (int)mux2, 22 );
  BOOST_CHECK_EQUAL( mux2d->nPreRead, 1 );
  BOOST_CHECK_EQUAL( mux2d->nPostRead, 1 );
  BOOST_CHECK_EQUAL( mux2d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux2d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux2d->nRead, 1 );
  BOOST_CHECK_EQUAL( mux2d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux2d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux2d->nWrite, 0 );

  BOOST_CHECK_EQUAL( (int)mux3, 23 );
  BOOST_CHECK_EQUAL( mux3d->nPreRead, 1 );
  BOOST_CHECK_EQUAL( mux3d->nPostRead, 1 );
  BOOST_CHECK_EQUAL( mux3d->nPreWrite, 0 );
  BOOST_CHECK_EQUAL( mux3d->nPostWrite, 0 );
  BOOST_CHECK_EQUAL( mux3d->nRead, 1 );
  BOOST_CHECK_EQUAL( mux3d->nReadNonBlocking, 0 );
  BOOST_CHECK_EQUAL( mux3d->nReadLatest, 0 );
  BOOST_CHECK_EQUAL( mux3d->nWrite, 0 );

  mux0d->resetCounters();
  mux0_2d->resetCounters();
  mux2d->resetCounters();
  mux3d->resetCounters();

  // write through transfer group
  /// @todo FIXME transfer group should become read-only in this scenario!!!
  mux0 = 24;
  mux0_2 = 24;
  mux2 = 30;
  mux3 = 33;
  try {
    group.write();
    BOOST_ERROR("Exception expected.");
  }
  catch(mtca4u::DeviceException &e) {
    BOOST_CHECK(e.getID() == mtca4u::DeviceException::REGISTER_IS_READ_ONLY);
  }

}
