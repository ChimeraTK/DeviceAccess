#define BOOST_TEST_MODULE BackendLoadingTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

//#include <cstdio>
//#include <boost/make_shared.hpp>

#include "BackendFactory.h"
#include "DeviceBackendException.h"
using namespace mtca4u;

BOOST_AUTO_TEST_SUITE(BackendLoadingTestSuite)

BOOST_AUTO_TEST_CASE( testBackendLoading ){
  BackendFactory::getInstance().setDMapFilePath("");

  // check that we can load backends always known to the factory,
  // but we cannot load the one coming from the plugin to exclude that we accidentally already
  // linkes the so file we want to load
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("sdm://./dummy=goodMapFile.map") );
  BOOST_CHECK_THROW( BackendFactory::getInstance().createBackend("sdm://./working=goodMapFile.map"),
		     BackendFactoryException);

  BackendFactory::getInstance().setDMapFilePath("runtimeLoading/runtimeLoading.dmap");
  // Each call of createBackend is loading the plugins. Check that this is not causing problems.
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("WORKING") );
  try{
    BackendFactory::getInstance().createBackend("WORKING");
  }catch(Exception &e){
    std::cout << "exception " << e.what() << std::endl;
  }
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("ANOTHER") );
}


BOOST_AUTO_TEST_SUITE_END()
