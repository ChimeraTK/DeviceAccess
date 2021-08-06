#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::unsubscribe(RegisterPath name) {
    std::lock_guard<std::mutex> variablesLock(_variablesMutex);
    auto varIter = _asyncVariables.find(name);
    if(varIter == _asyncVariables.end()) {
      throw ChimeraTK::logic_error(
          "NumericAddressedInterruptDispatcher: cannot unsubscribe register " + name + " because it is not subscribed");
    }
    auto& var = varIter->second;  // the iterator of a map is a key-value pair
    if(var->unsubscribe() == 0) { // no subscribers left if it returns 0
      _asyncVariables.erase(varIter);
    }
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::trigger() {
    std::lock_guard<std::mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger
    for(auto& var : _asyncVariables) var.second->trigger(ver);
    _lastVersion = ver; // only set _lastVersion after all variables have been triggered
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::getLastVersion() {
    // unfortunately we need a lock because VersionNumber cannot be made atomic as it is not trivially copyable
    std::lock_guard<std::mutex> variablesLock(_variablesMutex);
    return _lastVersion;
  }

} // namespace ChimeraTK
