// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/MuxedInterruptDistributor.h"

#include "async/DummyMuxedInterruptDistributor.h"
#include "async/GenericMuxedInterruptDistributor.h"
#include "async/SubDomain.h"
#include "async/TriggeredPollDistributor.h"
#include <nlohmann/json.hpp>

#include <tuple>

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  MuxedInterruptDistributorFactory::MuxedInterruptDistributorFactory() {
    // we already know about the build-in handlers
    _creatorFunctions["INTC"] = GenericMuxedInterruptDistributor::create;
    _creatorFunctions["dummy"] = DummyMuxedInterruptDistributor::create;
  }

  /********************************************************************************************************************/

  MuxedInterruptDistributorFactory& MuxedInterruptDistributorFactory::getInstance() {
    static MuxedInterruptDistributorFactory instance;
    return instance;
  }

  /********************************************************************************************************************/

  std::pair<std::string, std::string> MuxedInterruptDistributorFactory::
      getInterruptControllerNameAndDescriptionFromCatalogue(
          std::vector<size_t> const& subdomainID, DeviceBackend& backend) {
    for(auto const& metaDataEntry : backend.getMetadataCatalogue()) {
      auto const& key = metaDataEntry.first;
      if(key[0] == '!') {
        auto jkey = nlohmann::json::parse(std::string({++key.begin(), key.end()})); // key without the !
        auto interruptId = jkey.get<std::vector<size_t>>();

        if(interruptId == subdomainID) {
          auto jdescriptor = nlohmann::json::parse(metaDataEntry.second);
          auto controllerType = jdescriptor.begin().key();
          auto controllerDescription = jdescriptor.front().dump();
          return {controllerType, controllerDescription};
        }
      }
    }
    std::string subdomainIdString;
    for(auto i : subdomainID) {
      subdomainIdString += std::to_string(i) + ":";
    }
    subdomainIdString.pop_back();

    throw ChimeraTK::logic_error(
        "No interrupt controller description for SubDomain " + subdomainIdString + " in MetadataCatalogue");
  }

  /********************************************************************************************************************/

  boost::shared_ptr<MuxedInterruptDistributor> MuxedInterruptDistributorFactory::createMuxedInterruptDistributor(
      boost::shared_ptr<SubDomain<std::nullptr_t>> parent) {
    std::string name, description;
    std::tie(name, description) =
        getInterruptControllerNameAndDescriptionFromCatalogue(parent->getId(), *parent->getBackend());
    auto creatorFunctionIter = _creatorFunctions.find(name);
    if(creatorFunctionIter == _creatorFunctions.end()) {
      throw ChimeraTK::logic_error("Unknown interrupt controller type \"" + name + "\"");
    }
    return creatorFunctionIter->second(description, std::move(parent));
  }

  /********************************************************************************************************************/
  MuxedInterruptDistributor::MuxedInterruptDistributor(boost::shared_ptr<SubDomain<std::nullptr_t>> parent)
  : _backend(parent->getBackend()), _id(parent->getId()), _parent(std::move(parent)),
    _asyncDomain(_parent->getDomain()) {}

  /********************************************************************************************************************/
  template<typename DistributorType>
  boost::shared_ptr<AsyncAccessorManager> MuxedInterruptDistributor::getAccessorManager(
      std::vector<size_t> const& qualififedSubDomainId) {
    // the qualified SubDomainId is relative. We need an absolute (fully qualified ID) in case we have to crate a SubDomain
    assert(!qualififedSubDomainId.empty());
    auto fullyQualifiedId = _id;
    fullyQualifiedId.push_back(qualififedSubDomainId.front());

    // we can't use try_emplace because the map contains weak pointers
    boost::shared_ptr<SubDomain<std::nullptr_t>> subDomain;
    auto subDomainIter = _subDomains.find(qualififedSubDomainId.front());
    if(subDomainIter != _subDomains.end()) {
      subDomain = subDomainIter->second.lock();
    }
    if(!subDomain) {
      subDomain =
          boost::make_shared<SubDomain<std::nullptr_t>>(_backend, fullyQualifiedId, shared_from_this(), _asyncDomain);
      _subDomains[qualififedSubDomainId.front()] = subDomain;
      if(_asyncDomain->unsafeGetIsActive()) {
        // Creating a new version here is correct. Nothing has been distributed to any accessor connected to this
        // sub-interrupt yet because we just created this subDomain.
        subDomain->activate(nullptr, {});
      }
    }
    return subDomain->getAccessorManager<DistributorType>(qualififedSubDomainId);
  }

  /********************************************************************************************************************/
  void MuxedInterruptDistributor::activate(VersionNumber version) {
    for(auto& subDomainIter : _subDomains) {
      auto subDomain = subDomainIter.second.lock();
      if(subDomain) {
        subDomain->activate(nullptr, version);
      }
    }
  }

  /********************************************************************************************************************/
  void MuxedInterruptDistributor::sendException(const std::exception_ptr& e) {
    for(auto& subDomainIter : _subDomains) {
      auto subDomain = subDomainIter.second.lock();
      if(subDomain) {
        subDomain->sendException(e);
      }
    }
  }

} // namespace ChimeraTK::async
