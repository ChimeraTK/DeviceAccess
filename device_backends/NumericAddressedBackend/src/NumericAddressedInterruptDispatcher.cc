// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  NumericAddressedInterruptDispatcher::NumericAddressedInterruptDispatcher() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable);
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::trigger() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    if(!_isActive) return ver;

    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(var.second.get());
        assert(numericAddressAsyncVariable);
        numericAddressAsyncVariable->fillSendBuffer(ver);
        var.second->send(); // send function from  the AsyncVariable base class
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }

    return ver;
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::activate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger. Must be generated under mutex
    try {
      _transferGroup->read();

      for(auto& var : _asyncVariables) {
        auto* numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(var.second.get());
        assert(numericAddressAsyncVariable);
        numericAddressAsyncVariable->fillSendBuffer(ver);
        var.second->activateAndSend(); // function from  the AsyncVariable base class
      }
      _isActive = true;
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that
      // raised it.
    }

    return ver;
  }

} // namespace ChimeraTK
