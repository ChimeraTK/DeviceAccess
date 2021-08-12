#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "NumericAddressedBackendRegisterAccessor.h"
#include <mutex>

namespace ChimeraTK {

  /** Typeless base class. The implementations will have a list of all asynchronous
   *  accessors and one synchrounous accessor.
   */
  struct NumericAddressedAsyncVariable {
    virtual ~NumericAddressedAsyncVariable() = default;

    /** Activate all subscribers and send an initial value.
     */
    virtual void activate(VersionNumber const& version) = 0;
    /** Read the synchronous accessor and push the data to all subscribers,
     *  using the specified version number.
     */
    virtual void trigger(VersionNumber const& version) = 0;
    /** Unsubscribes the first invalid subscriber.
     *  Returns the number of remaining subscribers
     */
    virtual size_t unsubscribe() = 0;
    /** Send an exception to all subscribers. This automatically de-activates then.
     */
    virtual void sendException(std::exception_ptr e) = 0;
    /** Deactivate all subscribers without throwing an exception.
       *  This has to happen when a backend is closed.
       */
    virtual void deactivate() = 0;
  };

  /** The NumericAddressedInterruptDispatcher has two main functionalities:
   *  * It calls functions for all asynchronous accessors associated with one interrupt
   *  * It serves as a subscription manager
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class has implements a lock so
   *  dispatching an interrupt is safe against concurrent subscriptions/unsibscriptions.
   */
  class NumericAddressedInterruptDispatcher
  : public boost::enable_shared_from_this<NumericAddressedInterruptDispatcher> {
   public:
    /** Helper class to have a complete key to distinguish all accessors.
     *  The asynchronous variables contain the typed synchronous reader which has a UserType, user size and offset and might be raw or cooked,
     * so we need this helper object to have a complete description.
     */
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

    /** Request a new subscription. This function internally creates the correct asynchronous accessor
     *  and registers it. If it is the first accessor for that register with the same parameters (offset, size, UserType and raw mode)
     *  it will internally create the matching NumericAddressedAsyncVariable.
     */
    template<typename UserType>
    boost::shared_ptr<
        AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher, AccessorInstanceDescriptor>>
        subscribe(boost::shared_ptr<NumericAddressedBackend> backend, RegisterPath name, size_t numberOfWords,
            size_t wordOffsetInRegister, AccessModeFlags flags);

    /** Trigger all NumericAddressedAsyncVariables that are stored in this dispatcher. Creates a new VersionNumber and sends
     *  all data with this version.
     */
    void trigger();

    /** Allows a backend to get the last version number that was  send by this interrupt dispatcher. Usually only needed by dummies and for testing.
     */
    VersionNumber getLastVersion();

    /** This function must only be called from the destructor of the AsyncNDRegisterAccessor which is created in the subscribe function!
     */
    void unsubscribe(AccessorInstanceDescriptor const& descriptor);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(std::exception_ptr e);

    /** Activate all accessors and send the initial value. Generates a new version number which is used for
     *  all initial values and  which can be read out with getLastVersion().
     */
    void activate();

    /** Deactivate all subscribers without throwing an exception.
     *  This has to happen when a backend is closed.
     */
    void deactivate();

   private:
    std::recursive_mutex _variablesMutex;

    std::map<AccessorInstanceDescriptor, std::unique_ptr<NumericAddressedAsyncVariable>> _asyncVariables;

    VersionNumber _lastVersion;
    bool _isActive{false};
  };

  /** Implementation of the NumericAddressedAsyncVariable for the concrete UserType.
   */
  template<typename UserType>
  struct NumericAddressedAsyncVariableImpl : public NumericAddressedAsyncVariable {
    void activate(VersionNumber const& version) override;
    void trigger(VersionNumber const& version) override;
    size_t unsubscribe() override;
    void sendException(std::exception_ptr e) override;

    /** Helper function which loops all subscribers. It includes a copy minimisation
     *  (the last subscriber is swapped instead of copied).
     *  Inside of the loop it calls the templated labda function. Used in activate and trigger.
     */
    template<typename Function>
    void executeWithCopy(Function function, VersionNumber const& version);

    /** The constructor takes an already created synchronous accessor and a flag
     *  whether the variable is active. If the variable is active all new subscribers will automatically
     *  be activated and immediately get their initial value.
     */
    NumericAddressedAsyncVariableImpl(boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, bool isActive);

    /** Add an asynchronous accessor to the list of subscribers. If the variable is activated
     *  the subscribed accessor is immediately activated and will get its initial value.
     */
    void subscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
            NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>
            newSubscriber);

    boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor;

