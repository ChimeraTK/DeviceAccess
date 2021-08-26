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
      _transferGroup.read();

      for(auto& var : _asyncVariables) {
        auto numericAddressAsyncVariable = dynamic_cast<NumericAddressedAsyncVariable*>(var.second.get());
        assert(numericAddressAsyncVariable);
        numericAddressAsyncVariable->trigger(ver);
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. Backend's set exception has already been called by the accessor in the transfer group that raised it.
    }

    return ver;
  }

  bool NumericAddressedInterruptDispatcher::prepareActivate([[maybe_unused]] VersionNumber const& v) {
    try {
      _transferGroup.read();
    }
    catch(ChimeraTK::runtime_error&) {
      return false;
    }
    return true;
  }

} // namespace ChimeraTK
