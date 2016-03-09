#include "Device.h"
#include "RegisterCatalogue.h"

namespace mtca4u {

  boost::shared_ptr<RegisterInfo> RegisterCatalogue::getRegister(const RegisterPath& registerPathName) {
    return catalogue[registerPathName];   /// @todo TODO catch non-existing registers
  }
    
  void RegisterCatalogue::addRegister(boost::shared_ptr<RegisterInfo> registerInfo) {
    catalogue[registerInfo->getRegisterName()] = registerInfo;
  }
  

} /* namespace mtca4u */
