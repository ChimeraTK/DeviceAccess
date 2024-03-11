// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE RegisterCatalogue
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendRegisterCatalogue.h"

using namespace ChimeraTK;

/**********************************************************************************************************************/

class myRegisterInfo : public BackendRegisterInfoBase {
 public:
  myRegisterInfo(std::string path, unsigned int nbOfElements, unsigned int nbOfChannels, unsigned int nbOfDimensions,
      DataDescriptor dataDescriptor, bool readable, bool writeable, AccessModeFlags supportedFlags)
  : _path(path), _nbOfElements(nbOfElements), _nbOfChannels(nbOfChannels), _nbOfDimensions(nbOfDimensions),
    _dataDescriptor(dataDescriptor), _readable(readable), _writeable(writeable), _supportedFlags(supportedFlags) {}

  myRegisterInfo() = default;

  RegisterPath getRegisterName() const override { return _path; }

  unsigned int getNumberOfElements() const override { return _nbOfElements; }

  unsigned int getNumberOfChannels() const override { return _nbOfChannels; }

  const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

  bool isReadable() const override { return _readable; }

  bool isWriteable() const override { return _writeable; }

  AccessModeFlags getSupportedAccessModes() const override { return _supportedFlags; }

  [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
    auto* info = new myRegisterInfo(*this);
    return std::unique_ptr<BackendRegisterInfoBase>(info);
  }

  bool operator==(const myRegisterInfo& other) const {
    return _path == other._path && _nbOfElements == other._nbOfElements && _nbOfChannels == other._nbOfChannels &&
        _nbOfDimensions == other._nbOfDimensions && _dataDescriptor == other._dataDescriptor &&
        _readable == other._readable && _writeable == other._writeable && _supportedFlags == other._supportedFlags;
  }

 protected:
  RegisterPath _path;
  unsigned int _nbOfElements, _nbOfChannels, _nbOfDimensions;
  DataDescriptor _dataDescriptor;
  bool _readable, _writeable;
  AccessModeFlags _supportedFlags;
};

/**********************************************************************************************************************/

class CatalogueGenerator {
 public:
  BackendRegisterCatalogue<myRegisterInfo> generateCatalogue() {
    BackendRegisterCatalogue<myRegisterInfo> catalogue;

    catalogue.addRegister(theInfo);
    catalogue.addRegister(theInfo2);
    catalogue.addRegister(theInfo3);

    return catalogue;
  }

  DataDescriptor dataDescriptor{DataDescriptor::FundamentalType::numeric, false, false, 8, 3, DataType::int32};
  myRegisterInfo theInfo{"/some/register/name", 42, 3, 2, dataDescriptor, true, false, {AccessMode::raw}};

  DataDescriptor dataDescriptor2{DataDescriptor::FundamentalType::numeric, true, false, 12};
  myRegisterInfo theInfo2{
      "/some/other/name", 1, 1, 0, dataDescriptor2, true, true, {AccessMode::raw, AccessMode::wait_for_new_data}};

