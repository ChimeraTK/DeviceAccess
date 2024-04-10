// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "GenericInterruptControllerHandler.h"

#include "TriggerDistributor.h"

namespace ChimeraTK {

  void GenericInterruptControllerHandler::handle(VersionNumber version) {
    // Stupid testing implementation that always triggers all children
    for(auto& distributorIter : _distributors) {
      auto distributor = distributorIter.second.lock();
      // The weak pointer might have gone.
      // FIXME: We need a cleanup function which removes the map entry. Otherwise we might
      // be stuck with a bad weak pointer wich is tried in each handle() call.
      if(distributor) {
        distributor->distribute(nullptr, version);
      }
    }
  }

  /********************************************************************************************************************/

  std::unique_ptr<GenericInterruptControllerHandler> GenericInterruptControllerHandler::create(
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      [[maybe_unused]] std::string const& desrciption, boost::shared_ptr<TriggerDistributor> parent) {
    return std::make_unique<GenericInterruptControllerHandler>(
        controllerHandlerFactory, controllerID, std::move(parent));
  }

} // namespace ChimeraTK
