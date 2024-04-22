// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "async/AsyncAccessorManager.h"

namespace ChimeraTK::async {

  thread_local AsyncAccessorManager* AsyncAccessorManager::_isHoldingDomainLock{nullptr};

  /********************************************************************************************************************/
  void AsyncAccessorManager::unsubscribeImpl(TransferElementID id) {
    asyncVariableMapChanged(id);
    // The destructor of the AsyncVariable implementation must do all necessary clean-up
    _asyncVariables.erase(id);
  }
  /********************************************************************************************************************/
  void AsyncAccessorManager::unsubscribe(TransferElementID id) {
    if(_isHoldingDomainLock == this) {
      _delayedUnsubscriptions.push_back(id);
    }
    else {
      auto domainLock = _asyncDomain->getDomainLock();
      unsubscribeImpl(id);
    }
  }

  /********************************************************************************************************************/
  void AsyncAccessorManager::sendException(const std::exception_ptr& e) {
    _isHoldingDomainLock = this;
    assert(_delayedUnsubscriptions.empty());

    for(auto& var : _asyncVariables) {
      var.second->sendException(e);
    }
    _isHoldingDomainLock = nullptr;

    for(auto id : _delayedUnsubscriptions) {
      unsubscribeImpl(id);
    }
    _delayedUnsubscriptions.clear();
  }

} // namespace ChimeraTK::async
