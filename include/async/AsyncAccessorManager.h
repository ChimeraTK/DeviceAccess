// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "../VersionNumber.h"
#include "AsyncNDRegisterAccessor.h"

#include <utility>

namespace ChimeraTK::async {

  /** Typeless base class. The implementations will have a weak pointer of an AsyncNDRegisterAccessor<UserType>
   *  and will implement the pure virtual functions which act on the accessor.
   */
  struct AsyncVariable {
    virtual ~AsyncVariable() = default;

    /** Send the value from the _sendBuffer of the AsyncVariableImpl.
     *  The buffer has to be prepared before calling this function (incl. version number and data validity flag).
     *  The buffer is swapped out to avoid unnecessary copies. If you need a copy, you have to make one
     *  before calling this function.
     */
    virtual void send() = 0;

    /** Send an exception to all subscribers. Must only be called from within deactivateAsyncAndExecute().
     */
    virtual void sendException(std::exception_ptr e) = 0;

    /** Helper functions for the creation of an AsyncNDRegisterAccessor. As the creating code cannot use
     *  the catalogue, each backend has to implement these functions appropriately.
     */
    virtual unsigned int getNumberOfChannels() = 0;
    virtual unsigned int getNumberOfSamples() = 0;
    virtual const std::string& getUnit() = 0;
    virtual const std::string& getDescription() = 0;

    /** Fill the send buffer with data and version number. It is implementation specific where this information is
     * coming from.
     */
    virtual void fillSendBuffer() = 0;
  };

  /** Helper class to have a complete descriton to create an Accessor.
   *  It contains all the information given to DeviceBackend:getNDRegisterAccessor, incl. the offset in the
   *  register which is not known to the catalogue entry and the accessor itself. Just just to keep the number of
   * parameters for createAsyncVariable in check.
   */
  struct AccessorInstanceDescriptor {
    RegisterPath name;
    std::type_index type;
    size_t numberOfWords;
    size_t wordOffsetInRegister;
    AccessModeFlags flags;
  };

  /** The AsyncAccessorManager has three main functionalities:
   *  * It manages the subscription/unsubscription mechanism.
   *  * It serves as a factory for the asynchronous accessors which are during
   *    the subscription to get consistent behaviour.
   *  * The manager provides functions for all asynchronous accessors subscribed
   *    to this manager, like activation or sending exceptions.
   *
   *  This is done in a single class because the container with the fluctuating number of
   *  subscribed variables is not thread safe. This class implements a lock so data
   *  distribution to the variables is safe against concurrent subscriptions/unsubscriptions.
   *
   *  The AsyncAccessorManager has some pure virtual functions. They have implementation
   *  specific functionality which must be provided by a derived version of the AsyncAccessorManager.
   */
  class AsyncAccessorManager : public boost::enable_shared_from_this<AsyncAccessorManager> {
   public:
    explicit AsyncAccessorManager(boost::shared_ptr<DeviceBackend> backend, boost::shared_ptr<Domain> asyncDomain)
    : _backend(std::move(backend)), _asyncDomain(std::move(asyncDomain)) {}
    virtual ~AsyncAccessorManager() = default;

    /** Request a new subscription. This function internally creates the correct asynchronous accessor and a matching
     * AsyncVariable. A weak pointer to the AsyncNDRegisterAccessor is registered in the AsyncVariable, and a shared
     * pointer is returned to the calling code.
     */
    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    /** This function must only be called from the destructor of the AsyncNDRegisterAccessor which is created in the
     * subscribe function!
     */
    void unsubscribe(TransferElementID id);

    /** Send an exception to all accessors. This automatically de-activates them.
     */
    void sendException(const std::exception_ptr& e);

