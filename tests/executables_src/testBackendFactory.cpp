#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <cstdio>

#include "BackendFactory.h"

//#undef TEST_DMAP_FILE_PATH
//#define TEST_DMAP_FILE_PATH "/testDummies.dmap"

using namespace mtca4u;

class BackendFactoryTest
{
public:
  static void testCreateBackend();
};

class BackendFactoryTestSuite : public test_suite {
  public:
    BackendFactoryTestSuite() : test_suite("Utilities test suite") {
      BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
      boost::shared_ptr<BackendFactoryTest> backendFactoryTest(new BackendFactoryTest);
      add(BOOST_TEST_CASE(BackendFactoryTest::testCreateBackend));
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "Utilities test suite";
  framework::master_test_suite().add(new BackendFactoryTestSuite);
  return NULL;
}


void BackendFactoryTest::testCreateBackend() {
  setenv(ENV_VAR_DMAP_FILE, "/usr/local/include/dummies.dmap", 1);
  std::string testFilePath = TEST_DMAP_FILE_PATH;
  std::string oldtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "Old";
  std::string invalidtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "disabled";
  BackendFactory::getInstance().setDMapFilePath(invalidtestFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),BackendFactoryException);//file does not exist. alias cannot be found in any other file.
  BackendFactory::getInstance().setDMapFilePath(oldtestFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),BackendFactoryException); //file found but not an existing alias.
  boost::shared_ptr<DeviceBackend> testPtr;
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD0")); //entry in old dummies.dmap
  BOOST_CHECK(testPtr);
  testPtr.reset();
  BackendFactory::getInstance().setDMapFilePath(testFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),Exception); //not an existing alias.
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD9")); //entry in dummies.dmap
  BOOST_CHECK(testPtr);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("FAKE1"),BackendFactoryException); //entry in dummies.dmap for unregistered device
}

