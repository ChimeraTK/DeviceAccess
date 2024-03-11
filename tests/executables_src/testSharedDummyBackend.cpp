// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include "BackendFactory.h"
#include "Device.h"
#include "ProcessManagement.h"
#include "SharedDummyBackend.h"
#include "sharedDummyHelpers.h"
#include "Utilities.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

  using namespace ChimeraTK;
  using namespace boost::unit_test_framework;

  static TestLocker testLocker("shareddummyTest.dmap");

  struct TestFixture {
    bool testRegisterNotInCatalogue(const std::string& registerPath) {
      // Also get the backend to test the catalogue
      auto backendInstance = BackendFactory::getInstance().createBackend("SHDMEMDEV");

      auto catalogue = backendInstance->getRegisterCatalogue();

      // the register is not in the iterable catalogue (might be a hidden register though)
      bool found = false;
      for(auto& info : catalogue) {
        if(info.getRegisterName() == registerPath) {
          found = true;
          break;
        }
      }

      return !found;
    }
  };

  BOOST_AUTO_TEST_SUITE(SharedDummyBackendTestSuite)

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testOpenClose) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.open();
    BOOST_CHECK(dev.isOpened());
    // you must always be able to call open  and close again
    dev.open();
    BOOST_CHECK(dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
  }

  /********************************************************************************************************************/

  BOOST_AUTO_TEST_CASE(testReadWrite) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());

    // Write/read some values to/from the shared memory
    ChimeraTK::OneDRegisterAccessor<int> processVars11 = dev.getOneDRegisterAccessor<int>("FEATURE1/AREA");
    int n = 0;
    std::generate(processVars11.begin(), processVars11.end(), [n]() mutable { return n++; });
    processVars11.write();
    processVars11.read();

    ChimeraTK::OneDRegisterAccessor<int> processVars23 = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA3");
    n = 0;
    std::generate(processVars23.begin(), processVars23.end(), [n]() mutable { return n++; });
    processVars23.write();
    processVars23.read();

    // Write to memory and check values mirrored by another process
    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite21 = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    n = 0;
    std::generate(processVarsWrite21.begin(), processVarsWrite21.end(), [n]() mutable { return n++; });
    processVarsWrite21.write();

    // start second accessing application
    BOOST_CHECK(!std::system("./testSharedDummyBackendExt "
                             "--run_test=SharedDummyBackendTestSuite/testReadWrite"));

    // Check if values have been written back by the other application
    ChimeraTK::OneDRegisterAccessor<int> processVarsRead = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
    processVarsRead.read();

    BOOST_CHECK(static_cast<std::vector<int>>(processVarsWrite21) == static_cast<std::vector<int>>(processVarsRead));
    dev.close();
  }

  /********************************************************************************************************************/

  BOOST_FIXTURE_TEST_CASE(testWriteToReadOnly, TestFixture) {
    setDMapFilePath("shareddummyTest.dmap");
    Device dev;
    dev.open("SHDMEMDEV");

    ScalarRegisterAccessor<int> roRegisterOne{dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_1")};
    ScalarRegisterAccessor<int> roRegisterTwo_dw{
        dev.getScalarRegisterAccessor<int>("WORD_READ_ONLY_2.DUMMY_WRITEABLE")};

    BOOST_CHECK(roRegisterOne.isReadOnly());
    BOOST_CHECK(!roRegisterOne.isWriteable());
    BOOST_CHECK(!roRegisterTwo_dw.isReadOnly());
    BOOST_CHECK(roRegisterTwo_dw.isWriteable());

    BOOST_CHECK(testRegisterNotInCatalogue("WORD_READ_ONLY_2.DUMMY_WRITEABLE"));

    BOOST_CHECK_THROW(roRegisterOne.write(), ChimeraTK::logic_error);

    roRegisterOne = 0;
    roRegisterTwo_dw = 25;
    roRegisterTwo_dw.write();

    // Start second accessing application
    // This is complementary and has a writeable accessor to WORD_READ_ONLY_1
    // to which it mirrors the value of the second register
    BOOST_CHECK(!std::system("./testSharedDummyBackendExt "
                             "--run_test=SharedDummyBackendTestSuite/testWriteToReadOnly"));

    roRegisterOne.read();
    BOOST_CHECK_EQUAL(roRegisterTwo_dw, roRegisterOne);

    dev.close();
  }

  /********************************************************************************************************************/

  BOOST_FIXTURE_TEST_CASE(testCreateBackend, TestFixture) {
    setDMapFilePath("shareddummyTest.dmap");
    auto backendInst1 = BackendFactory::getInstance().createBackend("SHDMEMDEV");
    auto backendInst2 = BackendFactory::getInstance().createBackend("SHDMEMDEV");
    auto backendInst3 = BackendFactory::getInstance().createBackend("SHDMEMDEV2");

    BOOST_CHECK(backendInst1.get() == backendInst2.get());
    BOOST_CHECK(backendInst3.get() != backendInst2.get());
  }

} // anonymous namespace

/**********************************************************************************************************************/
BOOST_AUTO_TEST_SUITE_END()
