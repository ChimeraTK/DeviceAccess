///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
using namespace boost::unit_test_framework;

#include "RegisterCatalogue.h"

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

class myRegisterInfo : public RegisterInfo {
 public:
  myRegisterInfo(std::string path, unsigned int nbOfElements, unsigned int nbOfChannels, unsigned int nbOfDimensions,
      DataDescriptor dataDescriptor, bool readable, bool writeable, AccessModeFlags supportedFlags)
  : _path(path), _nbOfElements(nbOfElements), _nbOfChannels(nbOfChannels), _nbOfDimensions(nbOfDimensions),
    _dataDescriptor(dataDescriptor), _readable(readable), _writeable(writeable), _supportedFlags(supportedFlags) {}
  RegisterPath getRegisterName() const override { return _path; }
  unsigned int getNumberOfElements() const override { return _nbOfElements; }
  unsigned int getNumberOfChannels() const override { return _nbOfChannels; }
  unsigned int getNumberOfDimensions() const override { return _nbOfDimensions; }
  const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }
  bool isReadable() const override { return _readable; }
  bool isWriteable() const override { return _writeable; }
  AccessModeFlags getSupportedAccessModes() const override { return _supportedFlags; }

 protected:
  RegisterPath _path;
  unsigned int _nbOfElements, _nbOfChannels, _nbOfDimensions;
  DataDescriptor _dataDescriptor;
  bool _readable, _writeable;
  AccessModeFlags _supportedFlags;
};

void RegisterCatalogueTest::testRegisterCatalogue() {
  RegisterCatalogue catalogue;
  boost::shared_ptr<RegisterInfo> info;

  RegisterInfo::DataDescriptor dataDescriptor(
      RegisterInfo::FundamentalType::numeric, false, false, 8, 3, DataType::int32);
  catalogue.addRegister(boost::shared_ptr<RegisterInfo>(
      new myRegisterInfo("/some/register/name", 42, 3, 2, dataDescriptor, true, false, {AccessMode::raw})));
  info = catalogue.getRegister("/some/register/name");
  BOOST_CHECK(info->getRegisterName() == "/some/register/name");
  BOOST_CHECK(info->getNumberOfElements() == 42);
  BOOST_CHECK(info->getNumberOfChannels() == 3);
  BOOST_CHECK(info->getNumberOfDimensions() == 2);
  BOOST_CHECK(info->getDataDescriptor().fundamentalType() == RegisterInfo::FundamentalType::numeric);
  BOOST_CHECK(info->getDataDescriptor().isSigned() == false);
  BOOST_CHECK(info->getDataDescriptor().isIntegral() == false);
  BOOST_CHECK(info->getDataDescriptor().nDigits() == 8);
  BOOST_CHECK(info->getDataDescriptor().nFractionalDigits() == 3);
  BOOST_CHECK(info->getDataDescriptor().rawDataType() == DataType::int32);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isNumeric());
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isIntegral());
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isSigned());
  BOOST_CHECK(info->isReadable() == true);
  BOOST_CHECK(info->isWriteable() == false);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::raw) == true);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::wait_for_new_data) == false);

  RegisterInfo::DataDescriptor dataDescriptor2(RegisterInfo::FundamentalType::numeric, true, false, 12);
  catalogue.addRegister(boost::shared_ptr<RegisterInfo>(new myRegisterInfo(
      "/some/other/name", 1, 1, 0, dataDescriptor2, true, true, {AccessMode::raw, AccessMode::wait_for_new_data})));
  info = catalogue.getRegister("/some/other/name");
  BOOST_CHECK(info->getRegisterName() == "/some/other/name");
  BOOST_CHECK(info->getNumberOfElements() == 1);
  BOOST_CHECK(info->getNumberOfChannels() == 1);
  BOOST_CHECK(info->getNumberOfDimensions() == 0);
  BOOST_CHECK(info->getDataDescriptor().fundamentalType() == RegisterInfo::FundamentalType::numeric);
  BOOST_CHECK(info->getDataDescriptor().isSigned() == false);
  BOOST_CHECK(info->getDataDescriptor().isIntegral() == true);
  BOOST_CHECK(info->getDataDescriptor().nDigits() == 12);
  BOOST_CHECK(info->getDataDescriptor().rawDataType() == DataType::none);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isNumeric() == false);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isIntegral() == false);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isSigned() == false);
  BOOST_CHECK(info->isReadable() == true);
  BOOST_CHECK(info->isWriteable() == true);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::raw) == true);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::wait_for_new_data) == true);

  RegisterInfo::DataDescriptor dataDescriptor3(RegisterInfo::FundamentalType::string);
  catalogue.addRegister(
      boost::shared_ptr<RegisterInfo>(new myRegisterInfo("/justAName", 1, 1, 0, dataDescriptor3, false, false, {})));
  info = catalogue.getRegister("/justAName");
  BOOST_CHECK(info->getRegisterName() == "/justAName");
  BOOST_CHECK(info->getNumberOfElements() == 1);
  BOOST_CHECK(info->getNumberOfChannels() == 1);
  BOOST_CHECK(info->getNumberOfDimensions() == 0);
  BOOST_CHECK(info->getDataDescriptor().fundamentalType() == RegisterInfo::FundamentalType::string);
  BOOST_CHECK(info->getDataDescriptor().rawDataType() == DataType::none);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isNumeric() == false);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isIntegral() == false);
  BOOST_CHECK(info->getDataDescriptor().rawDataType().isSigned() == false);
  BOOST_CHECK(info->isReadable() == false);
  BOOST_CHECK(info->isWriteable() == false);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::raw) == false);
  BOOST_CHECK(info->getSupportedAccessModes().has(AccessMode::wait_for_new_data) == false);
}
