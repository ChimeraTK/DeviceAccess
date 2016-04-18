#define BOOST_TEST_MODULE BackendFactoryTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <cstdio>

#include "BackendFactory.h"
#include "MapException.h"
using namespace mtca4u;

//#undef TEST_DMAP_FILE_PATH
//#define TEST_DMAP_FILE_PATH "/testDummies.dmap"

BOOST_AUTO_TEST_SUITE(BackendFactoryTestSuite)

BOOST_AUTO_TEST_CASE( testCreateBackend ){
  BackendFactory::getInstance().setDMapFilePath("");
  try{
      BackendFactory::getInstance().createBackend("test");
      BOOST_ERROR("BackendFactory::getInstance() with empty dmap file did not throw."); //LCOV_EXCL_LINE
  }catch(DeviceException & e){
      BOOST_CHECK(e.getID() == DeviceException::NO_DMAP_FILE);
  }

  std::string testFilePath = TEST_DMAP_FILE_PATH;
  std::string oldtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "Old";
  std::string invalidtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "disabled";
  BackendFactory::getInstance().setDMapFilePath(invalidtestFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"), LibMapException);//dmap file not found exception .
  BackendFactory::getInstance().setDMapFilePath(oldtestFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"), LibMapException); //file found but not an existing alias.
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

BOOST_AUTO_TEST_CASE( testCreateFromUri ){
  // this has to work without dmap file
  BackendFactory::getInstance().setDMapFilePath("");

  boost::shared_ptr<DeviceBackend> testPtr;
  testPtr = BackendFactory::getInstance().createBackend("sdm://./dummy=mtcadummy.map"); //get some dummy
  // just check that something has been created. That it's the correct thing is another test.
  BOOST_CHECK(testPtr);
}


BOOST_AUTO_TEST_SUITE_END()
