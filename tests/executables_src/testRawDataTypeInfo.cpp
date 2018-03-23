#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestRawDataTypeInfo
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <Device.h>
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE( testRawAccessor ){
  setDMapFilePath("dummies.dmap");

  Device d;
  d.open("PCIE1");

  auto registerCatalogue = d.getRegisterCatalogue();
  auto registerInfo = registerCatalogue.getRegister("BOARD/WORD_USER");

  BOOST_CHECK( registerInfo->getDataDescriptor().isIntegral() == false );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType() == DataType::int32 );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isNumeric() );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isIntegral() );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isSigned() );

// check in integral data type
  registerInfo = registerCatalogue.getRegister("BOARD/WORD_STATUS");

  BOOST_CHECK( registerInfo->getDataDescriptor().isIntegral() == true );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType() == DataType::int32 );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isNumeric() );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isIntegral() );
  BOOST_CHECK( registerInfo->getDataDescriptor().rawDataType().isSigned() );


  setDMapFilePath("dummies.dmap");

  ///@todo FIXME Test something that does not have raw data transfer
}
