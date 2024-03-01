// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <mutex>

namespace ChimeraTK {

  class AsyncDomain : public boost::enable_shared_from_this<AsyncDomain> {
   public:
    virtual void sendException(const std::exception_ptr& e) noexcept = 0;
    virtual ~AsyncDomain() = default;

   protected:
    // The mutex has to be recursive because an exception can occur within distribute, which is automatically triggering
    // a sendException. This would deadlock with a regular mutex.
    std::recursive_mutex _mutex;
    // Everything in this class is protected by the mutex to the outside
    // ATTENTION: The friends are not allowed to modify the _isActive flag, only to read it!!
    bool _isActive{false};

    friend class AsyncAccessorManager;
    friend class TriggeredPollDistributor;
    friend class TriggerDistributor;
    friend class InterruptControllerHandler;

    template<typename SourceType>
    friend class VariableDistributor;

    template<typename UserType>
    friend class AsyncNDRegisterAccessor;
  };
} // namespace ChimeraTK
