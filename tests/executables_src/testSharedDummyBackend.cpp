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
#include <utility>

using namespace ChimeraTK;

// Static function prototypes
//static std::string createExpectedShmName(std::string, std::string);
//static bool shm_exists(std::string);

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
    for(size_t i=0; i<processVars11.getNElements(); ++i){
      processVars11[i] = i;
    }
    processVars11.write();
    processVars11.read();

    ChimeraTK::OneDRegisterAccessor<int> processVars23
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA3");
    for(size_t i=0; i<processVars23.getNElements(); ++i){
      processVars23[i] = i;
    }
    processVars23.write();
    processVars23.read();


    // Write to memory and check values mirrored by another process
    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite21
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    for(size_t i=0; i<processVarsWrite21.getNElements(); ++i){
      processVarsWrite21[i] = i;
    }
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


//// Static helper functions
//static std::string createExpectedShmName(std::string instanceId, std::string mapFileName){
//  std::string mapFileHash{std::to_string(std::hash<std::string>{}(mapFileName))};
//  std::string instanceIdHash{std::to_string(std::hash<std::string>{}(instanceId))};
//  std::string userHash{std::to_string(std::hash<std::string>{}(getUserName()))};
//
//  return "ChimeraTK_SharedDummy_" + instanceIdHash + "_" + mapFileHash + "_" + userHash;
//}
//
//static bool shm_exists(std::string shmName){
//
//  bool result;
//
//  try{
//    boost::interprocess::managed_shared_memory shm{boost::interprocess::open_only, shmName.c_str()};
//    result =  shm.check_sanity();
//  }
//  catch(const std::exception & ex){
//    result = false;
//  }
//  return result;
//}
