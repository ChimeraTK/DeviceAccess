// This test is an additional application accessing the shared memory.
// It is called from related tests called by Ctest
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;

#include "Device.h"
#include "Utilities.h"
#include <unistd.h>
#include <cstdlib>
#include <thread>

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE( SharedDummyBackendTestSuite )

BOOST_AUTO_TEST_CASE( testReadWrite ) {

    setDMapFilePath("shareddummyTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SHDMEMDEV");
    BOOST_CHECK(dev.isOpened());


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

    dev.close();

}
BOOST_AUTO_TEST_SUITE_END()
