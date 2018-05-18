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
static std::string createExpectedShmName(std::string, std::string);
static bool shm_exists(std::string);

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
    // Use hardcoded information from the dmap-file to
    // only use public interface here
    std::string instanceId{""};
    std::string mapFileName{"shareddummy.map"};

    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);

    std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string())};

    {
      Device dev;
      BOOST_CHECK(!dev.isOpened());
      dev.open("SHDMEMDEV");
      BOOST_CHECK(dev.isOpened());

      BOOST_CHECK(shm_exists(shmName));

      // Write some values to the shared memory
      ChimeraTK::OneDRegisterAccessor<int> processVarsWrite
        = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
      for(size_t i=0; i<processVarsWrite.getNElements(); ++i){
        processVarsWrite[i] = i;
      }
      processVarsWrite.write();

      //start second accessing application
      BOOST_CHECK(!std::system("../bin/testSharedDummyBackend2ndApp --run_test=SharedDummyBackendTestSuite/testReadWrite"));

      // Check if values have been written back by the other application
      ChimeraTK::OneDRegisterAccessor<int> processVarsRead
        = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
      processVarsRead.read();

      BOOST_CHECK((std::vector<int>)processVarsWrite == (std::vector<int>)processVarsRead);
      dev.close();
    }

    //Test if memory is removed
    BOOST_CHECK(!shm_exists(shmName));
}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_SUITE_END()


// Static helper functions
static std::string createExpectedShmName(std::string instanceId, std::string mapFileName){
  std::string mapFileHash{std::to_string(std::hash<std::string>{}(mapFileName))};
  std::string instanceIdHash{std::to_string(std::hash<std::string>{}(instanceId))};
  std::string userHash{std::to_string(std::hash<std::string>{}(getUserName()))};

  return "ChimeraTK_SharedDummy_" + instanceIdHash + "_" + mapFileHash + "_" + userHash;
}

static bool shm_exists(std::string shmName){

  bool result;

  try{
    boost::interprocess::managed_shared_memory shm{boost::interprocess::open_only, shmName.c_str()};
    result =  shm.check_sanity();
  }
  catch(const std::exception & ex){
    result = false;
  }
  return result;
}
