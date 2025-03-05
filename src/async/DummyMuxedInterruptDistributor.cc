// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "async/DummyMuxedInterruptDistributor.h"

#include "async/SubDomain.h"
#include <nlohmann/json.hpp>

namespace ChimeraTK::async {

  DummyMuxedInterruptDistributor::DummyMuxedInterruptDistributor(
      boost::shared_ptr<SubDomain<std::nullptr_t>> parent, ChimeraTK::RegisterPath const& module)
  : MuxedInterruptDistributor(std::move(parent)), _module(module) {
    _activeInterrupts = _backend->getRegisterAccessor<uint32_t>(_module / "active_ints", 1, 0, {});
    if(!_activeInterrupts->isReadable()) {
      throw ChimeraTK::runtime_error(
          "DummyMuxedInterruptDistributor: Handshake register not readable: " + _activeInterrupts->getName());
    }
  }

  /********************************************************************************************************************/

  void DummyMuxedInterruptDistributor::handle(VersionNumber version) {
    try {
      _activeInterrupts->read();
      for(uint32_t i = 0; i < 32; ++i) {
        if(_activeInterrupts->accessData(0) & 0x1U << i) {
          try {
            auto subDomain = _subDomains.at(i).lock();
            if(subDomain) {
              subDomain->distribute(nullptr, version);
            }
          }
          catch(std::out_of_range&) {
            _backend->setException(
                "ERROR: DummyMuxedInterruptDistributor reports unknown active interrupt " + std::to_string(i));
          }
        }
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
    }
  }

  /********************************************************************************************************************/

  std::unique_ptr<DummyMuxedInterruptDistributor> DummyMuxedInterruptDistributor::create(
      std::string const& desrciption, boost::shared_ptr<SubDomain<std::nullptr_t>> parent) {
    auto jdescription = nlohmann::json::parse(desrciption);
    auto module = jdescription["module"].get<std::string>();
    return std::make_unique<DummyMuxedInterruptDistributor>(std::move(parent), module);
  }

} // namespace ChimeraTK::async
