// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AsyncDomainsContainer.h"

#include "Exception.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  void AsyncDomainsContainer::distributeExceptions() {
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

  AsyncDomainsContainer::~AsyncDomainsContainer() {
    try {
      auto threadCreationLock = std::lock_guard(_threadCreationMutex);

      if(_distributorThread.joinable()) {
        _startExceptionDistribution.push_overwrite_exception(std::make_exception_ptr(StopThread{}));
        // Now we can join the thread.
        _distributorThread.join();
      }
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

  void AsyncDomainsContainer::sendExceptions(const std::string& exceptionMessage) {
    if(_isSendingExceptions) {
      throw ChimeraTK::logic_error(
          "AsyncDomainsContainer::sendExceptions() called before previous distribution was ready.");
    }

    if(!_threadIsRunning) {
      // No subscribers yet. Don't write anything into the queue.
      return;
    }

    _isSendingExceptions = true;

    _startExceptionDistribution.push(exceptionMessage);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<AsyncDomain> AsyncDomainsContainer::getAsyncDomain(size_t key) {
    std::lock_guard<std::mutex> domainsLock(_domainsMutex);
    // Creates the entry if it is not there. The returned shared pointer is empty in this case.
    // (It might also be empty if the entry exists, but the async domain was deleted.)
    return _asyncDomains[key].lock();
  }

  /********************************************************************************************************************/

  void AsyncDomainsContainer::forEach(const std::function<void(size_t, boost::shared_ptr<AsyncDomain>&)>& executeMe) {
    std::lock_guard<std::mutex> domainsLock(_domainsMutex);
    for(auto& keyAndDomain : _asyncDomains) {
      auto domain = keyAndDomain.second.lock();
      if(domain) {
        executeMe(keyAndDomain.first, domain);
      }
    }
  }

} // namespace ChimeraTK