   protected:
    /*** Each implementation must provide a function to create specific AsyncVariables.
     *   When the variable is returned, async accessor is not set yet. This would leave the whole creation
     *   of the async accessor to the backend, would mean a lot of code duplication. It also cannot be
     *   retrieved from the AsyncVariable as this only contains a weak pointer.
     *   If the isActive flag is set, the _sendBuffer must already contain the initial value. The variable itself
     *   is not activated yet as the async accessor is still not set.
     */
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(
        createAsyncVariable, std::unique_ptr<AsyncVariable>(AccessorInstanceDescriptor const&));

    std::map<TransferElementID, std::unique_ptr<AsyncVariable>> _asyncVariables;

    boost::shared_ptr<DeviceBackend> _backend;
    boost::shared_ptr<Domain> _asyncDomain;

    /** This virtual function lets derived classes react on subscribe / unsubscribe. */
    virtual void asyncVariableMapChanged(TransferElementID) {}

    /**
     * We have to remember that we are holding the domain lock before we lock
     * a weak pointer to an AsyncNDRegisterAccessor inside the AsyncVariable to resolve a race conditon:
     * If all other owners go out of scope while we are distributing data or exceptions, releasing the weak pointer
     * will call unsubscribe(). In this case we must not try to get the domain lock again in unsubscribe because we are
     * already holding it in this thread. This implementation was chosen as opposed to using a recursive mutex, because
     * re-acquiring the recursive mutex might cause lock order inversion and possible dead locks if other locks are
     * involved in backends that are not part of the core framework.
     *
     * The variable is holding a pointer to the actual instance of the AsyncAccessorManager while distributing data or
     * exceptions, and nullptr otherwise.
     */
    thread_local static AsyncAccessorManager* _isHoldingDomainLock;
    /**
     * If an unsubscription request is coming in while iterating the _asyncVariables container, we have to remember it
     * and do it afterwards.
     */
    std::list<TransferElementID> _delayedUnsubscriptions;

    /** Internal helper function to avoid code duplication. */
    void unsubscribeImpl(TransferElementID id);
  };

  template<typename SourceType>
  class SourceTypedAsyncAccessorManager : public AsyncAccessorManager {
   public:
    using AsyncAccessorManager::AsyncAccessorManager;

    /**
     * Distribute the given data to the subsribers. Called by the SubDomain in its distribute() and activate()
     *
     * The given VersionNumber shall be attached to the data, except in the case of a TriggeredPollDistributor which
     * might use a DataConsistencyRealm to obtain a different VersionNumber, which it will return in that case. In all
     * other cases, the same VersionNumber as passed will be returned.
     */
    VersionNumber distribute(SourceType, VersionNumber version);

    /**
     * Implement this function in case there is a step between setting the source buffer and filling the user buffers.
     * The TriggeredPollDistributor is using this to poll the data from the device.
     * The return status reports whether this step was successful. If not, the distribution will not happen.
     * In case prepareIntermediateBuffers() is not successful, it must (explicitly or implicitly) call the backend's
     * setException() method.
     */
    virtual bool prepareIntermediateBuffers() { return true; };

   protected:
    SourceType _sourceBuffer;
    VersionNumber _version{nullptr};
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
   * access this weak pointer and do all the locking and nullptr checking.
   */
  template<typename UserType>
  struct AsyncVariableImpl : public AsyncVariable {
    AsyncVariableImpl(size_t nChannels, size_t nElements);

    void send() final;
    void sendException(std::exception_ptr e) final;
    unsigned int getNumberOfChannels() override;
    unsigned int getNumberOfSamples() override;

    typename NDRegisterAccessor<UserType>::Buffer _sendBuffer;

   private:
    // This weak pointer is private so the user cannot bypass correct locking and nullptr-checking.
    boost::weak_ptr<AsyncNDRegisterAccessor<UserType>> _asyncAccessor;
    friend AsyncAccessorManager; // is allowed to set the _asyncAccessor

    VersionNumber _lastSentVersion{nullptr};
  };

  /********************************************************************************************************************/
  // Implementations
  /********************************************************************************************************************/

