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
#include <csignal>
#include <thread>
#include <chrono>
#include <utility>

using namespace ChimeraTK;

//TODO remove
//// Static function prototypes
//static std::string createExpectedShmName(std::string, std::string);
//static bool shm_exists(std::string);

static void interrupt_handler(int);

static bool terminationCaught = false;

BOOST_AUTO_TEST_SUITE( SharedDummyBackendTestSuite )

struct TestFixture {
  TestFixture() : argc(framework::master_test_suite().argc),
      argv(framework::master_test_suite().argv){}

  int argc;
  char **argv;
};


/*********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE( testRobustnessMain, TestFixture ) {

    unsigned nIterations = 0;
    if(argc != 2){
      std::cout << "Illegal number of arguments. Test case must be called with the number of read/write cycles!"
                << std::endl;
    }
    else{
      nIterations = atoi(argv[1]);
    }

    setDMapFilePath("shareddummyTest.dmap");
    // Use hardcoded information from the dmap-file to
    // only use public interface here
    std::string instanceId{""};
    std::string mapFileName{"shareddummy.map"};

    boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);

    //TODO remove
    //std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string())};

    //FIXME Move to scope below
    bool readbackCorrect = false;
    bool waitingForResponse = true;
    const unsigned maxIncorrectIterations = 10; /* Timeout while waiting for 2nd application */
    unsigned iterations = 0;
    unsigned incorrectIterations = 0;

    {
      Device dev;
      BOOST_CHECK(!dev.isOpened());
      dev.open("SHDMEMDEV");
      BOOST_CHECK(dev.isOpened());

      //TODO remove
      //BOOST_CHECK(shm_exists(shmName));

      ChimeraTK::OneDRegisterAccessor<int> processVarsWrite
        = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
      std::vector<int> processVarsOld (processVarsWrite.getNElements(), 0);

      do{
        // Write some values to the shared memory
        for(size_t i=0; i<processVarsWrite.getNElements(); ++i){
          processVarsWrite[i] = i + iterations;
        }
        processVarsWrite.write();


        // Check if values have been written back by the other application
        ChimeraTK::OneDRegisterAccessor<int> processVarsRead
          = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
        // Read as long as the readback from last iteration has been overwritten
        do{
          processVarsRead.read();
        }
        while((std::vector<int>)processVarsRead == (std::vector<int>)processVarsOld && !waitingForResponse);

        if((std::vector<int>)processVarsWrite == (std::vector<int>)processVarsRead){
          if(waitingForResponse){
            waitingForResponse = false;
          }
          readbackCorrect = true;
        }
        else{
          readbackCorrect = false;
          if(!waitingForResponse){
            std::cout << "Corrupted data detected:" << std::endl;
            for(const auto pvr : processVarsRead){
              std::cout << "    " << pvr << std::endl;
            }
          }
        }

        if(waitingForResponse){
          incorrectIterations++;
        }
        else{
          iterations++;
        }
        processVarsOld = processVarsWrite;
      }
      while((readbackCorrect || waitingForResponse) &&
            incorrectIterations != maxIncorrectIterations && iterations != nIterations);

      BOOST_CHECK(readbackCorrect);
      std::cout << "Finished test after " << iterations
                << " of " << nIterations << " Iterations." << std::endl;

      dev.close();
    }
}


/**
 * This test case implements a second application accessing the shared memory
 * which mirrors the values to another register bar.
 * For a robustness test, it can be called with the argument "KEEP_RUNNING", so
 * that it constantly operates on the shared memory. In this case, it can be terminated
 * gracefully by sending SIGINT.
 */
BOOST_FIXTURE_TEST_CASE( testReadWrite, TestFixture ) {

    signal(SIGINT, interrupt_handler);

    bool keepRunning = false;

    if(argc == 2 && strcmp(argv[1], "KEEP_RUNING")){
      keepRunning = true;
    }

    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());

    do{
      ChimeraTK::OneDRegisterAccessor<int> processVarsRead
        = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
      for(size_t i=0; i<processVarsRead.getNElements(); ++i){
        processVarsRead[i] = i;
      }
      processVarsRead.read();

      ChimeraTK::OneDRegisterAccessor<int> processVarsWrite
        = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");

      for(size_t i=0; i<processVarsRead.getNElements(); ++i){
        processVarsWrite[i] = processVarsRead[i];
      }
      processVarsWrite.write();
    }
    while(keepRunning && !terminationCaught);
    dev.close();

}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_SUITE_END()

/**
 * Catch interrupt signal, so we can terminate the test and still
 * clean up the shared memory.
 */
static void interrupt_handler(int signal){

  std::cout << "Caught interrupt signal ("
            << signal << "). Terminating..." << std::endl;
  terminationCaught = true;
}


// FIXME Remove
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
