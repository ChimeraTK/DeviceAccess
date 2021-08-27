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

    /** Activate all subscribers and send an initial value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     */
    virtual void activateAndSend() = 0;

    /** Send the value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     */
    virtual void send() = 0;

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
    void unsubscribe(TransferElementID id);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(std::exception_ptr e);

    /** Activate all accessors and send the initial value. Generates a new version number which is used for
     *  all initial values and  which can be read out with getLastVersion().
     *
     *  It calls perpareActive before iterating the list of accessors.
     */
    virtual VersionNumber activate() = 0;

    /** Actions to be taken before activate() is called on all AsyncVariables.
     *  Returns whether the preparation was successful. Only in success the activation takes place.
     *
     *  Example: The NumericAddressedBackend is executing read() on a TransferGroup to get all initial values.
     *  If an exception occurs, this function returns false and the activation does not take place.
     */
    //virtual bool prepareActivate([[maybe_unused]] VersionNumber const& v) { return true; }

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

    /*** Each implementation must provice a function to create specific AsyncVariables.
     *   When the variable is returned, async accessor is not set yet. This would leave the whole creation
     *   of the async accessor to the backend, would mean a lot of code duplication. It also cannot be
     *   retreived from the AsyncVariable as this only contains a waek pointer.
     *   If the isActiv flag is set, the _sendBuffer must already contain the initial value. The variable itself
     *   is not activated yet as the async accessor is still not set.
     */
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable,
        std::unique_ptr<AsyncVariable>(
            boost::shared_ptr<DeviceBackend>, AccessorInstanceDescriptor const&, bool isActive));

    // This mutext protects the _asyncVariables container and all its contents, and the _isActive flag. You must not touch those variables
    // without holding the mutex. It serves two pruposes:
    // 1. Variables can be added or removed from the container at any time. It is not safe to handle an element without holding the lock.
    // 2. The elements in the container are not thread-safe as well. We use the same lock as it is needed for 1. anyway.
    std::recursive_mutex _variablesMutex;
    std::map<TransferElementID, std::unique_ptr<AsyncVariable>> _asyncVariables; ///< protected by _variablesMutex
    bool _isActive{false};                                                       ///< protected by _variablesMutex
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
    void send() final;

    AsyncVariableImpl(size_t nChannels, size_t nElements);

    void sendException(std::exception_ptr e) final;
    void deactivate() final;
    void activateAndSend() final;

   private:
    // This weak pointer is private so the user cannot bypass correct locking and nullptr-checking.
    boost::weak_ptr<AsyncNDRegisterAccessor<UserType>> _asyncAccessor;
    friend AsyncAccessorManager; // is allowed to set the variable

   protected:
    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;
    std::atomic<bool> _isActive{false};
  };

  //*********************************************************************************************************************/
  // Implementations
  //*********************************************************************************************************************/

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::activateAndSend() {
    // The initial value must have been set before calling this function. We can just send it.
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->activate(_sendBuffer);
      _isActive = true;
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
    _isActive = false;
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->sendException(e);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  AsyncVariableImpl<UserType>::AsyncVariableImpl(size_t nChannels, size_t nElements)
  : _sendBuffer(nChannels, nElements) {
    // _isActive is false. You have to activate it after creation when necessary. Reason: the send buffer needs to
    // be filled before so we can send the initial value. As this is backend specific it cannot happen here, because
    // this code is executed first.
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::deactivate() {
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->deactivate();
    }
    _isActive = false;
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::send() {
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Possible race condition: The subscriber is being destructed.
      subscriber->sendDestructively(_sendBuffer);
    }
  }

  //*********************************************************************************************************************/
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> AsyncAccessorManager::subscribe(
      boost::shared_ptr<DeviceBackend> backend, RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister,
      AccessModeFlags flags) {
    std::lock_guard<std::recursive_mutex> variablesLock(_variablesMutex);

    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    auto untypedAsyncVariable =
        CALL_VIRTUAL_FUNCTION_TEMPLATE(createAsyncVariable, UserType, backend, descriptor, _isActive);

    auto asyncVariable = dynamic_cast<AsyncVariableImpl<UserType>*>(untypedAsyncVariable.get());
    // we take all the information we need for the async accessor from AsyncVariable because we cannot use the catalogue here
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

    asyncVariable->_asyncAccessor = newSubscriber;
    // Now that the AsyncVariable is complete we can finally activate it.
    if(_isActive) {
      asyncVariable->activateAndSend();
    }
    _asyncVariables[newSubscriber->getId()] = std::move(untypedAsyncVariable);
    return newSubscriber;
  }

} // namespace ChimeraTK
