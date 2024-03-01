// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomainsContainerBase.h"
#include "DeviceBackend.h"
#include "Exception.h"
#include <condition_variable>
#include <shared_mutex>

#include <ChimeraTK/cppext/finally.hpp>

#include <atomic>
#include <list>
#include <mutex>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * DeviceBackendImpl implements some basic functionality which should be available for all backends. This is required
   * to allow proper decorator patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl : public DeviceBackend {
   public:
    bool isOpen() override { return _opened; }

    bool isConnected() final {
      std::cerr << "Removed function DeviceBackendImpl::isConnected() called." << std::endl;
      std::cerr << "Do not use. This function has no valid meaning." << std::endl;
      std::terminate();
    }

    MetadataCatalogue getMetadataCatalogue() const override { return {}; }

    /**
     * Function to be (optionally) implemented by backends if additional actions are needed when switching to the
     * exception state.
     */
    virtual void setExceptionImpl() noexcept {}

    /**
     * Function to be called by backends when needing to check for an active exception. If an active exception is found,
     * the appropriate ChimeraTK::runtime_error is thrown by this function.
     */
    void checkActiveException() final;

    void setException(const std::string& message) noexcept final;

    bool isFunctional() const noexcept final;

    std::string getActiveExceptionMessage() noexcept;

   protected:
    /** Backends should call this function at the end of a (successful) open() call.*/
    void setOpenedAndClearException() noexcept;

    /** flag if backend is opened */
    std::atomic<bool> _opened{false};

    /** Container for AsyncDomains to support wait_for_new_data.
     *  The variable is initialised with an empty base implementation here. Backends which support push type accessors
     *  will replace it with a proper implementation.
     */
    std::unique_ptr<AsyncDomainsContainerBase> _asyncDomainsContainer{std::make_unique<AsyncDomainsContainerBase>()};

   private:
    /** flag if backend is in an exception state */
    std::atomic<bool> _hasActiveException{false};

    /**
     *  message for the current exception, if _hasActiveException is true. Access is protected by
     * _mx_activeExceptionMessage
     */
    std::string _activeExceptionMessage;

    /** mutex to protect access to _activeExceptionMessage */
    std::mutex _mx_activeExceptionMessage;
  };

  /********************************************************************************************************************/

  // This function is rather often called and hence implemented as inline in the header for performance reasons.
  inline bool DeviceBackendImpl::isFunctional() const noexcept {
    if(!_opened) return false;
    if(_hasActiveException) return false;
    return true;
  }

  /********************************************************************************************************************/

  // This function is rather often called and hence implemented as inline in the header for performance reasons.
  inline void DeviceBackendImpl::checkActiveException() {
    if(_hasActiveException) {
      std::lock_guard<std::mutex> lk(_mx_activeExceptionMessage);
      throw ChimeraTK::runtime_error(_activeExceptionMessage);
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
