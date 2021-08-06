#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include <mutex>

namespace ChimeraTK {

  class NumericAddressedInterruptDispatcher;

  // typeless base class
  struct NumericAddressedAsyncVariable {
    virtual ~NumericAddressedAsyncVariable() = default;

    virtual void activate(VersionNumber const& version) = 0;
    virtual void trigger(VersionNumber const& version) = 0;
    // returns the number of remaining subscribers
    virtual size_t unsubscribe();
    virtual void sendException(std::exception_ptr& e) = 0;
  };

  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public NumericAddressedAsyncVariable {
    void activate(VersionNumber const& version) override;
    void trigger(VersionNumber const& version) override;
    size_t unsubscribe() override;
    void sendException(std::exception_ptr& e) override;

    template<typename Function>
    void executeWithCopy(Function& function, VersionNumber const& version);

    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>> subscribers;

   protected:
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>> syncAccessor;
    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
  }; // namespace ChimeraTK

  class NumericAddressedInterruptDispatcher
  : public boost::enable_shared_from_this<NumericAddressedInterruptDispatcher> {
   public:
    NumericAddressedInterruptDispatcher(boost::shared_ptr<NumericAddressedBackend> b);

    template<typename UserType>
    void subscribe(RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    void unsubscribe(RegisterPath name);

    void trigger();

    VersionNumber getLastVersion();

   private:
    std::mutex _variablesMutex;
    std::map<RegisterPath, std::unique_ptr<NumericAddressedAsyncVariable>> _asyncVariables;
    VersionNumber _lastVersion;
    boost::shared_ptr<NumericAddressedBackend> _backend;
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  template<typename UserType>
  size_t NumericAddressedAsyncVariableImpl<UserType>::unsubscribe() {
    for(auto it = subscribers.begin(); it != subscribers.end(); ++it) {
      // This code is called from the destructor of an AsyncNDRegisterAccessor inside a boost::shared_ptr.
      // When this code is called the weak_ptr is already not lockable any more. We just use this to identify
      // which element is to be removed. If we get the wrong one it does not matter because then the other destructor will get it.
      if(it->lock().get() == nullptr) {
        subscribers.erase(it);
        return subscribers.size();
      }

      throw ChimeraTK::logic_error("NumericAddressedAsyncVariable::unsibscribe must only be called from the "
                                   "destructor of an AsyncDNRegisterAccessor!");
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::trigger(VersionNumber const& version) {
    executeWithCopy(
        [](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>& accessor,
            VersionNumber const& v) { accessor->sendDestructively(v); },
        version);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::activate(VersionNumber const& version) {
    executeWithCopy(
        [](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>& accessor,
            VersionNumber const& v) { accessor->activate(v); },
        version);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::sendException(std::exception_ptr& e) {
    for(auto& it : subscribers) {
      auto subscriber = it->lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        subscriber.sendException(e);
      }
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  template<typename Function>
  void NumericAddressedAsyncVariableImpl<UserType>::executeWithCopy(Function& function, VersionNumber const& version) {
    assert(!subscribers.empty());
    assert(syncAccessor);
    try {
      syncAccessor.read();
      _sendBuffer.value.swap(syncAccessor->accessChannels());
      _sendBuffer.dataValidity = syncAccessor->getDataValidity();
      _sendBuffer.versionNumber = version;

      for(auto& it = subscribers.begin(); it != --(subscribers.end()); ++it) {
        auto subscriber = it->lock();
        if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
          function(subscriber, _sendBuffer);
        }
      }
      auto subscriber = subscribers.back().lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        function(subscriber, _sendBuffer);
      }
    }
    catch(ChimeraTK::runtime_error&) {
      sendException(std::current_exception());
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedInterruptDispatcher::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard<std::mutex> variablesLock(_variablesMutex);

    auto asyncVariable = std::dynamic_pointer_cast<NumericAddressedAsyncVariableImpl<UserType>>(_asyncVariables[name]);
    asert(asyncVariable);
    if(!asyncVariable.syncAccessor) {
      // The async variable has just been created and there are no subscribers and no synchronous accessor yet.
      auto synchronousFlags = flags;
      synchronousFlags.remove(AccessMode::wait_for_new_data);
      asyncVariable.syncAccessor =
          _backend->getSyncRegisterAccessor<UserType>(name, numberOfWords, wordOffsetInRegister, synchronousFlags);
    }
    // we just take all the information we need for the async accessor from the sync accessor which has already done all the parsing
    asyncVariable.subscribers.push_back(boost::make_shared(
        AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>(_backend, shared_from_this(), name,
            asyncVariable.syncAccessor->getNumberOfChannels(), asyncVariable.syncAccessor->getNumberOfElements(), flags,
            asyncVariable.syncAccessor->getUnit(), asyncVariable.syncAccessor->getDescription())));
  }

} // namespace ChimeraTK
