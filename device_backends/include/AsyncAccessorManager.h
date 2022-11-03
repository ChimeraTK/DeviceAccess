// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncNDRegisterAccessor.h"

#include <mutex>

namespace ChimeraTK {

  /** Typeless base class. The implementations will have a weak pointer of an AsyncNDRegisterAccessor<UserType>
   *  and will implement the pure virtual functions which act on the accessor.
   */
  struct AsyncVariable {
    virtual ~AsyncVariable() = default;

    /** Activate the AsyncNDRegisterAccessor and send an initial value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     *  The buffer is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.

     */
    virtual void activateAndSend() = 0;

    /** Send the value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     *  The buffer is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.
     */
    virtual void send() = 0;

    /** Send an exception to all subscribers. This automatically de-activates then.
     */
    virtual void sendException(std::exception_ptr e) = 0;
    /** Deactivate all subscribers without throwing an exception.
     *  This has to happen when a backend is closed.
     */
    virtual void deactivate() = 0;

    /** Helper functions for the creation of an AsyncNDRegisterAccessor. As the creating code cannot use
     *  the catalogue, each backend has to implement these functions appropriately.
     */
    virtual unsigned int getNumberOfChannels() = 0;
    virtual unsigned int getNumberOfSamples() = 0;
    virtual const std::string& getUnit() = 0;
    virtual const std::string& getDescription() = 0;
    virtual bool isWriteable() = 0;
  };

  /** Helper class to have a complete descriton to create an Accessor.
   *  It contains all the information given to DeviceBackend:getNDRegisterAccessor, incl. the offset in the
   *  register which is not known to the accessor itself. Just just to keep the number of parameters for
   * createAsyncVariable in check.
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
  };

  /** The AsyncAccessorManager has three main functionalities:
   *  * It manages the subscription/unsubscription mechanism. During subscription an AsyncNDRegisterAccessor is
   *    created, so the manager also serves as a factory for the asynchronous accessors.
   *  * The manager provides functions for all asynchronous accessors associated with one interrupt
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class implements a lock so
   *  dispatching an interrupt is safe against concurrent subscriptions/unsubscriptions.
   *
   *  The AsyncAccessorManager has some pure virtual functions. The implementation is backend
   *  specific and must be provided by a derived version of the AsyncAccessorManager.
   */
  class AsyncAccessorManager : public boost::enable_shared_from_this<AsyncAccessorManager> {
   public:
    virtual ~AsyncAccessorManager() = default;

    /** Request a new subscription. This function internally creates the correct asynchronous accessor and a matching
     * AsyncVariable. A weak pointer to the AsyncNDRegisterAccessor is registered in the AsyncVariable, and a shared
     * pointer is returned to the calling code.
     */
    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(boost::shared_ptr<DeviceBackend> backend,
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /** This function must only be called from the destructor of the AsyncNDRegisterAccessor which is created in the
     * subscribe function!
     */
    void unsubscribe(TransferElementID id);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(std::exception_ptr e);

    /** Activate all accessors and send the initial value. Generates a new version number which is used for
     *  all initial values and  which can be read out with getLastVersion().
     *  This function has to be provided by each AsyncAccessorManager implementation.
     */
    virtual VersionNumber activate() = 0;

    /** Deactivate all subscribers without throwing an exception.
     *  This has to happen when a backend is closed.
     */
    void deactivate();

   protected:
    /*** Each implementation must provide a function to create specific AsyncVariables.
     *   When the variable is returned, async accessor is not set yet. This would leave the whole creation
     *   of the async accessor to the backend, would mean a lot of code duplication. It also cannot be
     *   retrieved from the AsyncVariable as this only contains a weak pointer.
     *   If the isActive flag is set, the _sendBuffer must already contain the initial value. The variable itself
     *   is not activated yet as the async accessor is still not set.
     */
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(createAsyncVariable,
        std::unique_ptr<AsyncVariable>(
            boost::shared_ptr<DeviceBackend>, AccessorInstanceDescriptor const&, bool isActive));

    // This mutex protects the _asyncVariables container and all its contents, and the _isActive flag. You must not
    // touch those variables without holding the mutex. It serves two purposes:
    // 1. Variables can be added or removed from the container at any time. It is not safe to handle an element without
    // holding the lock.
    // 2. The elements in the container are not thread-safe as well. We use the same lock as it is needed for 1. anyway.
    std::recursive_mutex _variablesMutex;
    std::map<TransferElementID, std::unique_ptr<AsyncVariable>> _asyncVariables; ///< protected by _variablesMutex
    bool _isActive{false};                                                       ///< protected by _variablesMutex
    unsigned _usageCount = 0;                                                    // number of asyncVariables in map

    /// this virtual function lets derived classes react on subscribe / unsubscribe
    virtual void usageCountChanged() {}
  };

  /** AsyncVariableImpl contains a weak pointer to an AsyncNDRegisterAccessor<UserType> and a send buffer
   * NDRegisterAccessor<UserType>::Buffer. This class provides implementation for those functions of the AsyncVariable
   * which touch the AsyncNDRegisterAccessor. It does implement the functions which provide the information needed to
   * create an AsyncNDRegisterAccessor, like getNumberOfChannels(). These are backend specific and need a dedicated
   * implementation per backend.
   *
   *  The AsyncVariableManager cannot hold a shared pointer of the AsyncNDRegisterAccessor because then you could never
   * get rid of a created accessor, which means the manager would just keep growing in memory if accessors are
   * dynamically created and removed. Hence a weak pointer is used, and this class provides all the functions that
   * access this weak pointer and to all the locking and nullptr checking.
   */
  template<typename UserType>
  struct AsyncVariableImpl : public AsyncVariable {
    AsyncVariableImpl(size_t nChannels, size_t nElements);

    void send() final;
    void sendException(std::exception_ptr e) final;
    void deactivate() final;
    void activateAndSend() final;

   private:
    // This weak pointer is private so the user cannot bypass correct locking and nullptr-checking.
    boost::weak_ptr<AsyncNDRegisterAccessor<UserType>> _asyncAccessor;
    friend AsyncAccessorManager; // is allowed to set the _asyncAccessor

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
    // _isActive is false. You have to activate it after creation when necessary. Reason: The AsyncNDRegisterAccessor is
    // not set, and the send buffer needs to be filled before so we can send the initial value. As the latter is backend
    // specific it cannot happen here, because this code is executed first.
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
    std::unique_lock<std::recursive_mutex> variablesLock(_variablesMutex);

    AccessorInstanceDescriptor descriptor(name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags);
    auto untypedAsyncVariable =
        CALL_VIRTUAL_FUNCTION_TEMPLATE(createAsyncVariable, UserType, backend, descriptor, _isActive);

    auto asyncVariable = dynamic_cast<AsyncVariableImpl<UserType>*>(untypedAsyncVariable.get());
    // we take all the information we need for the async accessor from AsyncVariable because we cannot use the catalogue
    // here
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType>>(backend, shared_from_this(), name,
        asyncVariable->getNumberOfChannels(), asyncVariable->getNumberOfSamples(), flags, asyncVariable->getUnit(),
        asyncVariable->getDescription());
    // Set the exception backend here. It might be that the accessor is already activated during subscription, and the
    // backend should be set at that point
    newSubscriber->setExceptionBackend(backend);

    if(asyncVariable->isWriteable()) {
      // for writeable variables we add another synchronous accessors (which knows how to access the data) to the
      // asyncAccessor (which is generic)
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
    _usageCount++;
    variablesLock.unlock(); // unlock before call to overwritten method
    usageCountChanged();
    return newSubscriber;
  }

} // namespace ChimeraTK
