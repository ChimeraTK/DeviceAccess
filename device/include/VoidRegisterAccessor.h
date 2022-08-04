// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Accessor class to read and write void-typed registers.
   */
  class VoidRegisterAccessor : public NDRegisterAccessorAbstractor<ChimeraTK::Void> {
   public:
    /**
     * Construct from pointer to implementation. Normally not called by the user, use Device::getVoidRegisterAccessor()
     * instead.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    VoidRegisterAccessor(const boost::shared_ptr<NDRegisterAccessor<Void>>& impl);

    /**
     * Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
     *
     * @attention Accessors created with this constructors will be dysfunctional, calling any member function will throw
     * an exception (by the boost::shared_ptr)!
     */
    VoidRegisterAccessor() = default;

    [[nodiscard]] bool isReadOnly() const;

    [[nodiscard]] bool isReadable() const;

    void read();

    bool readNonBlocking();

    bool readLatest();
  };

  /********************************************************************************************************************/

  inline void VoidRegisterAccessor::read() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    _impl->read();
  }

  /********************************************************************************************************************/

  inline bool VoidRegisterAccessor::readNonBlocking() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    return _impl->readNonBlocking();
  }

  /********************************************************************************************************************/

  inline bool VoidRegisterAccessor::readLatest() {
    if(!_impl->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("A VoidRegisterAccessor without wait_for_new_data is not readable.");
    }
    return _impl->readLatest();
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
