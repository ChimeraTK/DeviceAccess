#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include <mutex>

namespace ChimeraTK {

  // typeless base class
  struct NumericAddressedAsyncVariable {
    virtual ~NumericAddressedAsyncVariable() = default;

    virtual void activate(VersionNumber const& version) = 0;
    virtual void trigger(VersionNumber const& version) = 0;
    // returns the number of remaining subscribers
    virtual size_t unsubscribe() = 0;
    virtual void sendException(std::exception_ptr e) = 0;
  };

  class NumericAddressedInterruptDispatcher
  : public boost::enable_shared_from_this<NumericAddressedInterruptDispatcher> {
   public:
    // The asynchronous variables contain the typed synchronous reader which has a UserType, user size and offset and might be raw or cooked,
    // so we need a helper object as key for the map which stores all the asynchronous variables
    struct AccessorInstanceDescriptor {
      RegisterPath name;
      std::type_index type;
      size_t numberOfWords;
      size_t wordOffsetInRegister;
      AccessModeFlags flags;
      AccessorInstanceDescriptor(RegisterPath name_, std::type_index type_, size_t numberOfWords_,
          size_t wordOffsetInRegister_, AccessModeFlags flags_)
      : name(name_), type(type_), numberOfWords(numberOfWords_), wordOffsetInRegister(wordOffsetInRegister_),
        flags(flags_) {}
      bool operator<(AccessorInstanceDescriptor const& other) const;
    };

    template<typename UserType>
    boost::shared_ptr<
        AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher, AccessorInstanceDescriptor>>
        subscribe(boost::shared_ptr<NumericAddressedBackend> backend, RegisterPath name, size_t numberOfWords,
            size_t wordOffsetInRegister, AccessModeFlags flags);

    void trigger();

    VersionNumber getLastVersion();

    void unsubscribe(AccessorInstanceDescriptor const& descriptor);

   private:
    std::mutex _variablesMutex;

    std::map<AccessorInstanceDescriptor, std::unique_ptr<NumericAddressedAsyncVariable>> _asyncVariables;

    VersionNumber _lastVersion;
  };

  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public NumericAddressedAsyncVariable {
    void activate(VersionNumber const& version) override;
    void trigger(VersionNumber const& version) override;
    size_t unsubscribe() override;
    void sendException(std::exception_ptr e) override;

    template<typename Function>
    void executeWithCopy(Function function, VersionNumber const& version);

    NumericAddressedAsyncVariableImpl(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_);

    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
        NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>>
        subscribers;
    boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;

   protected:
    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
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
    }

    throw ChimeraTK::logic_error("NumericAddressedAsyncVariable::unsibscribe must only be called from the "
                                 "destructor of an AsyncDNRegisterAccessor!");
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::trigger(VersionNumber const& version) {
    executeWithCopy([](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
                            NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>& accessor,
                        typename NDRegisterAccessor<UserType>::Buffer& buf) { accessor->sendDestructively(buf); },
        version);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::activate(VersionNumber const& version) {
    executeWithCopy([](boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
                            NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>& accessor,
                        typename NDRegisterAccessor<UserType>::Buffer& buf) { accessor->activate(buf); },
        version);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
    for(auto& it : subscribers) {
      auto subscriber = it.lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        subscriber->sendException(e);
      }
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  NumericAddressedAsyncVariableImpl<UserType>::NumericAddressedAsyncVariableImpl(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_)
  : syncAccessor(syncAccessor_), _sendBuffer(syncAccessor->getNumberOfChannels(), syncAccessor->getNumberOfSamples()) {}

  //*********************************************************************************************************************/
  template<typename UserType>
  template<typename Function>
  void NumericAddressedAsyncVariableImpl<UserType>::executeWithCopy(Function function, VersionNumber const& version) {
    assert(!subscribers.empty());
    assert(syncAccessor);
    try {
      syncAccessor->read();
      _sendBuffer.value.swap(syncAccessor->accessChannels());
      _sendBuffer.dataValidity = syncAccessor->dataValidity();
      _sendBuffer.versionNumber = version;

      for(auto it = subscribers.begin(); it != --(subscribers.end()); ++it) {
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
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>
      NumericAddressedInterruptDispatcher::subscribe(boost::shared_ptr<NumericAddressedBackend> backend,
          RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard<std::mutex> variablesLock(_variablesMutex);

    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    if(!_asyncVariables[descriptor].get()) {
      // Variable does not exist yet. Create it.
      auto synchronousFlags = flags;
      synchronousFlags.remove(AccessMode::wait_for_new_data);
      _asyncVariables[descriptor] = std::make_unique<NumericAddressedAsyncVariableImpl<UserType>>(
          backend->getSyncRegisterAccessor<UserType>(name, numberOfWords, wordOffsetInRegister, synchronousFlags));
    }

    auto asyncVariable = dynamic_cast<NumericAddressedAsyncVariableImpl<UserType>*>(_asyncVariables[descriptor].get());
    // we just take all the information we need for the async accessor from the sync accessor which has already done all the parsing
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
        NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>(backend, shared_from_this(), name,
        asyncVariable->syncAccessor->getNumberOfChannels(), asyncVariable->syncAccessor->getNumberOfSamples(), flags,
        descriptor, asyncVariable->syncAccessor->getUnit(), asyncVariable->syncAccessor->getDescription());
    asyncVariable->subscribers.push_back(newSubscriber);
    return newSubscriber;
  }

  //  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedAsyncVariableImpl);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor);

} // namespace ChimeraTK
