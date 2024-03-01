// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TriggerDistributor.h"

#include "InterruptControllerHandler.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "TriggeredPollDistributor.h"
#include "VariableDistributor.h"

namespace ChimeraTK {

  TriggerDistributor::TriggerDistributor(boost::shared_ptr<DeviceBackend> backend,
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> interruptID,
      boost::shared_ptr<InterruptControllerHandler> parent, boost::shared_ptr<AsyncDomain> asyncDomain)
  : _id(std::move(interruptID)), _backend(backend), _interruptControllerHandlerFactory(controllerHandlerFactory),
    _parent(std::move(parent)), _asyncDomain(std::move(asyncDomain)) {}

  //*********************************************************************************************************************/

  template<typename DistributorType>
  boost::shared_ptr<DistributorType> TriggerDistributor::getDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    if(interruptID.size() == 1) {
      // return the distributor from this instance, not a nested one

      boost::weak_ptr<DistributorType>* weakDistributor{
          nullptr}; // Cannot create a reference here, but references in the "if constexpr" scope are not seen outside

      // return the distributor from this instance, not a nested one
      if constexpr(std::is_same<DistributorType, TriggeredPollDistributor>::value) {
        weakDistributor = &_pollDistributor;
      }
      else if constexpr(std::is_same<DistributorType, VariableDistributor<std::nullptr_t>>::value) {
        weakDistributor = &_variableDistributor;
      }
      else {
        throw ChimeraTK::logic_error("TriggerDistributor::getDistributorRecursive(): Wrong template parameter.");
      }

      auto distributor = weakDistributor->lock();
      if(!distributor) {
        distributor = boost::make_shared<DistributorType>(_backend, _id, shared_from_this(), _asyncDomain);
        *weakDistributor = distributor;
        if(_asyncDomain->_isActive) {
          if constexpr(std::is_same<DistributorType, TriggeredPollDistributor>::value) {
            distributor->activate({});
          }
          else if constexpr(std::is_same<DistributorType, VariableDistributor<std::nullptr_t>>::value) {
            distributor->activate(nullptr, {});
          }
        }
      }
      return distributor;
    }
    // get a nested poll distributor
    auto controllerHandler = _interruptControllerHandler.lock();
    if(!controllerHandler) {
      controllerHandler = _interruptControllerHandlerFactory->createInterruptControllerHandler(_id, shared_from_this());
      _interruptControllerHandler = controllerHandler;
    }

    return controllerHandler->getDistributorRecursive<DistributorType>({++interruptID.begin(), interruptID.end()});
  }

  //*********************************************************************************************************************/

  boost::shared_ptr<TriggeredPollDistributor> TriggerDistributor::getPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<TriggeredPollDistributor>(interruptID);
  }

  //*********************************************************************************************************************/

  boost::shared_ptr<VariableDistributor<std::nullptr_t>> TriggerDistributor::getVariableDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<VariableDistributor<std::nullptr_t>>(interruptID);
  }

  //*********************************************************************************************************************/

  void TriggerDistributor::distribute([[maybe_unused]] std::nullptr_t, VersionNumber version) {
    if(!_asyncDomain->_isActive) {
      return;
    }
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->trigger(version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->handle(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->distribute(nullptr, version);
    }
  }

  //*********************************************************************************************************************/

  void TriggerDistributor::activate([[maybe_unused]] std::nullptr_t, VersionNumber version) {
    auto pollDistributor = _pollDistributor.lock();
    if(pollDistributor) {
      pollDistributor->activate(version);
    }
    auto controllerHandler = _interruptControllerHandler.lock();
    if(controllerHandler) {
      controllerHandler->activate(version);
    }
    auto variableDistributor = _variableDistributor.lock();
    if(variableDistributor) {
      variableDistributor->activate(nullptr, version);
    }
  }

  //*********************************************************************************************************************/

  void TriggerDistributor::sendException(const std::exception_ptr& e) {
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

  //*********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<AsyncNDRegisterAccessor<UserType>> TriggerDistributor::SubscriptionImplementor<
      UserType>::subscribeTo(TriggerDistributor& triggerDistributor, RegisterPath name, size_t numberOfWords,
      size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto catalogue = triggerDistributor._backend->getRegisterCatalogue(); // need to store the clone you get
    const auto& backendCatalogue = catalogue.getImpl();
    // This code only works for backends which use the NumericAddressedRegisterCatalogue because we need the
    // interrupt description which is specific for those backends and not in the general catalogue.
    // If the cast fails, it will throw an exception.
    auto removeMe = backendCatalogue.getRegister(name);
    const auto& numericCatalogue = dynamic_cast<const NumericAddressedRegisterCatalogue&>(backendCatalogue);
    auto registerInfo = numericCatalogue.getBackendRegister(name);

    // Find the right place in the distribution tree to subscribe
    boost::shared_ptr<AsyncAccessorManager> distributor;
    if(registerInfo.getDataDescriptor().fundamentalType() == DataDescriptor::FundamentalType::nodata) {
      distributor = triggerDistributor.getVariableDistributorRecursive(registerInfo.interruptId);
    }
    else {
      distributor = triggerDistributor.getPollDistributorRecursive(registerInfo.interruptId);
    }

    return distributor->template subscribe<UserType>(name, numberOfWords, wordOffsetInRegister, flags);
  };

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(TriggerDistributor::SubscriptionImplementor);

} // namespace ChimeraTK
