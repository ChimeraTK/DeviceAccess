// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomainImpl.h"

#include <ChimeraTK/cppext/future_queue.hpp>

#include <iostream>
#include <map>
#include <sstream>
#include <thread>

namespace ChimeraTK {

  /**
   * The AsyncDomainsContainer has a container with AsyncDomains and is performing actions on all of them.
   *
   * The key type to identify the domain is of type size_t, which is the return type of std::hash, so you can easily use
   * the hashes in case the backend uses a different key type.
   *
   * Sending exceptions is implemented via a thread.
   * sendExceptions() is pushing the exception message into a queue and returns immediately.
   * A distributor thread is waiting for data in the queue and is sending the exceptions in all AsyncDomains.
   *
   * The reason for having a thread is a possible lock order inversion. In the distribution tree, accessor creation
   * must be mutually exclusibe with data distribution, hence locks cannot be avoided. As excaption can occur during
   * data distribution. and backend::setException() is called, this would lead to recursive calls to the distribution
   * tree, which might result in lock order inversions and deadlocks. To avoid this, setException will only put the
   * exception message into the queue and return, allowing the failing distribution call to complete. The exception
   * distribution will then be done by the thread in the AsyncDomainsContainer, after the mutex in the AsyncDomain is
   * free again.
   */
  class AsyncDomainsContainer {
   public:
    ~AsyncDomainsContainer();

    /** Request the sending of exceptions. This function stores the request and returns immediately.
     *  The actual exception distribution is done asynchronously and has not necessarily finished when the function call
     * returns. Use isSendingExceptions() to check whether the exception distribution has finished.
     */
    void sendExceptions(const std::string& exceptionMessage);

    /** Check whether an exception distribution is started and not completed yet. */
    bool isSendingExceptions() { return _isSendingExceptions; }

    /**
     *  Get an existing AsyncDomain for the specific key, or create a new one while holding the container lock.
     *  The function is templated to the backend type and a BackendSpecificDataType. The BackendSpecificDataType might
     *  vary for different AsyncDomains in the same backend.
     *
     *  The backend has to provide a function to get the initial value for that AsyncDomain.
     *  \code{.cpp}
     *  template<typename BackendSpecificDataType>
     *  std::pair<BackendSpecificUserType, VersionNumber> getAsyncDomainInitialValue(size_t asyncDomainId);
     *  \endcode
     *  This function is returning the initial value for the domain with the according asyncDomainID. If the version
     * number can be determined from the initial value data, the version number is set here. Otherwise the version
     * number must be VersionNumber{nullptr}. If the version number cannot be determined from the initial value **do
     * not create a new version number here!**. This must happen inside the AsyncDomain under the AsyncDomain lock to
     * avoid race conditions.
     *
     *  @param backend Shared pointer to the exact backend type. As template functions cannot be virtual, we need a
     *  pointer of the type that implements getAsyncDomainInitialValue().
     *  @param key The AsyncDomainId is the key for the container (map).
     *  @param activate Flag whether to activate the async domain if it is created.
     *
     */
    template<typename BackendType, typename BackendSpecificDataType>
    boost::shared_ptr<AsyncDomainImpl<BackendSpecificDataType>> getOrCreateAsyncDomain(
        boost::shared_ptr<BackendType> backend, size_t key, bool activate);

    /**
     * Return the shared pointer to the AsyncDomain for a key. The shared pointer might be nullptr if locking of the
     * weak pointer failed.
     */
    boost::shared_ptr<AsyncDomain> getAsyncDomain(size_t key);

    /**
     * Iterate all AsyncDomains under the container lock.
     * Each weak pointer is locked and the argument function is executed if the share_ptr is not nullptr.
     *
     * The first parameter of the callback function is the domain key, the second a shared pointer to the domain itself.
     */
    void forEach(const std::function<void(size_t, boost::shared_ptr<AsyncDomain>&)>& executeMe);

   protected:
    std::atomic_bool _isSendingExceptions{false};

    /** Endless loop executed in the thread. */
    void distributeExceptions();

    cppext::future_queue<std::string> _startExceptionDistribution{2};
    std::thread _distributorThread;
    std::mutex _threadCreationMutex;
    std::atomic_bool _threadIsRunning{false}; // Cache whether the thread is running so we don't have to lock the mutex
                                              // each time to be able to ask the thread itself.
    class StopThread : public std::exception {};

    std::mutex _domainsMutex;
    std::map<size_t, boost::weak_ptr<AsyncDomain>> _asyncDomains;
  };

  /********************************************************************************************************************/

  template<typename BackendType, typename BackendSpecificDataType>
  boost::shared_ptr<AsyncDomainImpl<BackendSpecificDataType>> AsyncDomainsContainer::getOrCreateAsyncDomain(
      boost::shared_ptr<BackendType> backend, size_t key, bool activate) {
    std::lock_guard<std::mutex> domainsLock(_domainsMutex);

    auto domain = _asyncDomains[key].lock();

    if(domain) {
      auto domainImpl = boost::dynamic_pointer_cast<AsyncDomainImpl<BackendSpecificDataType>>(domain);
      assert(domainImpl);
      return domainImpl;
    }

    // The domain does not exist, create it
    auto domainImpl = boost::make_shared<AsyncDomainImpl<BackendSpecificDataType>>(backend, key);
    // start the thread if not already running
    { // thread lock scope
      auto threadCreationLock = std::lock_guard(_threadCreationMutex);

      if(!_distributorThread.joinable()) {
        _distributorThread = std::thread([&] { distributeExceptions(); });
        _threadIsRunning = true;
      }
    } // end of thread lock scope

    if(activate) {
      auto subscriptionDone = backend->activateSubscription(key, domainImpl);
      // wait until the backend reports that the subscription is ready before proceeding with the creation of the
      // accessor, which will poll the initial value.
      subscriptionDone.wait();
      auto [value, version] = backend->template getAsyncDomainInitialValue<BackendSpecificDataType>(key);
      domainImpl->activate(value, version);
    }

    _asyncDomains[key] = domainImpl;
    return domainImpl;
  }

} // namespace ChimeraTK
