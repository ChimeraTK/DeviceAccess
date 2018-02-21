/*
 * testSubdeviceBackend.cpp
 *
 *  Created on: Jan 31, 2018
 *      Author: Martin Hierholzer
 */

#define BOOST_TEST_MODULE SubdeviceBackendTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE( SubdeviceBackendTestSuite )

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testOpenClose ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    BOOST_CHECK(!dev.isOpened());
    dev.open("SUBDEV1");
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());
    dev.open();
    BOOST_CHECK(dev.isOpened());
    dev.close();
    BOOST_CHECK(!dev.isOpened());

}

/// @todo Test exception handling!

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testMayReplaceOther ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    {
      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
      BOOST_CHECK(acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
      BOOST_CHECK(acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
    }

    {
      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1");
      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
    }

    {
      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
      BOOST_CHECK(acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
      BOOST_CHECK(acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
    }

    {
      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1");
      auto acc1_2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
    }

    {
      auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2");
      auto acc1_2  = dev.getScalarRegisterAccessor<int16_t>("APP.0.MY_REGISTER2");
      BOOST_CHECK(!acc1.getHighLevelImplElement()->mayReplaceOther(acc1_2.getHighLevelImplElement()));
      BOOST_CHECK(!acc1_2.getHighLevelImplElement()->mayReplaceOther(acc1.getHighLevelImplElement()));
    }

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteScalarRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 0, {AccessMode::raw});

    acc1 = 42;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, 42 );

    acc1 = -120;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2", 0, {AccessMode::raw});
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 1, {AccessMode::raw});

    acc2 = 666;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, 666 );

    acc2 = -99999;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, -99999 );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteScalarInAreaRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_AREA1", 0, {AccessMode::raw});
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 2, {AccessMode::raw});

    acc1 = 42;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, 42 );

    acc1 = -120;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_AREA1", 3, {AccessMode::raw});
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 5, {AccessMode::raw});

    acc2 = 666;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, 666 );

    acc2 = -99999;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, -99999 );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteArrayRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getOneDRegisterAccessor<int32_t>("APP.0.MY_AREA1", 0, 0, {AccessMode::raw});
    auto acc1t = target.getOneDRegisterAccessor<int32_t>("APP.0.THE_AREA", 6, 2, {AccessMode::raw});

    acc1 = { 10, 20, 30, 40, 50, 60 };
    acc1.write();
    acc1t.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1t == std::vector<int32_t>({ 10, 20, 30, 40, 50, 60 }) );


    acc1 = { 15, 25, 35, 45, 55, 65 };
    acc1.write();
    acc1t.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1t == std::vector<int32_t>({ 15, 25, 35, 45, 55, 65 }) );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteScalarCooked ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER1");   // 0 fractional bits
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 0, {AccessMode::raw});

    acc1 = 42;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, 42 );

    acc1 = -120;
    acc1.write();
    acc1t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1t, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER2");   // 2 fractional bits
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 1, {AccessMode::raw});

    acc2 = 666;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, 666*4 );

    acc2 = -333;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, (-333*4) & 0x3FFFF );   // the raw value does not get negative since we have 18 bits only

    acc2 = -99999;
    acc2.write();
    acc2t.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2t, 131072 );                // negative overflow

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testWriteArrayCooked ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getOneDRegisterAccessor<int32_t>("APP.0.MY_AREA1");
    auto acc1t = target.getOneDRegisterAccessor<int32_t>("APP.0.THE_AREA", 6, 2, {AccessMode::raw});

    acc1 = { 10, 20, 30, 40, 50, 60 };
    acc1.write();
    acc1t.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1t == std::vector<int32_t>({ 10 * 65536, 20 * 65536, 30 * 65536,
                                                                       40 * 65536, 50 * 65536, 60 * 65536 }) );


    acc1 = { 15, 25, 35, 45, 55, 65 };
    acc1.write();
    acc1t.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1t == std::vector<int32_t>({ 15 * 65536, 25 * 65536, 35 * 65536,
                                                                       45 * 65536, 55 * 65536, 65  * 65536}) );

    dev.close();

}

/*********************************************************************************************************************/
/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadScalarRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER1", 0, {AccessMode::raw});
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 0, {AccessMode::raw});

    acc1t = 42;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, 42 );

    acc1t = -120;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_REGISTER2", 0, {AccessMode::raw});
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 1, {AccessMode::raw});

    acc2t = 666;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, 666 );

    acc2t = -99999;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, -99999 );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadScalarInAreaRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_AREA1", 0, {AccessMode::raw});
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 2, {AccessMode::raw});

    acc1t = 42;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, 42 );

    acc1t = -120;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<int32_t>("APP.0.MY_AREA1", 3, {AccessMode::raw});
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 5, {AccessMode::raw});

    acc2t = 666;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, 666 );

    acc2t = -99999;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, -99999 );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadArrayRaw ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getOneDRegisterAccessor<int32_t>("APP.0.MY_AREA1", 0, 0, {AccessMode::raw});
    auto acc1t = target.getOneDRegisterAccessor<int32_t>("APP.0.THE_AREA", 6, 2, {AccessMode::raw});

    acc1t = { 10, 20, 30, 40, 50, 60 };
    acc1t.write();
    acc1.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1 == std::vector<int32_t>({ 10, 20, 30, 40, 50, 60 }) );


    acc1t = { 15, 25, 35, 45, 55, 65 };
    acc1t.write();
    acc1.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1 == std::vector<int32_t>({ 15, 25, 35, 45, 55, 65 }) );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadScalarCooked ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER1");   // 0 fractional bits
    auto acc1t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 0, {AccessMode::raw});

    acc1t = 42;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, 42 );

    acc1t = -120;
    acc1t.write();
    acc1.read();
    BOOST_CHECK_EQUAL( (int32_t)acc1, -120 );

    auto acc2  = dev.getScalarRegisterAccessor<double>("APP.0.MY_REGISTER2");   // 2 fractional bits
    auto acc2t = target.getScalarRegisterAccessor<int32_t>("APP.0.THE_AREA", 1, {AccessMode::raw});

    acc2t = 666*4;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, 666 );

    acc2t = -333*4;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, -333 );   // the raw value does not get negative since we have 18 bits only

    acc2t = 131072;
    acc2t.write();
    acc2.read();
    BOOST_CHECK_EQUAL( (int32_t)acc2, -32768 );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE( testReadArrayCooked ) {

    setDMapFilePath("subdeviceTest.dmap");

    Device dev;
    dev.open("SUBDEV1");
    Device target;
    target.open("TARGET1");

    auto acc1  = dev.getOneDRegisterAccessor<int32_t>("APP.0.MY_AREA1");
    auto acc1t = target.getOneDRegisterAccessor<int32_t>("APP.0.THE_AREA", 6, 2, {AccessMode::raw});

    acc1t = { 10 * 65536, 20 * 65536, 30 * 65536, 40 * 65536, 50 * 65536, 60 * 65536 };
    acc1t.write();
    acc1.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1 == std::vector<int32_t>({ 10, 20, 30, 40, 50, 60 }) );


    acc1t = { 15 * 65536, 25 * 65536, 35 * 65536, 45 * 65536, 55 * 65536, 65 * 65536 };
    acc1t.write();
    acc1.read();
    BOOST_CHECK( (std::vector<int32_t>)acc1 == std::vector<int32_t>({ 15, 25, 35, 45, 55, 65 }) );

    dev.close();

}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
