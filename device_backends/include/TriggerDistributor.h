// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "VersionNumber.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK {
  class InterruptControllerHandlerFactory;
  class InterruptControllerHandler;
  class TriggeredPollDistributor;
  class AsyncDomain;

  template<typename UserType>
  class VariableDistributor;

  template<typename UserType>
  class AsyncNDRegisterAccessor;

  /** Distribute a void type interrupt signal (trigger) to three possible consumers:
   *  \li InterruptControllerHandler
   *  \li TriggeredPollDistributor
   *  \li VariableDistributor<nullptr_t>
   */
  class TriggerDistributor : public boost::enable_shared_from_this<TriggerDistributor> {
   public:
    TriggerDistributor(boost::shared_ptr<DeviceBackend> backend,
        InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> interruptID,
        boost::shared_ptr<InterruptControllerHandler> parent, boost::shared_ptr<AsyncDomain> asyncDomain);

    void activate(std::nullptr_t, VersionNumber v);
    void distribute(std::nullptr_t, VersionNumber v);
    void sendException(const std::exception_ptr& e);

    boost::shared_ptr<TriggeredPollDistributor> getPollDistributorRecursive(std::vector<uint32_t> const& interruptID);
    boost::shared_ptr<VariableDistributor<std::nullptr_t>> getVariableDistributorRecursive(
        std::vector<uint32_t> const& interruptID);

    /**
     * Common implementation for getPollDistributorRecursive and getVariableDistributorRecursive to avoid code duplication.
     */
    /* Don't remove the non-templated functions when refactoring. They are needed to instantiate the template code. We
     * cannot put the template code into the header because it needs full class descriptions, which would lead to
     * circular header inclusion.
     */
    template<typename DistributorType>
    boost::shared_ptr<DistributorType> getDistributorRecursive(std::vector<uint32_t> const& interruptID);

    boost::shared_ptr<AsyncDomain> getAsyncDomain() { return _asyncDomain; }

    template<typename UserType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribe(
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

   protected:
    std::vector<uint32_t> _id;

    boost::shared_ptr<DeviceBackend> _backend;
    InterruptControllerHandlerFactory* _interruptControllerHandlerFactory;
    boost::weak_ptr<InterruptControllerHandler> _interruptControllerHandler;
    boost::weak_ptr<TriggeredPollDistributor> _pollDistributor;
    boost::weak_ptr<VariableDistributor<std::nullptr_t>> _variableDistributor;
    boost::shared_ptr<InterruptControllerHandler> _parent;
    boost::shared_ptr<AsyncDomain> _asyncDomain;

    // Helper class to get instances for all user types. We cannot put the implementation into the header because of
    // circular header inclusion, and we cannot write a "for all user types" macro for functions because of the return
    // value and the function signature.
    template<typename UserType>
    class SubscriptionImplementor {
     public:
      static boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribeTo(TriggerDistributor& triggerDistributor,
          RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
    };

    template<typename UserType>
    friend class SubscriptionImplementor;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TriggerDistributor::SubscriptionImplementor);

  //*****************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> TriggerDistributor::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    return SubscriptionImplementor<UserType>::subscribeTo(*this, name, numberOfWords, wordOffsetInRegister, flags);
  }

} // namespace ChimeraTK
