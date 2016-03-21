/*
 * testTransferGroup.cpp
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "TransferGroup.h"
#include "BufferingRegisterAccessor.h"
#include "Device.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

class TransferGroupTest {
  public:
    void testSimpleCase();
    void testLogicalNameMappedRegister();
};

class TransferGroupTestSuite : public test_suite {
  public:
    TransferGroupTestSuite() : test_suite("TransferGroup class test suite") {
      boost::shared_ptr<TransferGroupTest> transferGroupTest(new TransferGroupTest);

      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testSimpleCase, transferGroupTest) );
      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testLogicalNameMappedRegister, transferGroupTest) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "TransferGroup class test suite";
  framework::master_test_suite().add(new TransferGroupTestSuite());

  return NULL;
}

void TransferGroupTest::testSimpleCase() {

  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  mtca4u::Device device;

  device.open("PCIE2");

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
  group.addAccessor(a2);
  group.addAccessor(a3);
  group.addAccessor(a4);

  // check if a1 and a2 now share the same buffer, and the others not
  a1[0] = 333;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 333);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a2[0] = 666;
  BOOST_CHECK(a1[0] == 666);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a3[0] = 999;
  BOOST_CHECK(a1[0] == 666);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 42);
  a4[0] = 111;
  BOOST_CHECK(a1[0] == 666);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 111);

 // write will trigger writes of the underlying accessors in sequence, so a4 will write at last
  group.write();
  BOOST_CHECK(a1[0] == 666);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 111);
  a1[0] = 0;
  a2[0] = 0;
  a3[0] = 0;
  a4[0] = 0;

  // now read and check if values are as expected
  group.read();
  BOOST_CHECK(a1[0] == 111);
  BOOST_CHECK(a2[0] == 111);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 111);

  device.close();

}

void TransferGroupTest::testLogicalNameMappedRegister() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device, target1, target2;

  device.open("LMAP0");
  BufferingRegisterAccessor<int> a[6];
  a[0] = device.getBufferingRegisterAccessor<int>("","SingleWord");
  a[1] = device.getBufferingRegisterAccessor<int>("","FullArea");
  a[2] = device.getBufferingRegisterAccessor<int>("","PartOfArea");
  a[3] = device.getBufferingRegisterAccessor<int>("","Channel3");
  a[4] = device.getBufferingRegisterAccessor<int>("","Channel4");
  a[5] = device.getBufferingRegisterAccessor<int>("","Constant");

  // somewhat redundant check: underlying hardware accessors are different for all accessors
  for(int i=0; i<6; i++) {
    BOOST_CHECK( a[i].getHardwareAccessingElements().size() == 1 );
    for(int k=i+1; k<6; k++) {
      BOOST_CHECK( a[i].getHardwareAccessingElements()[0] != a[k].getHardwareAccessingElements()[0] );
    }
  }

  // add accessors to the transfer group
  TransferGroup group;
  for(int i=0; i<6; i++) {
    group.addAccessor(a[i]);
  }

  // now some accessors share the same underlying accessor
  BOOST_CHECK( a[3].getHardwareAccessingElements()[0] == a[4].getHardwareAccessingElements()[0] );

  // the others are still different
  BOOST_CHECK( a[0].getHardwareAccessingElements()[0] != a[1].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[0].getHardwareAccessingElements()[0] != a[3].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[0].getHardwareAccessingElements()[0] != a[5].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[1].getHardwareAccessingElements()[0] != a[2].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[1].getHardwareAccessingElements()[0] != a[3].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[1].getHardwareAccessingElements()[0] != a[5].getHardwareAccessingElements()[0] );
  BOOST_CHECK( a[3].getHardwareAccessingElements()[0] != a[5].getHardwareAccessingElements()[0] );

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
