#define BOOST_TEST_MODULE testExceptionsDummy

#include "ExceptionDevice.h"
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/Device.h>
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

auto exceptionDummy = boost::dynamic_pointer_cast<ExceptionDummy>(ctk::BackendFactory::getInstance().createBackend("(ExceptionDummy:1?map=test3.map)"));
ctk::Device device;

BOOST_AUTO_TEST_CASE(testExceptionsDummyDevice) {

 // test general function
 BOOST_CHECK(!device.isFunctional());
 device.open("(ExceptionDummy:1?map=test3.map)");
 BOOST_CHECK(device.isFunctional());


 // test throwExceptionRead
 exceptionDummy->throwExceptionRead = true;
 BOOST_CHECK(!device.isFunctional());
 BOOST_CHECK_THROW(device.read<int32_t>("/Integers/signed32"),ChimeraTK::runtime_error);
 BOOST_CHECK(!device.isFunctional());
 BOOST_CHECK_THROW(device.open("(ExceptionDummy:1?map=test3.map)"),ChimeraTK::runtime_error);
 BOOST_CHECK(!device.isFunctional());
 exceptionDummy->throwExceptionRead = false;
 BOOST_CHECK(!device.isFunctional());
 device.open("(ExceptionDummy:1?map=test3.map)");
 BOOST_CHECK(device.isFunctional());

 // test throwExceptionWrite
 exceptionDummy->throwExceptionWrite = true;
 BOOST_CHECK(!device.isFunctional());
 BOOST_CHECK_THROW(device.write<int32_t>("/Integers/signed32",0),ChimeraTK::runtime_error);
 BOOST_CHECK(!device.isFunctional());
 BOOST_CHECK_THROW(device.open("(ExceptionDummy:1?map=test3.map)"),ChimeraTK::runtime_error);
 BOOST_CHECK(!device.isFunctional());
 exceptionDummy->throwExceptionWrite = false;
 BOOST_CHECK(!device.isFunctional());
 device.open("(ExceptionDummy:1?map=test3.map)");
 BOOST_CHECK(device.isFunctional());

 // test throwExceptionOpen
 exceptionDummy->throwExceptionOpen = true;
 BOOST_CHECK(!device.isFunctional());
 BOOST_CHECK_THROW(device.open("(ExceptionDummy:1?map=test3.map)"),ChimeraTK::runtime_error);
 BOOST_CHECK(!device.isFunctional());
 exceptionDummy->throwExceptionOpen = false;
 device.open("(ExceptionDummy:1?map=test3.map)");
 BOOST_CHECK(device.isFunctional());

}