    /** Deactivate all subscribers without throwing an exception.
     *  This has to happen when a backend is closed.
     */
    void deactivate() override;

   protected:
    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
        NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>>
        _subscribers;

    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
    std::atomic<bool> _isActive{false};
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  template<typename UserType>
  size_t NumericAddressedAsyncVariableImpl<UserType>::unsubscribe() {
    for(auto it = _subscribers.begin(); it != _subscribers.end(); ++it) {
      // This code is called from the destructor of an AsyncNDRegisterAccessor inside a boost::shared_ptr.
      // When this code is called the weak_ptr is already not lockable any more. We just use this to identify
      // which element is to be removed. If we get the wrong one it does not matter because then the other destructor will get it.
      if(it->lock().get() == nullptr) {
        _subscribers.erase(it);
        return _subscribers.size();
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
    _isActive = true;
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
    _isActive = false;
    for(auto& it : _subscribers) {
      auto subscriber = it.lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        subscriber->sendException(e);
      }
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  NumericAddressedAsyncVariableImpl<UserType>::NumericAddressedAsyncVariableImpl(
      boost::shared_ptr<NDRegisterAccessor<UserType>> syncAccessor_, bool isActive)
  : syncAccessor(syncAccessor_), _sendBuffer(syncAccessor->getNumberOfChannels(), syncAccessor->getNumberOfSamples()),
    _isActive(isActive) {}

  //*********************************************************************************************************************/
  template<typename UserType>
  template<typename Function>
  void NumericAddressedAsyncVariableImpl<UserType>::executeWithCopy(Function function, VersionNumber const& version) {
    assert(!_subscribers.empty());
    assert(syncAccessor);
    try {
      syncAccessor->read();
      _sendBuffer.value.swap(syncAccessor->accessChannels());
      _sendBuffer.dataValidity = syncAccessor->dataValidity();
      _sendBuffer.versionNumber = version;

      for(auto it = _subscribers.begin(); it != --(_subscribers.end()); ++it) {
        auto subscriber = it->lock();
        if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
          function(subscriber, _sendBuffer);
        }
      }
      auto subscriber = _subscribers.back().lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        function(subscriber, _sendBuffer);
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // no action needed. The synchronous read() already triggered backend->setException();
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::subscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType,
          NumericAddressedInterruptDispatcher, NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>
          newSubscriber) {
    _subscribers.push_back(newSubscriber);
    if(_isActive) {
      try {
        syncAccessor->read();
        _sendBuffer.value.swap(syncAccessor->accessChannels());
        _sendBuffer.dataValidity = syncAccessor->dataValidity();
        _sendBuffer.versionNumber = syncAccessor->getVersionNumber();
        newSubscriber->activate(_sendBuffer);
      }
      catch(ChimeraTK::runtime_error&) {
        // no action needed. The synchronous read() already triggered backend->setException();
      }
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void NumericAddressedAsyncVariableImpl<UserType>::deactivate() {
    for(auto it : _subscribers) {
      auto subscriber = it.lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        subscriber->deactivate();
      }
    }
    _isActive = false;
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>
      NumericAddressedInterruptDispatcher::subscribe(boost::shared_ptr<NumericAddressedBackend> backend,
          RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    if(!_asyncVariables[descriptor].get()) {
      // Variable does not exist yet. Create it.
      auto synchronousFlags = flags;
      synchronousFlags.remove(AccessMode::wait_for_new_data);
      _asyncVariables[descriptor] = std::make_unique<NumericAddressedAsyncVariableImpl<UserType>>(
          backend->getSyncRegisterAccessor<UserType>(name, numberOfWords, wordOffsetInRegister, synchronousFlags),
          _isActive);
    }

    auto asyncVariable = dynamic_cast<NumericAddressedAsyncVariableImpl<UserType>*>(_asyncVariables[descriptor].get());
    // we just take all the information we need for the async accessor from the sync accessor which has already done all the parsing
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType, NumericAddressedInterruptDispatcher,
        NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor>>(backend, shared_from_this(), name,
        asyncVariable->syncAccessor->getNumberOfChannels(), asyncVariable->syncAccessor->getNumberOfSamples(), flags,
        descriptor, asyncVariable->syncAccessor->getUnit(), asyncVariable->syncAccessor->getDescription());
    asyncVariable->subscribe(newSubscriber);
    return newSubscriber;
  }

  //  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedAsyncVariableImpl);
  DECLARE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncNDRegisterAccessor, NumericAddressedInterruptDispatcher,
      NumericAddressedInterruptDispatcher::AccessorInstanceDescriptor);

} // namespace ChimeraTK
