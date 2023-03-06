// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE BackendLoadingTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

//#include <cstdio>
//#include <boost/make_shared.hpp>

#include "BackendFactory.h"
#include "Exception.h"
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(BackendLoadingTestSuite)

BOOST_AUTO_TEST_CASE(testBackendLoading) {
  BackendFactory::getInstance().setDMapFilePath("");

  // check that we can load backends always known to the factory,
  // but we cannot load the one coming from the plugin to exclude that we
  // accidentally already linkes the so file we want to load
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("(dummy?map=goodMapFile.map)"));
  BOOST_CHECK_THROW(
      BackendFactory::getInstance().createBackend("(working?map=goodMapFile.map"), ChimeraTK::logic_error);

  BackendFactory::getInstance().setDMapFilePath("runtimeLoading/wrongVersionPlugin.dmap");
  // although a plugin with a wrong version was in the dmap file, the other
  // backends can still be opended
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("MY_DUMMY"));

  // Only when we access the one where loading failed we get an error
  // (not using boost throw to print excaption.what())
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("WRONG_VERSION"), ChimeraTK::logic_error);

  // Now try loading valid plugins
  BackendFactory::getInstance().setDMapFilePath("runtimeLoading/runtimeLoading.dmap");
  // Each call of createBackend is loading the plugins. Check that this is not
  // causing problems.
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("WORKING"));
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("ANOTHER"));
}

BOOST_AUTO_TEST_SUITE_END()
