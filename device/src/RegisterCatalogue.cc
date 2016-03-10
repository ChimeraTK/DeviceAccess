#include "Device.h"
#include "RegisterCatalogue.h"

namespace mtca4u {

  boost::shared_ptr<RegisterInfo> RegisterCatalogue::getRegister(const RegisterPath& registerPathName) const {
    try {
      return catalogue.at(registerPathName);
    }
    catch(std::out_of_range &e) {
      throw DeviceException("Register '"+(registerPathName)+"' was not found in the catalogue ("+e.what()+").",
          DeviceException::REGISTER_DOES_NOT_EXIST);
    }
  }
    
  void RegisterCatalogue::addRegister(boost::shared_ptr<RegisterInfo> registerInfo) {
    catalogue[registerInfo->getRegisterName()] = registerInfo;
  }
  

} /* namespace mtca4u */
