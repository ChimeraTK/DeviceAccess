#pragma once

#include "AsyncNDRegisterAccessor.h"
#include "AccessorInstanceDescriptor.h"
#include <mutex>

namespace ChimeraTK {

  /** Typeless base class. The implementations will have a list of all asynchronous
   *  accessors.
   */
  struct AsyncVariable {
    virtual ~AsyncVariable() = default;

    /** Activate all subscribers and send an initial value.
     */
    virtual void activate(VersionNumber const& version) = 0;
    /** Read the synchronous accessor and push the data to all subscribers,
     *  using the specified version number.
     */
    virtual size_t unsubscribe() = 0;
    /** Send an exception to all subscribers. This automatically de-activates then.
     */
    virtual void sendException(std::exception_ptr e) = 0;
    /** Deactivate all subscribers without throwing an exception.
       *  This has to happen when a backend is closed.
       */
    virtual void deactivate() = 0;

    virtual unsigned int getNumberOfChannels() = 0;
    virtual unsigned int getNumberOfSamples() = 0;
    virtual const std::string& getUnit() = 0;
    virtual const std::string& getDescription() = 0;
    virtual bool isWriteable() = 0;
  };

  /** The SubscriptionManager has two main functionalities:
   *  * It manages the subscription/unsubscription mechanism
   *  * It calls functions for all asynchronous accessors associated with one interrupt
   *  * It serves as a subscription manager
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class has implements a lock so
   *  dispatching an interrupt is safe against concurrent subscriptions/unsibscriptions.
   */
  class AsyncAccessorManager : public boost::enable_shared_from_this<AsyncAccessorManager> {
   public:
    /** Request a new subscription. This function internally creates the correct asynchronous accessor
     *  and registers it. If it is the first accessor for that register with the same parameters (offset, size, UserType and raw mode)
     *  it will internally create the matching AsyncVariable.
     */
    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(boost::shared_ptr<DeviceBackend> backend,
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /** This function must only be called from the destructor of the AsyncNDRegisterAccessor which is created in the subscribe function!
     */
    void unsubscribe(AccessorInstanceDescriptor const& descriptor);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(std::exception_ptr e);

    /** Activate all accessors and send the initial value. Generates a new version number which is used for
     *  all initial values and  which can be read out with getLastVersion().
     */
    VersionNumber activate();

    /** Deactivate all subscribers without throwing an exception.
     *  This has to happen when a backend is closed.
     */
    void deactivate();

   protected:
    // Helper function for the implementing to execute something for each AsyncVariable.
    // This function first locks the mutex and then passes a reference to each AsyncVariable to the
    // calling function.
    template<typename FunctionType, typename... FunctionArgs>
    void executeForEach(FunctionType function, FunctionArgs... args) {
      std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);
      for(auto& var : _asyncVariables) {
        function(var.second, args...);
      }
    }

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable,
        std::unique_ptr<AsyncVariable>(boost::shared_ptr<DeviceBackend>, AccessorInstanceDescriptor const&, bool));

   private:
    std::recursive_mutex _variablesMutex;

    std::map<AccessorInstanceDescriptor, std::unique_ptr<AsyncVariable>> _asyncVariables;

