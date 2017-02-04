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

  BackendFactory::getInstance().setDMapFilePath("runtimeLoading/wrongVersionPlugin.dmap");
  // although a plugin with a wrong version was in the dmap file, the other backends can
  // still be opended
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("MY_DUMMY") );

  // Only when we access the one where loading failed we get an error
  // (not using boost throw to print excaption.what())
  try{
    BackendFactory::getInstance().createBackend("WRONG_VERSION");
    // we should not reach this point if the test succeed. The line above should throw.
    BOOST_ERROR("createBackend did not throw as expected.");
  }catch( BackendFactoryException &e ){
    std::cout << "exptected exception: " << e.what() << std::endl;
  }

  // Now try loading valid plugins
  BackendFactory::getInstance().setDMapFilePath("runtimeLoading/runtimeLoading.dmap");
  // Each call of createBackend is loading the plugins. Check that this is not causing problems.
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("WORKING") );
  BOOST_CHECK_NO_THROW( BackendFactory::getInstance().createBackend("ANOTHER") );
}


BOOST_AUTO_TEST_SUITE_END()
