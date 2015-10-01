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
  std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
  std::string oldtestFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH + (std::string)("Old");;
  std::string rntestFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH + (std::string)("disabled");
  std::rename(testFilePath.c_str(),rntestFilePath.c_str());
  boost::shared_ptr<DeviceBackend> testPtr = BackendFactory::getInstance().createBackend("test");//cannot open dmap file
  BOOST_CHECK(!testPtr);
  std::rename(oldtestFilePath.c_str(),testFilePath.c_str());
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),Exception); //not an existing alias.
  testPtr.reset();
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD0")); //entry in old dummies.dmap
  BOOST_CHECK(testPtr);
  testPtr.reset();
  std::rename(testFilePath.c_str(),oldtestFilePath.c_str());
  std::rename(rntestFilePath.c_str(),testFilePath.c_str());
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),Exception); //not an existing alias.
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD0")); //entry in dummiesdmap
  BOOST_CHECK(testPtr);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("FAKE1"),BackendFactoryException); //entry in dummies.dmap for unregistered device
}

