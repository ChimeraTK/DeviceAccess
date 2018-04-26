#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RegisterAccessSpecifierTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE( RegisterAccessSpecifierTestSuite )

BOOST_AUTO_TEST_CASE( testRegisterAccess ) {
    Device dev;
    dev.open("sdm://./pci:pcieunidummys6=registerAccess.map");
    BOOST_CHECK(dev.isOpened());

    {
        auto accessor = dev.getScalarRegisterAccessor<int>("BOARD.WORD_FIRMWARE");
        BOOST_CHECK(accessor.isReadOnly());
        BOOST_CHECK(!accessor.isWriteable());
        BOOST_CHECK(accessor.isReadable());
    }

    {
        auto accessor = dev.getScalarRegisterAccessor<int>("ADC.WORD_CLK_DUMMY");
        BOOST_CHECK(!accessor.isReadOnly());
        BOOST_CHECK(accessor.isWriteable());
        BOOST_CHECK(accessor.isReadable());
    }

    {
        auto accessor = dev.getScalarRegisterAccessor<int>("ADC.WORD_ADC_ENA");
        BOOST_CHECK(!accessor.isReadOnly());
        BOOST_CHECK(accessor.isWriteable());
        BOOST_CHECK(!accessor.isReadable());
    }

}

BOOST_AUTO_TEST_SUITE_END()
