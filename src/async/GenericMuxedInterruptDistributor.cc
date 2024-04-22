// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/GenericMuxedInterruptDistributor.h"

#include "async/SubDomain.h"

namespace ChimeraTK::async {

  void GenericMuxedInterruptDistributor::handle(VersionNumber version) {
    // Stupid testing implementation that always triggers all children
    for(auto& subDomainIter : _subDomains) {
      auto subDomain = subDomainIter.second.lock();
      // The weak pointer might have gone.
      // FIXME: We need a cleanup function which removes the map entry. Otherwise we might
      // be stuck with a bad weak pointer wich is tried in each handle() call.
      if(subDomain) {
        subDomain->distribute(nullptr, version);
      }
    }
  }

  /********************************************************************************************************************/

  std::unique_ptr<GenericMuxedInterruptDistributor> GenericMuxedInterruptDistributor::create(
      [[maybe_unused]] std::string const& desrciption, boost::shared_ptr<SubDomain<std::nullptr_t>> parent) {
    return std::make_unique<GenericMuxedInterruptDistributor>(std::move(parent));
  }

} // namespace ChimeraTK::async
