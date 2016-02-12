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
};

class TransferGroupTestSuite : public test_suite {
  public:
    TransferGroupTestSuite() : test_suite("TransferGroup class test suite") {
      boost::shared_ptr<TransferGroupTest> transferGroupTest(new TransferGroupTest);

      add( BOOST_CLASS_TEST_CASE(&TransferGroupTest::testSimpleCase, transferGroupTest) );
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

}
