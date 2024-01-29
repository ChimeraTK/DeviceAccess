// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "RegisterInfo.h"

namespace ChimeraTK {

  /*******************************************************************************************************************/

  RegisterInfo::RegisterInfo(std::unique_ptr<BackendRegisterInfoBase>&& impl) : _impl(std::move(impl)) {}

  /*******************************************************************************************************************/

  RegisterInfo::RegisterInfo(const RegisterInfo& other) {
    operator=(other);
  }

  /*******************************************************************************************************************/

  RegisterInfo& RegisterInfo::operator=(const RegisterInfo& other) {
    if(other.isValid()) {
      _impl = other.getImpl().clone();
    }
    else {
      _impl = nullptr;
    }
    return *this;
  }

  /*******************************************************************************************************************/

  RegisterPath RegisterInfo::getRegisterName() const {
    return _impl->getRegisterName();
  }

  /*******************************************************************************************************************/

  unsigned int RegisterInfo::getNumberOfElements() const {
    return _impl->getNumberOfElements();
  }

  /*******************************************************************************************************************/

  unsigned int RegisterInfo::getNumberOfChannels() const {
    return _impl->getNumberOfChannels();
  }

  /*******************************************************************************************************************/

  unsigned int RegisterInfo::getNumberOfDimensions() const {
    return _impl->getNumberOfDimensions();
  }

  /*******************************************************************************************************************/

  const DataDescriptor& RegisterInfo::getDataDescriptor() const {
    return _impl->getDataDescriptor();
  }

  /*******************************************************************************************************************/

  bool RegisterInfo::isReadable() const {
    return _impl->isReadable();
  }

  /*******************************************************************************************************************/

  bool RegisterInfo::isWriteable() const {
    return _impl->isWriteable();
  }

  /*******************************************************************************************************************/

  AccessModeFlags RegisterInfo::getSupportedAccessModes() const {
    return _impl->getSupportedAccessModes();
  }

  /*******************************************************************************************************************/

  bool RegisterInfo::isValid() const {
    return _impl != nullptr;
  }

  /*******************************************************************************************************************/

  BackendRegisterInfoBase& RegisterInfo::getImpl() {
    return *_impl;
  }

  /*******************************************************************************************************************/

  const BackendRegisterInfoBase& RegisterInfo::getImpl() const {
    return *_impl;
  }

  /*******************************************************************************************************************/

} // namespace ChimeraTK
