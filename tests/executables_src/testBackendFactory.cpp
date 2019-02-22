#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE BackendFactoryTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/make_shared.hpp>
#include <cstdio>

#include "BackendFactory.h"
#include "DummyBackend.h"
#include "Exception.h"

#include "DeviceAccessVersion.h"
namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

//#undef TEST_DMAP_FILE_PATH
//#define TEST_DMAP_FILE_PATH "/testDummies.dmap"

struct NewBackend : public DummyBackend {
  using DummyBackend::DummyBackend;

  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string /*host*/, std::string instance, std::list<std::string> parameters, std::string /*mapFileName*/) {
    return returnInstance<NewBackend>(instance, convertPathRelativeToDmapToAbs(parameters.front()));
  }

  // no registerer, we do it manually
};

BOOST_AUTO_TEST_SUITE(BackendFactoryTestSuite)

BOOST_AUTO_TEST_CASE(testCreateBackend) {
  BackendFactory::getInstance().setDMapFilePath("");
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"), ChimeraTK::logic_error);

  std::string testFilePath = TEST_DMAP_FILE_PATH;
  std::string oldtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "Old";
  std::string invalidtestFilePath = std::string(TEST_DMAP_FILE_PATH) + "disabled";
  BOOST_CHECK_THROW(BackendFactory::getInstance().setDMapFilePath(invalidtestFilePath), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),
      ChimeraTK::logic_error); // dmap file not found
                               // exception .
  BackendFactory::getInstance().setDMapFilePath(oldtestFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),
      ChimeraTK::logic_error); // file found but not
                               // an existing alias.
  boost::shared_ptr<DeviceBackend> testPtr;
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD0")); // entry in old dummies.dmap
  BOOST_CHECK(testPtr);
  testPtr.reset();
  BackendFactory::getInstance().setDMapFilePath(testFilePath);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("test"),
      ChimeraTK::logic_error);                                                            // not an existing
                                                                                          // alias.
  BOOST_CHECK_NO_THROW(testPtr = BackendFactory::getInstance().createBackend("DUMMYD9")); // entry in dummies.dmap
  BOOST_CHECK(testPtr);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("FAKE1"),
      ChimeraTK::logic_error); // entry in
                               // dummies.dmap for
                               // unregistered
                               // device
  boost::shared_ptr<DeviceBackend> testPtr2;
  BOOST_CHECK_NO_THROW(testPtr2 = BackendFactory::getInstance().createBackend("DUMMYD9")); // open existing backend
                                                                                           // again
  BOOST_CHECK(testPtr2 == testPtr);                                                        // must be same
}

BOOST_AUTO_TEST_CASE(testPluginMechanism) {
  // check the registation of a new backed, called NewBackend ;-)
  // Throws with the wrong version (00.18 did not have the feature yet, so its
  // safe to use it) It however is only happening when the backend is tried to
  // be instantiated because otherwise we would end up in uncatchable exceptions
  // while loading a dmap file with a broken backend.
  BOOST_CHECK_NO_THROW(ChimeraTK::BackendFactory::getInstance().registerBackendType(
      "newBackend", "", &NewBackend::createInstance, "00.18"));

  BOOST_CHECK_THROW(
      BackendFactory::getInstance().createBackend("sdm://./newBackend=goodMapFile.map"), ChimeraTK::logic_error);

  BOOST_CHECK_NO_THROW(ChimeraTK::BackendFactory::getInstance().registerBackendType(
      "newBackend", "", &NewBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION));

  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("sdm://./newBackend=goodMapFile.map"));

  BOOST_CHECK_THROW(
      ChimeraTK::BackendFactory::getInstance().loadPluginLibrary("notExisting.so"), ChimeraTK::logic_error);

  BOOST_CHECK_NO_THROW(ChimeraTK::BackendFactory::getInstance().loadPluginLibrary("./libWorkingBackend.so"));
  // check that the backend really is registered
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("(working?map=goodMapFile.map)"));
  BOOST_CHECK_NO_THROW(BackendFactory::getInstance().createBackend("sdm://./working=goodMapFile.map"));

  BOOST_CHECK_THROW(
      ChimeraTK::BackendFactory::getInstance().loadPluginLibrary("libNotRegisteringPlugin.so"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("sdm://./notRegisteringPlugin=goodMapFile.map"),
      ChimeraTK::logic_error);

  BOOST_CHECK_NO_THROW(ChimeraTK::BackendFactory::getInstance().loadPluginLibrary("./libWrongVersionBackend.so"));
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("sdm://./wrongVersionBackend=goodMapFile.map"),
      ChimeraTK::logic_error);

  BOOST_CHECK_NO_THROW(ChimeraTK::BackendFactory::getInstance().loadPluginLibrary("./libWrongVersionBackendCompat.so"));
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("sdm://./libWrongVersionBackendCompat=goodMapFile.map"),
      ChimeraTK::logic_error);

  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("(unregisteredBackend)"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(BackendFactory::getInstance().createBackend("sdm://./unregisteredBackend"), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testCreateFromUri) {
  // this has to work without dmap file
  BackendFactory::getInstance().setDMapFilePath("");

  boost::shared_ptr<DeviceBackend> testPtr;
  testPtr = BackendFactory::getInstance().createBackend("sdm://./dummy=mtcadummy.map"); // get some dummy
  // just check that something has been created. That it's the correct thing is
  // another test.
  BOOST_CHECK(testPtr);
}

BOOST_AUTO_TEST_SUITE_END()
