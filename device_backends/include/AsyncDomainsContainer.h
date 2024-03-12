// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "AsyncDomainsContainerBase.h"
#include "Exception.h"

#include <ChimeraTK/cppext/future_queue.hpp>

#include <iostream>
#include <map>
#include <sstream>
#include <thread>

namespace ChimeraTK {

  /**
   * The AsyncDomainsContainer has a container with AsyncDomains and is performing actions on all of them.
   *
   * At the moment, the only common action is sending exceptions. This is implemented via a thread.
   * sendExceptions() is pushing the exception message into a queue and returns immediately.
   * A distributor thread is waiting for data in the queue and is sending the exceptions in all AsyncDomains.
   *
   * The reason for having a thread is is possible lock order inversion. In the distribution tree, accessor creation
   * must be mutually exclusibe with data distribution, hence locks cannot be avoided. As excaption can occur during
   * data distribution. and backend::setException() is called, this would lead to recursive calls to the distribution
   * tree, which might result in lock order inversions and deadlocks. To avoid this, setException will only put the
   * exception message into the queue and return, allowing the failing distribution call to complete. The exception
   * distribution will then be done by the thread in the AsyncDomainsContainer, after the mutex in the AsyncDomain is
   * free again.
   */
  template<typename KeyType>
  class AsyncDomainsContainer : public AsyncDomainsContainerBase {
   public:
    explicit AsyncDomainsContainer();
    ~AsyncDomainsContainer() override;

    void sendExceptions(const std::string& exceptionMessage) override;

    /**
     * Adds an AyncDomain to the container. If there is already an entry for that key, and that weak pointer can be
     * locked, it throws a ChimeraTK::logic_error. This function must only be called if the either an entry for that key
     * does not exist, or the according share pointer is gone.
     */
    void addAsyncDomain(KeyType key, const boost::shared_ptr<AsyncDomain>& asyncDomain);

   protected:
    void distributeExceptions();
    cppext::future_queue<std::string> _startExceptionDistribution{2};
    std::thread _distributorThread;
    class StopThread : public std::exception {};

    std::mutex _domainsMutex;
    std::map<KeyType, boost::weak_ptr<AsyncDomain>> _asyncDomains;
  };

  /********************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::addAsyncDomain(KeyType key, const boost::shared_ptr<AsyncDomain>& asyncDomain) {
    std::lock_guard<std::mutex> domainsLock(_domainsMutex);

    auto domainIter = _asyncDomains.find(key);
    if((domainIter != _asyncDomains.end()) && (domainIter->second.lock())) {
      std::stringstream errorMessage;
      errorMessage << "AsyncDomainsContainer: AsyncDomain with key '" << key << "' already exists.";
      throw ChimeraTK::runtime_error(errorMessage.str());
    }
    _asyncDomains[key] = asyncDomain;
  }

  /********************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::distributeExceptions() {
    std::string exceptionMessage;
    while(true) {
      // block to wait until setException has been called
      try {
        _startExceptionDistribution.pop_wait(exceptionMessage);
      }
      catch(StopThread&) {
        return;
      }

      auto ex = std::make_exception_ptr(ChimeraTK::runtime_error(exceptionMessage));

      {
        std::lock_guard<std::mutex> containerLock(_domainsMutex);
        for(auto& keyAndDomain : _asyncDomains) {
          auto domain = keyAndDomain.second.lock();
          if(domain) {
            domain->sendException(ex);
          }
        }
      } // lock scope

      _isSendingExceptions = false;
    }
  }

  /********************************************************************************************************************/

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::AsyncDomainsContainer() {
    _distributorThread = std::thread([&] { distributeExceptions(); });
  }

  /********************************************************************************************************************/

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::~AsyncDomainsContainer() {
    try {
      _startExceptionDistribution.push_overwrite_exception(std::make_exception_ptr(StopThread{}));
      // Now we can join the thread.
      _distributorThread.join();
    }
    catch(std::system_error& e) {
      // Destructors must not throw. All exceptions that can occur here are system errors, which only show up if there
      // is no hope anyway. All we can do is terminate.
      std::cerr << "Unrecoverable system error in ~AsyncDomainsContainer(): " << e.what() << " !!! TERMINATING !!!"
                << std::endl;
      std::terminate();
    }

    // Unblock a potentially waiting open call
    _isSendingExceptions = false;
  }

  /********************************************************************************************************************/

  template<typename KeyType>
  void AsyncDomainsContainer<KeyType>::sendExceptions(const std::string& exceptionMessage) {
    if(_isSendingExceptions) {
      throw ChimeraTK::logic_error(
          "AsyncDomainsContainer::sendExceptions() called before previous distribution was ready.");
    }
    _isSendingExceptions = true;

    _startExceptionDistribution.push(exceptionMessage);
  }
} // namespace ChimeraTK
