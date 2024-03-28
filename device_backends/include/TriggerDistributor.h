// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"
#include "VersionNumber.h"

#include <boost/make_shared.hpp>

namespace ChimeraTK {
  class InterruptControllerHandlerFactory;
  class InterruptControllerHandler;
  class AsyncDomain;

  template<typename UserType>
  class AsyncNDRegisterAccessor;

  namespace detail {
    template<typename UserType, typename BackendSpecificDataType>
    class AsyncDataAdapterSubscriptionImplementor;
  } // namespace detail

  /********************************************************************************************************************/

  /** Distribute a typed interrupt signal (trigger) to three possible consumers:
   *  \li InterruptControllerHandler
   *  \li TriggeredPollDistributor
   *  \li VariableDistributor<BackendSpecificDataType>
   */
  template<typename BackendSpecificDataType>
  class TriggerDistributor : public boost::enable_shared_from_this<TriggerDistributor<BackendSpecificDataType>> {
   public:
    TriggerDistributor(boost::shared_ptr<DeviceBackend> backend, std::vector<uint32_t> interruptID,
        boost::shared_ptr<InterruptControllerHandler> parent, boost::shared_ptr<AsyncDomain> asyncDomain);

    void activate(BackendSpecificDataType, VersionNumber v);
    void distribute(BackendSpecificDataType, VersionNumber v);
    void sendException(const std::exception_ptr& e);

    /**
     * Common implementation for getting a TriggeredPollDistributor or a VariableDistributor<nullptr_t> to avoid code
     * duplication. It only works for those two template types. The implementation and the code instantiations are in
     * the .cc file to  avoid circular header inclusion.
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
    boost::weak_ptr<InterruptControllerHandler> _interruptControllerHandler;
    boost::weak_ptr<TriggeredPollDistributor> _pollDistributor;
    boost::weak_ptr<VariableDistributor<std::nullptr_t>> _variableDistributor;
    boost::shared_ptr<InterruptControllerHandler> _parent;
    boost::shared_ptr<AsyncDomain> _asyncDomain;

    template<typename UserType, typename BackendDataType>
    friend class detail::AsyncDataAdapterSubscriptionImplementor;
  };

  /********************************************************************************************************************/

  namespace detail {
    // Helper class to get instances for all user types. We cannot put the implementation into the header because of
    // circular header inclusion, and we cannot write a "for all user types" macro for functions because of the return
    // value and the function signature.
    template<typename UserType, typename BackendSpecificDataType>
    class AsyncDataAdapterSubscriptionImplementor {
     public:
      static boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> subscribeTo(
          TriggerDistributor<BackendSpecificDataType>& triggerDistributor, RegisterPath name, size_t numberOfWords,
          size_t wordOffsetInRegister, AccessModeFlags flags);
    };

  } // namespace detail

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  TriggerDistributor<BackendSpecificDataType>::TriggerDistributor(boost::shared_ptr<DeviceBackend> backend,
      std::vector<uint32_t> interruptID, boost::shared_ptr<InterruptControllerHandler> parent,
      boost::shared_ptr<AsyncDomain> asyncDomain)
  : _id(std::move(interruptID)), _backend(backend), _parent(std::move(parent)), _asyncDomain(std::move(asyncDomain)) {}

