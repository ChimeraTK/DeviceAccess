#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include <mutex>

namespace ChimeraTK {

  class NumericAddressedInterruptDispatcher;

  // typeless base class
  struct NumericAddressedAsyncVariable {
    virtual void trigger(VersionNumber const& version) = 0;
    virtual void activate(VersionNumber const& version) = 0;
    virtual ~NumericAddressedAsyncVariable() = default;
    // returns the number of remaining subscribers
    virtual size_t unsubscribe();
  };

  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public NumericAddressedAsyncVariable {
    size_t unsubscribe() override {
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

    void trigger(VersionNumber const& version) override {
      executeWithCopy(
          [](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>& accessor,
              VersionNumber const& v) { accessor->sendDestructively(v); },
          version);
    }

    void activate(VersionNumber const& version) override {
      executeWithCopy(
          [](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>& accessor,
              VersionNumber const& v) { accessor->activate(v); },
          version);
    }

    template<typename Function>
    void executeWithCopy(Function& function, VersionNumber const& version) {
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
        for(auto& it : subscribers) {
          auto subscriber = it->lock();
          if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
            subscriber.sendException(std::current_exception());
          }
        }
      }
    }

    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>>> subscribers;

   protected:
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher>> syncAccessor;
    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
  }; // namespace ChimeraTK

  class NumericAddressedInterruptDispatcher
  : public boost::enable_shared_from_this<NumericAddressedInterruptDispatcher> {
   public:
    NumericAddressedInterruptDispatcher(boost::shared_ptr<NumericAddressedBackend> b) : _backend(b) {}

    template<typename UserType>
    void subscribe(RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      std::lock_guard<std::mutex> variablesLock(_variablesMutex);

      auto asyncVariable =
          std::dynamic_pointer_cast<NumericAddressedAsyncVariableImpl<UserType>>(_asyncVariables[name]);
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
              asyncVariable.syncAccessor->getNumberOfChannels(), asyncVariable.syncAccessor->getNumberOfElements(),
              flags, asyncVariable.syncAccessor->getUnit(), asyncVariable.syncAccessor->getDescription())));
    }

    void unsubscribe(RegisterPath name) {
      std::lock_guard<std::mutex> variablesLock(_variablesMutex);
      auto varIter = _asyncVariables.find(name);
      if(varIter == _asyncVariables.end()) {
        throw ChimeraTK::logic_error("NumericAddressedInterruptDispatcher: cannot unsubscribe register " + name +
            " because it is not subscribed");
      }
      auto& var = varIter->second;  // the iterator of a map is a key-value pair
      if(var->unsubscribe() == 0) { // no subscribers left if it returns 0
        _asyncVariables.erase(varIter);
      }
    }

    void trigger() {
      std::lock_guard<std::mutex> variablesLock(_variablesMutex);
      VersionNumber ver; // a common VersionNumber for this trigger
      for(auto& var : _asyncVariables) var.second->trigger(ver);
      _lastVersion = ver; // only set _lastVersion after all variables have been triggered
    }

    VersionNumber getLastVersion() {
      // unfortunately we need a lock because VersionNumber cannot be made atomic as it is not trivially copyable
      std::lock_guard<std::mutex> variablesLock(_variablesMutex);
      return _lastVersion;
    }

   private:
    std::mutex _variablesMutex;
    std::map<RegisterPath, std::unique_ptr<NumericAddressedAsyncVariable>> _asyncVariables;
    VersionNumber _lastVersion;
    boost::shared_ptr<NumericAddressedBackend> _backend;
  };
} // namespace ChimeraTK
