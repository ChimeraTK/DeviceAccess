// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AsyncAccessorManager.h"

namespace ChimeraTK {

  //*********************************************************************************************************************/
  void AsyncAccessorManager::unsubscribe(TransferElementID id) {
    // The destructor of the AsyncVariable implementation must do all necessary clean-up
    _asyncVariables.erase(id);

    asyncVariableMapChanged();
  }

  //*********************************************************************************************************************/
  void AsyncAccessorManager::sendException(const std::exception_ptr& e) {
    for(auto& var : _asyncVariables) {
      var.second->sendException(e);
    }
  }

} // namespace ChimeraTK
