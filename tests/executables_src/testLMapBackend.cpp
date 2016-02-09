/*
 * testLMapBackend.cpp
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "Device.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

class LMapBackendTest {
  public:
    void testDirectReadWrite();
};

class LMapBackendTestSuite : public test_suite {
  public:
    LMapBackendTestSuite() : test_suite("LogicalNameMappingBackend test suite") {
      boost::shared_ptr<LMapBackendTest> lMapBackendTest(new LMapBackendTest);

      add( BOOST_CLASS_TEST_CASE(&LMapBackendTest::testDirectReadWrite, lMapBackendTest) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "LogicalNameMappingBackend test suite";
  framework::master_test_suite().add(new LMapBackendTestSuite());

  return NULL;
}

void LMapBackendTest::testDirectReadWrite() {

  BackendFactory::getInstance().setDMapFilePath("logicalnamemap.dmap");
  mtca4u::Device device;

  device.open("LMAP0");

}