  /********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::sendException(std::exception_ptr e) {
    auto subscriber = _asyncAccessor.lock();
    if(subscriber.get() != nullptr) { // Solves possible race condition: The subscriber is being destructed.
      subscriber->sendException(e);
    }
  }

  /********************************************************************************************************************/
  template<typename UserType>
  unsigned int AsyncVariableImpl<UserType>::getNumberOfChannels() {
    return _sendBuffer.value.size();
  }

  /********************************************************************************************************************/
  template<typename UserType>
  unsigned int AsyncVariableImpl<UserType>::getNumberOfSamples() {
    if(_sendBuffer.value.size() > 0) {
      return _sendBuffer.value[0].size();
    }
    return 0;
  }

  /********************************************************************************************************************/
  template<typename UserType>
  AsyncVariableImpl<UserType>::AsyncVariableImpl(size_t nChannels, size_t nElements)
  : _sendBuffer(nChannels, nElements) {}

  /********************************************************************************************************************/
  template<typename UserType>
  void AsyncVariableImpl<UserType>::send() {
    auto subscriber = _asyncAccessor.lock();

    if(_lastSentVersion > _sendBuffer.versionNumber) {
      throw ChimeraTK::logic_error(std::format("Trying to send decreased version {} < {} on AsyncVariable {}.",
          _sendBuffer.versionNumber, _lastSentVersion, subscriber->getName()));
    }
    _lastSentVersion = _sendBuffer.versionNumber;

    if(subscriber.get() != nullptr) { // Solves possible race condition: The subscriber is being destructed.
      subscriber->sendDestructively(_sendBuffer);
    }
  }

  /********************************************************************************************************************/
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> AsyncAccessorManager::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    AccessorInstanceDescriptor descriptor{name, typeid(UserType), numberOfWords, wordOffsetInRegister, flags};
    auto untypedAsyncVariable = CALL_VIRTUAL_FUNCTION_TEMPLATE(createAsyncVariable, UserType, descriptor);

    auto asyncVariable = dynamic_cast<AsyncVariableImpl<UserType>*>(untypedAsyncVariable.get());
    // we take all the information we need for the async accessor from AsyncVariable because we cannot use the catalogue
    // here
    auto newSubscriber = boost::make_shared<AsyncNDRegisterAccessor<UserType>>(_backend, shared_from_this(),
        _asyncDomain, name, asyncVariable->getNumberOfChannels(), asyncVariable->getNumberOfSamples(), flags,
        asyncVariable->getUnit(), asyncVariable->getDescription());
    // Set the exception backend here. It might be that the accessor is already activated during subscription, and the
    // backend should be set at that point
    newSubscriber->setExceptionBackend(_backend);

    asyncVariable->_asyncAccessor = newSubscriber;
    // Now that the AsyncVariable is complete we can finally activate it.
    if(_asyncDomain->unsafeGetIsActive()) {
      asyncVariable->fillSendBuffer();
      asyncVariable->send();
    }

    _asyncVariables[newSubscriber->getId()] = std::move(untypedAsyncVariable);
    asyncVariableMapChanged(newSubscriber->getId());

    return newSubscriber;
  }
  /********************************************************************************************************************/
  template<typename SourceType>
  VersionNumber SourceTypedAsyncAccessorManager<SourceType>::distribute(SourceType data, VersionNumber version) {
    if(!_asyncDomain->unsafeGetIsActive()) {
      return version;
    }

    _sourceBuffer = data;
    _version = version;

    if(prepareIntermediateBuffers()) {
      assert(_delayedUnsubscriptions.empty());
      for(auto& var : _asyncVariables) {
        var.second->fillSendBuffer();
        _isHoldingDomainLock = this;
        var.second->send(); // function from  the AsyncVariable base class
        _isHoldingDomainLock = nullptr;
      }
      for(auto id : _delayedUnsubscriptions) {
        unsubscribeImpl(id);
      }
      _delayedUnsubscriptions.clear();
    }

    return _version;
  }

} // namespace ChimeraTK::async
