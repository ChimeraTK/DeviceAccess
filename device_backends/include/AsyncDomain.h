// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <mutex>

namespace ChimeraTK {

  /**
   * The AsyncDomain is the thread-safe entry point for each distribution tree.
   * Distributing data to accessors, sending exceptions and subscription of new accessors will all happen from
   * different threads. This class implements a central mutex such that only one operation on the distribution three is
   * executed at the same time.
   *
   * This base class is providing the mutex and the _isActive flag, which is needed throughout the distribution tree.
   * It also has a virtual setException() function to a allow sending exception from code that does not know
   * about the distributor type.
   *
   * All other functions depend on a backend-specific data type and the according distributor. Hence the
   * AsyncDomainImpl class is templated on both.
   */
  class AsyncDomain : public boost::enable_shared_from_this<AsyncDomain> {
   public:
    virtual void sendException(const std::exception_ptr& e) noexcept = 0;
    virtual void deactivate() = 0;
    virtual ~AsyncDomain() = default;

    std::lock_guard<std::mutex> getDomainLock() { return std::lock_guard<std::mutex>{_mutex}; }

   protected:
    // This mutex is protecting all members and all functions in AsyncDomain and AsyncDomainImpl
    std::mutex _mutex;
    bool _isActive{false};

    /**
     * Friend classes are allowed to read the _isActiveFlag without acquiring the mutex.
     * The friend's functions are only called from the AsyncDomain functions after already locking the mutex.
     */
    bool unsafeGetIsActive() const { return _isActive; }

    // The friend functions are only allowed to call unsafeGetIsActive(). They must not touch any of the
    // internal variables directly.
    friend class AsyncAccessorManager;
    friend class TriggeredPollDistributor;
    template<typename BackendSpecificDataType>
    friend class TriggerDistributor;
    friend class InterruptControllerHandler;

    template<typename SourceType>
    friend class SourceTypedAsyncAccessorManager;

    template<typename UserType>
    friend class AsyncNDRegisterAccessor;
  };
} // namespace ChimeraTK
