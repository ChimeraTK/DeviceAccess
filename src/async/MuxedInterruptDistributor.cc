// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/MuxedInterruptDistributor.h"

#include "async/DummyMuxedInterruptDistributor.h"
#include "async/GenericMuxedInterruptDistributor.h"
#include "async/SubDomain.h"
#include "async/TriggeredPollDistributor.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <nlohmann/json.hpp> // https://json.nlohmann.me/features/element_access/

using json = nlohmann::json;

#include <cassert>
#include <tuple>

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  MuxedInterruptDistributorFactory::MuxedInterruptDistributorFactory() {
    // we already know about the build-in handlers
    _controllerTypes["INTC"] = {
        GenericMuxedInterruptDistributor::create, GenericMuxedInterruptDistributor::augmentCatalogue};
    _controllerTypes["dummy"] = {DummyMuxedInterruptDistributor::create, nullptr};
  }

  /********************************************************************************************************************/

  /**
   * Extract the controller type and its description (the dumped value) of an interrupt handler entry's controller
   * object. The entry reader records the invariant that exactly one controller key remains (it erases 'subhandler' and
   * drops underscore-prefixed keys), so the check guards against a malformed entry silently picking the wrong key. The
   * check validates rather than asserts, since this copy also reads the descriptor of a traditional map file from the
   * metadata catalogue, which never passes through the entry reader and is otherwise unchecked.
   */
  static std::pair<std::string, std::string> getInterruptControllerNameAndDescription(json const& descriptor) {
    if(!descriptor.is_object() || descriptor.size() != 1) {
      throw ChimeraTK::logic_error(
          "An interrupt controller descriptor must be an object with exactly one controller key.");
    }
    auto controllerType = descriptor.begin().key();
    auto controllerDescription = descriptor.front().dump();
    return {controllerType, controllerDescription};
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
        try {
          auto jkey = nlohmann::json::parse(std::string({++key.begin(), key.end()})); // key without the leading !
          auto interruptId = jkey.get<std::vector<size_t>>();

          if(interruptId == subdomainID) {
            auto jdescriptor = nlohmann::json::parse(metaDataEntry.second);
            return getInterruptControllerNameAndDescription(jdescriptor);
          }
        }
        catch(json::parse_error& e) {
          throw ChimeraTK::logic_error("Json parse error in \"" + key + "\": " + e.what());
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
    auto recordIter = _controllerTypes.find(name);
    if(recordIter == _controllerTypes.end()) {
      throw ChimeraTK::logic_error("Unknown interrupt controller type \"" + name + "\"");
    }
    return recordIter->second.creator(description, std::move(parent));
  }

  /********************************************************************************************************************/

  void MuxedInterruptDistributorFactory::augmentCatalogue(std::vector<size_t> const& interruptId,
      std::string const& controllerType, std::string const& controllerDescription,
      NumericAddressedRegisterCatalogue& catalogue) {
    auto recordIter = _controllerTypes.find(controllerType);
    if(recordIter == _controllerTypes.end()) {
      return;
    }
    if(!recordIter->second.augmenter) {
      return;
    }
    recordIter->second.augmenter(interruptId, controllerDescription, catalogue);
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
        activateSubDomain(*subDomain, {});
      }
    }
    return subDomain->getAccessorManager<DistributorType>(qualififedSubDomainId);
  }

  /********************************************************************************************************************/

  template boost::shared_ptr<AsyncAccessorManager> MuxedInterruptDistributor::getAccessorManager<
      TriggeredPollDistributor>(std::vector<size_t> const& qualififedSubDomainId);

  /********************************************************************************************************************/

  template boost::shared_ptr<AsyncAccessorManager> MuxedInterruptDistributor::getAccessorManager<
      VariableDistributor<std::nullptr_t>>(std::vector<size_t> const& qualififedSubDomainId);

  /********************************************************************************************************************/
  void MuxedInterruptDistributor::activate(VersionNumber version) {
    for(auto& subDomainIter : _subDomains) {
      auto subDomain = subDomainIter.second.lock();
      if(subDomain) {
        activateSubDomain(*subDomain, version);
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

  /********************************************************************************************************************/
  void MuxedInterruptDistributor::activateSubDomain(
      SubDomain<std::nullptr_t>& subDomain, VersionNumber const& version) {
    subDomain.activate(nullptr, version);
  }

} // namespace ChimeraTK::async
