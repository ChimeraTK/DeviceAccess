// This test is an additional application accessing the shared memory.
// It is called from related tests, not by Ctest
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

#include "Device.h"
#include "Utilities.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <thread>

using namespace ChimeraTK;

static void interrupt_handler(int);

static bool terminationCaught = false;

BOOST_AUTO_TEST_SUITE( SharedDummyBackendTestSuite )

struct TestFixture {
  TestFixture() : argc(framework::master_test_suite().argc),
      argv(framework::master_test_suite().argv){}

  int argc;
  char **argv;
};

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
