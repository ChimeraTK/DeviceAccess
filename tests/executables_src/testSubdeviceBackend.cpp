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

using namespace mtca4u;

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

BOOST_AUTO_TEST_SUITE_END()
