///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
using namespace boost::unit_test_framework;

#include "BackendRegisterCatalogue.h"

using namespace ChimeraTK;

class RegisterCatalogueTest {
 public:
  void testRegisterCatalogue();
};

class registerCatalogueTestSuite : public test_suite {
 public:
  registerCatalogueTestSuite() : test_suite("RegisterCatalogue class test suite") {
    boost::shared_ptr<RegisterCatalogueTest> test(new RegisterCatalogueTest());

    add(BOOST_CLASS_TEST_CASE(&RegisterCatalogueTest::testRegisterCatalogue, test));
  }
};

bool init_unit_test() {
  framework::master_test_suite().p_name.value = "RegisterCatalogue class test suite";
  framework::master_test_suite().add(new registerCatalogueTestSuite());

  return true;
}

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

  unsigned int getNumberOfDimensions() const override { return _nbOfDimensions; }

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

void RegisterCatalogueTest::testRegisterCatalogue() {
  BackendRegisterCatalogue<myRegisterInfo> catalogue;

  DataDescriptor dataDescriptor(DataDescriptor::FundamentalType::numeric, false, false, 8, 3, DataType::int32);
  myRegisterInfo theInfo("/some/register/name", 42, 3, 2, dataDescriptor, true, false, {AccessMode::raw});
  catalogue.addRegister(theInfo);

  DataDescriptor dataDescriptor2(DataDescriptor::FundamentalType::numeric, true, false, 12);
  myRegisterInfo theInfo2(
      "/some/other/name", 1, 1, 0, dataDescriptor2, true, true, {AccessMode::raw, AccessMode::wait_for_new_data});
  catalogue.addRegister(theInfo2);

  DataDescriptor dataDescriptor3(DataDescriptor::FundamentalType::string);
  myRegisterInfo theInfo3("/justAName", 1, 1, 0, dataDescriptor3, false, false, {});
  catalogue.addRegister(theInfo3);

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
  BOOST_TEST(theImpl_casted != &theInfo);

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

  // create clone of the entire catalogue (must be a deep copy)
  auto cat_copy = RegisterCatalogue(catalogue.clone());
  BOOST_TEST(cat_copy.getNumberOfRegisters() == 3);
  for(auto& info_copy : cat_copy) {
    auto info_orig = catalogue.getBackendRegister(info_copy.getRegisterName());
    auto& info_copy_casted = dynamic_cast<const myRegisterInfo&>(info_copy);
    BOOST_CHECK(info_orig == info_copy_casted);
    BOOST_TEST(&info_orig != &info_copy);
  }
}