    bool _isActive{false};
  };

  /** This class provides parts the implementation of AsyncVariable. It still is a base class with has pure virtual functions, but
   *  at the same time provides some final implementations of virtuals functions from AsyncVariable.
   */
  template<typename UserType>
  struct AsyncVariableImpl : public AsyncVariable {
    /** Implementation that activates all subscribers and sends the initial value (complete buffer with version number and data validity).
     *  This function has to be called by all activate implementations to distribute the data to the subscribers.
     *
     *  The initial value is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.
     */
    void activateImpl(typename NDRegisterAccessor<UserType>::Buffer& initialValue);

    /** Implementation that activates all subscribers and sends the initial value (complete buffer with version number and data validity).
     *  This function has to be called by all activate implementations to distribute the data to the subscribers.
     *
     *  The initial value is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.
     */
    void dispatch(typename NDRegisterAccessor<UserType>::Buffer& initialValue);

    /** Add an asynchronous accessor to the list of subscribers. If the variable is activated
     *  the subscribed accessor is immediately activated and will get its initial value.
     */
    void subscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> newSubscriber);

    AsyncVariableImpl(bool isActive, size_t nChannels, size_t nElements);

    size_t unsubscribe() final;
    void sendException(std::exception_ptr e) final;
    void deactivate() final;
    void activate(VersionNumber const& version) final;

   protected:
    virtual typename NDRegisterAccessor<UserType>::Buffer getInitialValue(VersionNumber const& versionNumber) = 0;

    std::list<boost::weak_ptr<AsyncNDRegisterAccessor<UserType>>> _subscribers;

    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
    std::atomic<bool> _isActive{false};

    /** Helper function which loops all subscribers. It includes a copy minimisation
     *  (the last subscriber is swapped instead of copied).
     *  Inside of the loop it calls the templated labda function. Used in activate and dispatch.
     */
    template<typename Function>
    void executeWithCopy(Function function, typename std::vector<std::vector<UserType>>& value,
        VersionNumber versionNumber, DataValidity dataValidity);
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  template<typename UserType>
  size_t AsyncVariableImpl<UserType>::unsubscribe() {
    for(auto it = _subscribers.begin(); it != _subscribers.end(); ++it) {
      // This code is called from the destructor of an AsyncNDRegisterAccessor inside a boost::shared_ptr.
      // When this code is called the weak_ptr is already not lockable any more. We just use this to identify
      // which element is to be removed. If we get the wrong one it does not matter because then the other destructor will get it.
      if(it->lock().get() == nullptr) {
        _subscribers.erase(it);
        return _subscribers.size();
      }
    }

    std::cerr << "AsyncVariable::unsubscribe must only be called from the destructor of an AsyncDNRegisterAccessor!"
              << std::endl;
    assert(false);
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::activate(VersionNumber const& version) {
    auto initialValue = getInitialValue(version);
    executeWithCopy([](boost::shared_ptr<AsyncNDRegisterAccessor<UserType>>& accessor,
                        typename NDRegisterAccessor<UserType>::Buffer& buf) { accessor->activate(buf); },
        initialValue.value, initialValue.versionNumber, initialValue.dataValidity);
    _isActive = true;
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
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
  AsyncVariableImpl<UserType>::AsyncVariableImpl(bool isActive, size_t nChannels, size_t nElements)
  : _sendBuffer(nChannels, nElements), _isActive(isActive) {}

  //*********************************************************************************************************************/
  template<typename UserType>
  template<typename Function>
  void AsyncVariableImpl<UserType>::executeWithCopy(Function function,
      typename std::vector<std::vector<UserType>>& value, VersionNumber versionNumber, DataValidity dataValidity) {
    assert(!_subscribers.empty());

    for(auto it = _subscribers.begin(); it != --(_subscribers.end()); ++it) {
      auto subscriber = it->lock();
      if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
        // Do the copy here
        // Don't use operator =, this is a move operator which swaps
        _sendBuffer.value = value;
        _sendBuffer.dataValidity = dataValidity;
        _sendBuffer.versionNumber = versionNumber;

        function(subscriber, _sendBuffer);
      }
    }
    auto subscriber = _subscribers.back().lock();
    _sendBuffer.value.swap(value);
    _sendBuffer.dataValidity = dataValidity;
    _sendBuffer.versionNumber = versionNumber;
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      // on the last element we give away the input buffer
      function(subscriber, _sendBuffer);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::subscribe(boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> newSubscriber) {
    _subscribers.push_back(newSubscriber);
    if(_isActive) {
      auto initialValue = getInitialValue({}); // {} is the a new version number
      newSubscriber->activate(initialValue);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::deactivate() {
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
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> AsyncAccessorManager::subscribe(
      boost::shared_ptr<DeviceBackend> backend, RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    if(!_asyncVariables[descriptor].get()) {
      // Variable does not exist yet. Create it.
      _asyncVariables[descriptor] =
          CALL_VIRTUAL_FUNCTION_TEMPLATE(createAsyncVariable, UserType, backend, descriptor, _isActive);
    }

    auto asyncVariable = dynamic_cast<AsyncVariableImpl<UserType>*>(_asyncVariables[descriptor].get());
    // we just take all the information we need for the async accessor from the sync accessor which has already done all the parsing
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType>>(backend, shared_from_this(), name,
        asyncVariable->getNumberOfChannels(), asyncVariable->getNumberOfSamples(), flags, descriptor,
        asyncVariable->getUnit(), asyncVariable->getDescription());
    // Set the exception backend here. It might be that the accessor is already activated during subscription, and the backend shoud be set at that point
    newSubscriber->setExceptionBackend(backend);

    if(asyncVariable->isWriteable()) {
      // for writeable variables we add another synchronous accessors (which knows how to access the data) to the asyncAccessor (which is generic)
      auto synchronousFlags = flags;
      synchronousFlags.remove(AccessMode::wait_for_new_data);

      // Don't call backend->getSyncRegisterAccessor() here. It might skip the overriding of a backend.
      newSubscriber->setWriteAccessor(
          backend->getRegisterAccessor<UserType>(name, numberOfWords, wordOffsetInRegister, synchronousFlags));
    }

    asyncVariable->subscribe(newSubscriber);
    return newSubscriber;
  }

} // namespace ChimeraTK