  /********************************************************************************************************************/
  template<typename BackendSpecificDataType>
  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> TriggerDistributor<BackendSpecificDataType>::subscribe(
      RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    return detail::AsyncDataAdapterSubscriptionImplementor<UserType, BackendSpecificDataType>::subscribeTo(
        *this, name, numberOfWords, wordOffsetInRegister, flags);
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  template<typename DistributorType>
  boost::shared_ptr<DistributorType> TriggerDistributor<BackendSpecificDataType>::getDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    if(interruptID.size() == 1) {
      // return the distributor from this instance, not a from further down the tree

      boost::weak_ptr<DistributorType>* weakDistributor{
          nullptr}; // Cannot create a reference here, but references in the "if constexpr" scope are not seen outside

      if constexpr(std::is_same<DistributorType, TriggeredPollDistributor>::value) {
        weakDistributor = &_pollDistributor;
      }
      else if constexpr(std::is_same<DistributorType, VariableDistributor<BackendSpecificDataType>>::value) {
        weakDistributor = &_variableDistributor;
      }
      else {
        throw ChimeraTK::logic_error("TriggerDistributor::getDistributorRecursive(): Wrong template parameter.");
      }

      auto distributor = weakDistributor->lock();
      if(!distributor) {
        distributor = boost::make_shared<DistributorType>(_backend, this->shared_from_this(), _asyncDomain);
        *weakDistributor = distributor;
        if(_asyncDomain->unsafeGetIsActive()) {
          distributor->distribute(nullptr, {});
        }
      }
      return distributor;
    }
    // get a distributor from further down the tree, behind one or more InterruptControllerHandlers
    auto controllerHandler = _interruptControllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _backend->createInterruptControllerHandler(_id, this->shared_from_this());
      _interruptControllerHandler = controllerHandler;
    }

    return controllerHandler->getDistributorRecursive<DistributorType>({++interruptID.begin(), interruptID.end()});
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void TriggerDistributor<BackendSpecificDataType>::distribute(BackendSpecificDataType data, VersionNumber version) {
    if(!_asyncDomain->unsafeGetIsActive()) {
      return;
    }
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->distribute(nullptr, version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->handle(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(data, version);
    }
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void TriggerDistributor<BackendSpecificDataType>::activate(BackendSpecificDataType data, VersionNumber version) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->distribute(nullptr, version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->activate(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(data, version);
    }
  }

  /********************************************************************************************************************/

  template<typename BackendSpecificDataType>
  void TriggerDistributor<BackendSpecificDataType>::sendException(const std::exception_ptr& e) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->sendException(e);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->sendException(e);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->sendException(e);
    }
  }

  /********************************************************************************************************************/

  namespace detail {

    template<typename UserType, typename BackendSpecificDataType>
    boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> AsyncDataAdapterSubscriptionImplementor<UserType,
        BackendSpecificDataType>::subscribeTo(TriggerDistributor<BackendSpecificDataType>& triggerDistributor,
        RegisterPath name, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      auto catalogue = triggerDistributor._backend->getRegisterCatalogue(); // need to store the clone you get
      const auto& backendCatalogue = catalogue.getImpl();
      // This code only works for backends which use the NumericAddressedRegisterCatalogue because we need the
      // interrupt description which is specific for those backends and not in the general catalogue.
      // If the cast fails, it will throw an exception.
      const auto& numericCatalogue = dynamic_cast<const NumericAddressedRegisterCatalogue&>(backendCatalogue);
      auto registerInfo = numericCatalogue.getBackendRegister(name);

      // Find the right place in the distribution tree to subscribe
      boost::shared_ptr<AsyncAccessorManager> distributor;
      if constexpr(std::is_same<BackendSpecificDataType, std::nullptr_t>::value) {
        // Special implementation for data type nullptr_t: Use a poll distributor if the data is not
        // FundamentalType::nodata itself
        if(registerInfo.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::nodata) {
          distributor = triggerDistributor.template getDistributorRecursive<VariableDistributor<std::nullptr_t>>(
              registerInfo.interruptId);
        }
        else {
          distributor =
              triggerDistributor.template getDistributorRecursive<TriggeredPollDistributor>(registerInfo.interruptId);
        }
      }
      else {
        // For all other BackendSpecificDataType use the according VariableDistributor.
        // This scheme might need some improvement later.
        triggerDistributor.template getDistributorRecursive<VariableDistributor<BackendSpecificDataType>>(
            registerInfo.interruptId);
      }

      return distributor->template subscribe<UserType>(name, numberOfWords, wordOffsetInRegister, flags);
    }

    INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_USER_TYPES(AsyncDataAdapterSubscriptionImplementor, std::nullptr_t);
  } // namespace detail

  /********************************************************************************************************************/

} // namespace ChimeraTK
