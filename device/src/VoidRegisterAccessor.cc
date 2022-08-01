// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "VoidRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  VoidRegisterAccessor::VoidRegisterAccessor(const boost::shared_ptr<NDRegisterAccessor<Void>>& impl)
  : NDRegisterAccessorAbstractor(impl) {
    if(!impl->getAccessModeFlags().has(AccessMode::wait_for_new_data) && !impl->isWriteable()) {
      throw ChimeraTK::logic_error(
          "A VoidRegisterAccessor without wait_for_new_data does not make sense for non-writeable register " +
          impl->getName());
    }
  }

  /********************************************************************************************************************/

  bool VoidRegisterAccessor::isReadOnly() const {
    // synchronous void accessors are never readable, hence they are never read-only
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      return false;
    }
    return _impl->isReadOnly();
  }

  /********************************************************************************************************************/

  bool VoidRegisterAccessor::isReadable() const {
    // synchronous void accessors are never readable
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      return false;
    }
    return _impl->isReadable();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
