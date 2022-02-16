#include "RegisterInfo.h"

namespace ChimeraTK {
  RegisterPath RegisterInfo::getRegisterName() const{
    return impl->getRegisterName();
  }

  unsigned int RegisterInfo::getNumberOfElements() const{
    return impl->getNumberOfElements();
  }

  unsigned int RegisterInfo::getNumberOfChannels() const{
    return impl->getNumberOfChannels();
  }

  unsigned int RegisterInfo::getNumberOfDimensions() const{
    return impl->getNumberOfDimensions();
  }

  const DataDescriptor& RegisterInfo::getDataDescriptor() const{
    return impl->getDataDescriptor();
  }

  bool RegisterInfo::isReadable() const{
    return impl->isReadable();
  }

  bool RegisterInfo::isWriteable() const{
    return impl->isWriteable();
  }

  AccessModeFlags RegisterInfo::getSupportedAccessModes() const{
    return impl->getSupportedAccessModes();
  }

  boost::shared_ptr<RegisterInfoImpl> RegisterInfo::getImpl(){
      return impl;
  }

}
