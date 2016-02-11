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

  BufferingRegisterAccessor<int> a1 = device.getBufferingRegisterAccessor<int>("ADC","AREA_DMA_VIA_DMA");
  BufferingRegisterAccessor<int> a2 = device.getBufferingRegisterAccessor<int>("ADC","AREA_DMA_VIA_DMA");
  BufferingRegisterAccessor<int> a3 = device.getBufferingRegisterAccessor<int>("ADC","AREA_DMAABLE");
  BufferingRegisterAccessor<double> a4 = device.getBufferingRegisterAccessor<double>("ADC","AREA_DMA_VIA_DMA");

  TransferGroup group;
  group.addAccessor(a1);
  group.addAccessor(a2);
  group.addAccessor(a3);
  group.addAccessor(a4);

  group.read();


}
