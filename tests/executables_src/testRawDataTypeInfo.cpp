// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestRawDataTypeInfo
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"

#include <limits>

using namespace ChimeraTK;

// DataType::getNumberOfBytes() returns the element data width in bytes for every DataType value: 1/2/4/8 for the
// integer types by their width, 4 for float32 and 8 for float64, sizeof(ChimeraTK::Boolean) for Boolean, 0 for
// none and Void (no data). We cannot test string, as it will trigger an assertion.
BOOST_AUTO_TEST_CASE(TestGetNumberOfBytes) {
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::int8).getNumberOfBytes()) == 1);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::uint8).getNumberOfBytes()) == 1);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::int16).getNumberOfBytes()) == 2);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::uint16).getNumberOfBytes()) == 2);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::int32).getNumberOfBytes()) == 4);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::uint32).getNumberOfBytes()) == 4);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::int64).getNumberOfBytes()) == 8);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::uint64).getNumberOfBytes()) == 8);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::float32).getNumberOfBytes()) == 4);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::float64).getNumberOfBytes()) == 8);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::Boolean).getNumberOfBytes()) == sizeof(ChimeraTK::Boolean));
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::none).getNumberOfBytes()) == 0);
  BOOST_TEST((ChimeraTK::DataType(ChimeraTK::DataType::Void).getNumberOfBytes()) == 0);
}

BOOST_AUTO_TEST_CASE(testRawAccessor) {
  setDMapFilePath("dummies.dmap");

  Device d;
  d.open("DUMMYD3");

  auto registerCatalogue = d.getRegisterCatalogue();
  auto registerInfo = registerCatalogue.getRegister("BOARD/WORD_USER");

  BOOST_CHECK(registerInfo.getDataDescriptor().isIntegral() == false);
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType() == DataType::int32);
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isNumeric());
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isIntegral());
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isSigned());

  // check in integral data type
  registerInfo = registerCatalogue.getRegister("BOARD/WORD_STATUS");

  BOOST_CHECK(registerInfo.getDataDescriptor().isIntegral() == true);
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType() == DataType::int32);
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isNumeric());
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isIntegral());
  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType().isSigned());

  Device d2;
  d2.open("SEQUENCES");

  auto registerCatalogue2 = d2.getRegisterCatalogue();
  registerInfo = registerCatalogue2.getRegister("TEST/DMA");

  BOOST_CHECK(registerInfo.getDataDescriptor().rawDataType() == DataType::none);

  ///@todo FIXME Test something that does not have raw data transfer
}