  DataDescriptor dataDescriptor3{DataDescriptor::FundamentalType::string};
  myRegisterInfo theInfo3{"/justAName", 1, 1, 0, dataDescriptor3, false, false, {}};
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE(RegisterCatalogueTestSuite)

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDirectAccess) {
  CatalogueGenerator generator;
  auto catalogue = generator.generateCatalogue();

  BOOST_TEST(catalogue.getNumberOfRegisters() == 3);

  auto info = catalogue.getRegister("/some/register/name");
  // check accessor functions of RegisterInfo class
  BOOST_TEST(info.getRegisterName() == "/some/register/name");
  BOOST_TEST(info.getNumberOfElements() == 42);
  BOOST_TEST(info.getNumberOfChannels() == 3);
  BOOST_TEST(info.getNumberOfDimensions() == 2);
  BOOST_TEST(info.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::numeric);
  BOOST_TEST(info.getDataDescriptor().isSigned() == false);
  BOOST_TEST(info.getDataDescriptor().isIntegral() == false);
  BOOST_TEST(info.getDataDescriptor().nDigits() == 8);
  BOOST_TEST(info.getDataDescriptor().nFractionalDigits() == 3);
  BOOST_TEST(info.getDataDescriptor().rawDataType() == DataType::int32);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isNumeric());
  BOOST_TEST(info.getDataDescriptor().rawDataType().isIntegral());
  BOOST_TEST(info.getDataDescriptor().rawDataType().isSigned());
  BOOST_TEST(info.isReadable() == true);
  BOOST_TEST(info.isWriteable() == false);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::raw) == true);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::wait_for_new_data) == false);
  // check that the RegisterInfoImpl object is a copy of the original object (different address but identical content).
  auto& theImpl = info.getImpl();
  auto theImpl_casted = dynamic_cast<myRegisterInfo*>(&theImpl);
  BOOST_TEST(theImpl_casted != nullptr);
  BOOST_TEST(theImpl_casted != &generator.theInfo);

  info = catalogue.getRegister("/some/other/name");
  BOOST_TEST(info.getRegisterName() == "/some/other/name");
  BOOST_TEST(info.getNumberOfElements() == 1);
  BOOST_TEST(info.getNumberOfChannels() == 1);
  BOOST_TEST(info.getNumberOfDimensions() == 0);
  BOOST_TEST(info.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::numeric);
  BOOST_TEST(info.getDataDescriptor().isSigned() == false);
  BOOST_TEST(info.getDataDescriptor().isIntegral() == true);
  BOOST_TEST(info.getDataDescriptor().nDigits() == 12);
  BOOST_TEST(info.getDataDescriptor().rawDataType() == DataType::none);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isNumeric() == false);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isIntegral() == false);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isSigned() == false);
  BOOST_TEST(info.isReadable() == true);
  BOOST_TEST(info.isWriteable() == true);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::raw) == true);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::wait_for_new_data) == true);

  info = catalogue.getRegister("/justAName");
  BOOST_TEST(info.getRegisterName() == "/justAName");
  BOOST_TEST(info.getNumberOfElements() == 1);
  BOOST_TEST(info.getNumberOfChannels() == 1);
  BOOST_TEST(info.getNumberOfDimensions() == 0);
  BOOST_TEST(info.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::string);
  BOOST_TEST(info.getDataDescriptor().rawDataType() == DataType::none);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isNumeric() == false);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isIntegral() == false);
  BOOST_TEST(info.getDataDescriptor().rawDataType().isSigned() == false);
  BOOST_TEST(info.isReadable() == false);
  BOOST_TEST(info.isWriteable() == false);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::raw) == false);
  BOOST_TEST(info.getSupportedAccessModes().has(AccessMode::wait_for_new_data) == false);
}
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testClone) {
  CatalogueGenerator generator;
  auto catalogue = generator.generateCatalogue();

  // create clone of the entire catalogue (must be a deep copy)
  std::unique_ptr<BackendRegisterCatalogue<myRegisterInfo>> cat_copy(
      dynamic_cast<BackendRegisterCatalogue<myRegisterInfo>*>(catalogue.clone().release()));
  BOOST_TEST(cat_copy->getNumberOfRegisters() == 3);

  BOOST_CHECK(
      catalogue.getBackendRegister("/some/register/name") == cat_copy->getBackendRegister("/some/register/name"));
  BOOST_CHECK(catalogue.getBackendRegister("/some/other/name") == cat_copy->getBackendRegister("/some/other/name"));
  BOOST_CHECK(catalogue.getBackendRegister("/justAName") == cat_copy->getBackendRegister("/justAName"));

  std::vector<myRegisterInfo> seenObjects;
  for(auto& i : *cat_copy) {
    seenObjects.push_back(i);
  }

  BOOST_TEST(seenObjects.size() == 3);
  BOOST_CHECK(seenObjects[0] == generator.theInfo);
  BOOST_CHECK(seenObjects[1] == generator.theInfo2);
  BOOST_CHECK(seenObjects[2] == generator.theInfo3);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeBasedLoopBackend) {
  CatalogueGenerator generator;
  auto catalogue = generator.generateCatalogue();

  std::vector<myRegisterInfo> seenObjects;
  for(auto& elem : catalogue) {
    seenObjects.push_back(elem);
  }

  BOOST_TEST(seenObjects.size() == 3);
  BOOST_CHECK(seenObjects[0] == generator.theInfo);
  BOOST_CHECK(seenObjects[1] == generator.theInfo2);
  BOOST_CHECK(seenObjects[2] == generator.theInfo3);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeBasedLoopBackendConst) {
  CatalogueGenerator generator;
  const auto catalogue = generator.generateCatalogue();

  std::vector<myRegisterInfo> seenObjects;
  for(const auto& elem : catalogue) {
    seenObjects.push_back(elem);
  }

  BOOST_TEST(seenObjects.size() == 3);
  BOOST_CHECK(seenObjects[0] == generator.theInfo);
  BOOST_CHECK(seenObjects[1] == generator.theInfo2);
  BOOST_CHECK(seenObjects[2] == generator.theInfo3);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeBasedLoopFrontend) {
  CatalogueGenerator generator;
  auto backend_catalogue = generator.generateCatalogue();
  RegisterCatalogue catalogue(backend_catalogue.clone());

  std::vector<RegisterInfo> seenObjects;
  for(const auto& elem : catalogue) {
    seenObjects.emplace_back(elem.clone());
  }

  BOOST_TEST(seenObjects.size() == 3);
  BOOST_TEST(seenObjects[0].getRegisterName() == generator.theInfo.getRegisterName());
  BOOST_TEST(seenObjects[1].getRegisterName() == generator.theInfo2.getRegisterName());
  BOOST_TEST(seenObjects[2].getRegisterName() == generator.theInfo3.getRegisterName());
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
