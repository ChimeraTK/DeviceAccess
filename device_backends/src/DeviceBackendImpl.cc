// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeviceBackendImpl.h"

#include <unistd.h>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void DeviceBackendImpl::setOpenedAndClearException() noexcept {
    // busy-wait until all exceptions have been distributed to the AsyncAccessors
    while(_asyncDomainsContainer->isSendingExceptions()) {
      usleep(10000);
    }

    _opened = true;
    _hasActiveException = false;
    std::lock_guard<std::mutex> lk(_mx_activeExceptionMessage);
    _activeExceptionMessage = "(exception cleared)";
  }

  /********************************************************************************************************************/

  void DeviceBackendImpl::setException(const std::string& message) noexcept {
    // set exception flag and atomically get the previous state
    bool hadExceptionBefore = _hasActiveException.exchange(true);

    // if previously backend was already in exception state, don't continue here
    if(hadExceptionBefore) {
      return;
    }

    // set exception message
    {
      std::lock_guard<std::mutex> lk(_mx_activeExceptionMessage);
      _activeExceptionMessage = message;
    }

    // execute backend-specific code
    setExceptionImpl();

    // finally turn off all async accessors and distribute the exception to them
    _asyncDomainsContainer->sendExceptions(message);
  }

  /********************************************************************************************************************/

  std::string DeviceBackendImpl::getActiveExceptionMessage() noexcept {
    std::lock_guard<std::mutex> lk(_mx_activeExceptionMessage);
    return _activeExceptionMessage;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
