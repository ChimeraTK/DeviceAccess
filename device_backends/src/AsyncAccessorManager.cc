#include "AsyncAccessorManager.h"

namespace ChimeraTK {

  //*********************************************************************************************************************/
  bool AccessorInstanceDescriptor::operator<(AccessorInstanceDescriptor const& other) const {
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
  void AsyncAccessorManager::unsubscribe(TransferElementID id) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    // The descrutor of the AsyncVariable implementation must do all necesary clean-up
    _asyncVariables.erase(id);
  }

  //*********************************************************************************************************************/
  void AsyncAccessorManager::sendException(std::exception_ptr e) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    _isActive = false;
    for(auto& var : _asyncVariables) {
      var.second->sendException(e);
    }
  }

  //*********************************************************************************************************************/
  //  VersionNumber AsyncAccessorManager::activate() {
  //    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

  //    VersionNumber ver; // a common VersionNumber for all variables
  //    if(prepareActivate(ver)) {
  //      for(auto& var : _asyncVariables) {
  //        var.second->activateAndSend();
  //      }
  //      _isActive = true;
  //    }

  //    return ver;
  //  }

  //*********************************************************************************************************************/
  void AsyncAccessorManager::deactivate() {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
    for(auto& var : _asyncVariables) {
      var.second->deactivate();
    }
    _isActive = false;
  }

} // namespace ChimeraTK
