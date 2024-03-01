// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "InterruptControllerHandler.h"

#include "Axi4_Intc.h"
#include "DummyIntc.h"
#include "TriggerDistributor.h"
#include "TriggeredPollDistributor.h"

#include <tuple>

namespace ChimeraTK {
  //*********************************************************************************************************************/
  InterruptControllerHandlerFactory::InterruptControllerHandlerFactory(DeviceBackend* backend) : _backend(backend) {
    // we already know about the build-in handlers
    _creatorFunctions["AXI4_INTC"] = Axi4_Intc::create;
    _creatorFunctions["dummy"] = DummyIntc::create;
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandlerFactory::addControllerDescription(
      std::vector<uint32_t> const& controllerID, std::string const& name, std::string const& description) {
    _controllerDescriptions[controllerID] = {name, description};
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<InterruptControllerHandler> InterruptControllerHandlerFactory::createInterruptControllerHandler(
      std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent) {
    assert(!controllerID.empty());
    std::string name, description;
    try {
      std::tie(name, description) = _controllerDescriptions.at(controllerID);
    }
    catch(std::out_of_range&) {
      std::string idAsString;
      for(auto i : controllerID) {
        idAsString += std::to_string(i) + ":";
      }
      idAsString.pop_back(); // remove last ":"
      throw ChimeraTK::logic_error("Unknown interrupt controller ID " + idAsString);
    }
    auto creatorFunctionIter = _creatorFunctions.find(name);
    if(creatorFunctionIter == _creatorFunctions.end()) {
      throw ChimeraTK::logic_error("Unknown interrupt controller type \"" + name + "\"");
    }
    return creatorFunctionIter->second(this, controllerID, description, std::move(parent));
  }

  //*********************************************************************************************************************/
  boost::shared_ptr<DeviceBackend> InterruptControllerHandlerFactory::getBackend() {
    return boost::dynamic_pointer_cast<DeviceBackend>(_backend->shared_from_this());
  }

  //*********************************************************************************************************************/
  InterruptControllerHandler::InterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
      std::vector<uint32_t> controllerID, boost::shared_ptr<TriggerDistributor> parent)
  : _backend(controllerHandlerFactory->getBackend()), _controllerHandlerFactory(controllerHandlerFactory),
    _id(std::move(controllerID)), _parent(std::move(parent)), _asyncDomain(_parent->getAsyncDomain()) {}

  //*********************************************************************************************************************/
  template<typename DistributorType>
  boost::shared_ptr<DistributorType> InterruptControllerHandler::getDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    // assert(false); // FIXME: needs container lock!
    assert(!interruptID.empty());
    auto qualifiedInterruptId = _id;
    qualifiedInterruptId.push_back(interruptID.front());

    // we can't use try_emplace because the map contains weak pointers
    boost::shared_ptr<TriggerDistributor> distributor;
    auto distributorIter = _distributors.find(interruptID.front());
    if(distributorIter == _distributors.end()) {
      distributor = boost::make_shared<TriggerDistributor>(
          _backend, _controllerHandlerFactory, qualifiedInterruptId, shared_from_this(), _asyncDomain);
      _distributors[interruptID.front()] = distributor;
      if(_asyncDomain->_isActive) {
        // Creating a new version here is correct. Nothing has been distributed to any accessor connected to this
        // sub-interrupt yet because we just created this distributor.
        distributor->activate(nullptr, {});
      }
    }
    else {
      distributor = distributorIter->second.lock();
      if(!distributor) {
        distributor = boost::make_shared<TriggerDistributor>(
            _backend, _controllerHandlerFactory, qualifiedInterruptId, shared_from_this(), _asyncDomain);
        distributorIter->second = distributor;
        if(_asyncDomain->_isActive) {
          distributor->activate(nullptr, {});
        }
      }
    }
    return distributor->getDistributorRecursive<DistributorType>(interruptID);
  }

  //*********************************************************************************************************************/

  boost::shared_ptr<TriggeredPollDistributor> InterruptControllerHandler::getPollDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<TriggeredPollDistributor>(interruptID);
  }

  //*********************************************************************************************************************/

  boost::shared_ptr<VariableDistributor<std::nullptr_t>> InterruptControllerHandler::getVariableDistributorRecursive(
      std::vector<uint32_t> const& interruptID) {
    return getDistributorRecursive<VariableDistributor<std::nullptr_t>>(interruptID);
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::activate(VersionNumber version) {
    for(auto& distributorIter : _distributors) {
      auto distributor = distributorIter.second.lock();
      if(distributor) {
        distributor->activate(nullptr, version);
      }
    }
  }

  //*********************************************************************************************************************/
  void InterruptControllerHandler::sendException(const std::exception_ptr& e) {
    for(auto& distributorIter : _distributors) {
      auto distributor = distributorIter.second.lock();
      if(distributor) {
        distributor->sendException(e);
      }
    }
  }

} // namespace ChimeraTK
