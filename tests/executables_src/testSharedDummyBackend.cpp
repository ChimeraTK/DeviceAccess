#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "Utilities.h"
#include "ProcessManagement.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <algorithm>
#include <utility>

using namespace ChimeraTK;


BOOST_AUTO_TEST_SUITE( SharedDummyBackendTestSuite )

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testOpenClose ) {
    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.open();
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadWrite ) {

    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());

    // Write/read some values to/from the shared memory
    ChimeraTK::OneDRegisterAccessor<int> processVars11
      = dev.getOneDRegisterAccessor<int>("FEATURE1/AREA");
    int n = 0;
    std::generate(processVars11.begin(), processVars11.end(), [n] () mutable { return n++; });
    processVars11.write();
    processVars11.read();

    ChimeraTK::OneDRegisterAccessor<int> processVars23
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA3");
    n = 0;
    std::generate(processVars23.begin(), processVars23.end(), [n] () mutable { return n++; });
    processVars23.write();
    processVars23.read();


    // Write to memory and check values mirrored by another process
    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite21
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    n = 0;
    std::generate(processVarsWrite21.begin(), processVarsWrite21.end(), [n] () mutable { return n++; });
    processVarsWrite21.write();

    //start second accessing application
    BOOST_CHECK(!std::system("../bin/testSharedDummyBackendExt --run_test=SharedDummyBackendTestSuite/testReadWrite"));

    // Check if values have been written back by the other application
    ChimeraTK::OneDRegisterAccessor<int> processVarsRead
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
    processVarsRead.read();

    BOOST_CHECK((std::vector<int>)processVarsWrite21 == (std::vector<int>)processVarsRead);
    dev.close();
}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_SUITE_END()

