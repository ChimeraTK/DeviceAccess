#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SharedDummyBackendTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include <Utilities.h>

#include <vector>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <chrono>

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


    ChimeraTK::OneDRegisterAccessor<int> processVarsWrite
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA1");
    for(size_t i=0; i<processVarsWrite.getNElements(); ++i){
      processVarsWrite[i] = i;
    }
    processVarsWrite.write();

    //start second accessing application
    BOOST_CHECK(!std::system("./bin/testSharedDummyBackendReadWrite"));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ChimeraTK::OneDRegisterAccessor<int> processVarsRead
      = dev.getOneDRegisterAccessor<int>("FEATURE2/AREA2");
    processVarsRead.read();


    BOOST_CHECK((std::vector<int>)processVarsWrite == (std::vector<int>)processVarsRead);
    dev.close();

    //TODO Test if memory is removed from /dev/shm
}

/*********************************************************************************************************************/

//BOOST_AUTO_TEST_CASE( testMayReplaceOther ) {
//
//    setDMapFilePath("subdeviceTest.dmap");
//
//    Device dev;
//    dev.open("SUBDEV1");
//    Device target;
//    target.open("TARGET1");
//
//    {
//      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
//      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
//      BOOST_CHECK(acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
//      BOOST_CHECK(acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
//    }
//
//    {
//      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
//      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1");
//      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
//      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
//    }
//
//    {
//      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
//      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
//      BOOST_CHECK(acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
//      BOOST_CHECK(acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
//    }
//
//    {
//      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1");
//      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
//      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
//      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
//    }
//
//    {
//      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
//      auto acc1_2  = dev.getScalarRegisterAccessor<int16_t>("APP.0.MY_REGISTER2");
//      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
//      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
//    }
//
//}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
