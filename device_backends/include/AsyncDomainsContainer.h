// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AsyncDomain.h"
#include "AsyncDomainsContainerBase.h"
#include "Exception.h"

#include <ChimeraTK/cppext/future_queue.hpp>

#include <map>
#include <thread>

namespace ChimeraTK {

  template<typename KeyType>
  class AsyncDomainsContainer : public AsyncDomainsContainerBase {
   public:
    explicit AsyncDomainsContainer();
    ~AsyncDomainsContainer() override;

    std::map<KeyType, boost::weak_ptr<AsyncDomain>> asyncDomains;
    void sendExceptions(const std::string& exceptionMessage) override;

   protected:
    void distributeExceptions();
    cppext::future_queue<std::string> _startExceptionDistribution{2};
    std::thread _distributorThread;
    class StopThread {};
  };

  /*******************************************************************************************************************/

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

      try {
        throw ChimeraTK::runtime_error(exceptionMessage);
      }
      catch(...) {
        for(auto& keyAndDomain : asyncDomains) {
          auto domain = keyAndDomain.second.lock();
          if(domain) {
            domain->sendException(std::current_exception());
          }
        }
      }

      _isSendingExceptions = false;
    }
  }

  /*******************************************************************************************************************/

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::AsyncDomainsContainer() {
    _distributorThread = std::thread([&] { distributeExceptions(); });
  }

  /*******************************************************************************************************************/

  template<typename KeyType>
  AsyncDomainsContainer<KeyType>::~AsyncDomainsContainer() {
    try {
      try {
        throw StopThread();
      }
      catch(...) {
        _startExceptionDistribution.push_overwrite_exception(std::current_exception());
      }
      // Now we can join the thread.
      _distributorThread.join();
    }
    catch(std::system_error& e) {
      // Destructors must not throw. All exceptions that can occur here are system errors, which only show up if there
      // is no hope anyway. All we can do it terminate.
      std::cerr << "Unrecoverable system error in ~AsyncDomainsContainer(): " << e.what() << " !!! TERMINATING !!!"
                << std::endl;
      std::terminate();
    }

    // Unblock a potentially waiting open call
    _isSendingExceptions = false;
  }

  /*******************************************************************************************************************/

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
