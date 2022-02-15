#include "RegisterCatalogue.h"
#include "Device.h"

namespace ChimeraTK {

  boost::shared_ptr<RegisterInfoImpl> RegisterCatalogue::getRegister(const RegisterPath& registerPathName) const {
    auto it =
        std::find_if(catalogue.begin(), catalogue.end(), [registerPathName](boost::shared_ptr<RegisterInfoImpl> info) {
          return info->getRegisterName() == registerPathName;
        });
    if(it == catalogue.end()) {
      throw ChimeraTK::logic_error("Register '" + (registerPathName) + "' was not found in the catalogue.");
    }
    return *it;
  }

  /********************************************************************************************************************/

  bool RegisterCatalogue::hasRegister(const RegisterPath& registerPathName) const {
    auto it =
        std::find_if(catalogue.begin(), catalogue.end(), [registerPathName](boost::shared_ptr<RegisterInfoImpl> info) {
          return info->getRegisterName() == registerPathName;
        });
    if(it == catalogue.end()) {
      return false;
    }
    return true;
  }

  /********************************************************************************************************************/

  void RegisterCatalogue::addRegister(boost::shared_ptr<RegisterInfoImpl> registerInfo) {
    catalogue.push_back(registerInfo);
  }

  /********************************************************************************************************************/

  const std::string& RegisterCatalogue::getMetadata(const std::string& key) const {
    try {
      return metadata.at(key);
    }
    catch(std::out_of_range& e) {
      throw ChimeraTK::logic_error("Metadata '" + (key) + "' was not found in the catalogue (" + e.what() + ").");
    }
  }

  /********************************************************************************************************************/

  void RegisterCatalogue::addMetadata(const std::string& key, const std::string& value) { metadata[key] = value; }

  /********************************************************************************************************************/

  size_t RegisterCatalogue::getNumberOfRegisters() const { return catalogue.size(); }

  /********************************************************************************************************************/

  size_t RegisterCatalogue::getNumberOfMetadata() const { return metadata.size(); }

  /********************************************************************************************************************/

  RegisterCatalogue::metadata_iterator RegisterCatalogue::metadata_begin() { return metadata.begin(); }

  /********************************************************************************************************************/

  RegisterCatalogue::metadata_const_iterator RegisterCatalogue::metadata_begin() const { return metadata.cbegin(); }

  /********************************************************************************************************************/

  RegisterCatalogue::metadata_iterator RegisterCatalogue::metadata_end() { return metadata.end(); }

  /********************************************************************************************************************/

  RegisterCatalogue::metadata_const_iterator RegisterCatalogue::metadata_end() const { return metadata.cend(); }

} /* namespace ChimeraTK */
