#include "NumericAddressedInterruptDispatcher.h"

namespace ChimeraTK {

  //*********************************************************************************************************************/
  bool NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor::operator<(
      AccessorInstanceDescriptor const& other) const {
    if(name == other.name) {
      if(type == other.type) {
        if(numberOfWords == other.numberOfWords) {
          if(wordOffsetInRegister == other.wordOffsetInRegister)
            return (flags < other.flags);
          else
            return (wordOffsetInRegister < other.wordOffsetInRegister);
        }
        else
          return (numberOfWords < other.numberOfWords);
      }
      else
        return (type < other.type);
    }
    else
      return (name < other.name);
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::unsubscribe(
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor const& descriptor) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    auto varIter = _asyncVariables.find(descriptor);
    if(varIter == _asyncVariables.end()) {
      throw ChimeraTK::logic_error("NumericAddressedInterruptDispatcher: cannot unsubscribe register " +
          descriptor.name + " because it is not subscribed in that configuration.");
    }
    auto& var = varIter->second;  // the iterator of a map is a key-value pair
    if(var->unsubscribe() == 0) { // no subscribers left if it returns 0
      _asyncVariables.erase(varIter);
    }
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::trigger() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for this trigger
    for(auto& var : _asyncVariables) {
      var.second->trigger(ver);
    }
    _lastVersion = ver; // only set _lastVersion after all variables have been triggered
  }

  //*********************************************************************************************************************/
  VersionNumber NumericAddressedInterruptDispatcher::getLastVersion() {
    // unfortunately we need a lock because VersionNumber cannot be made atomic as it is not trivially copyable
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    return _lastVersion;
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::sendException(std::exception_ptr e) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    _isActive = false;
    for(auto& var : _asyncVariables) {
      var.second->sendException(e);
    }
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::activate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    VersionNumber ver; // a common VersionNumber for all variables
    for(auto& var : _asyncVariables) {
      var.second->activate(ver);
    }
    _lastVersion = ver; // only set _lastVersion after all variables have been triggered
    _isActive = true;
  }

  //*********************************************************************************************************************/
  void NumericAddressedInterruptDispatcher::deactivate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    for(auto& var : _asyncVariables) {
      var.second->deactivate();
    }
    _isActive = false;
  }

  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor);

} // namespace ChimeraTK