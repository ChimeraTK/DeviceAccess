// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DummyInterruptControllerHandler.h"

#include "TriggerDistributor.h"
#include <nlohmann/json.hpp>

namespace ChimeraTK {

  DummyInterruptControllerHandler::DummyInterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
      std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent,
      ChimeraTK::RegisterPath const& module)
  : InterruptControllerHandler(controllerHandlerFactory, controllerID, parent), _module(module) {
    _activeInterrupts = _backend->getRegisterAccessor<uint32_t>(_module / "active_ints", 1, 0, {}); //thething to do with path: append to the name of the desired register.
    //chane module to the path. active_ints = ICR ... etc. // '/' is an overloaded operator to string append with a slash inbetween!!!
    if(!_activeInterrupts->isReadable()) {
      throw ChimeraTK::runtime_error("DummyInterruptControllerHandler: Handshake register not readable: " + _activeInterrupts->getName());
    }
  }

  /*****************************************************************************************************************/

  void DummyInterruptControllerHandler::handle(VersionNumber version) {
    //version number uniquely tags the interupt.
    try {
      _activeInterrupts->read();
      for(uint32_t i = 0; i < 32; ++i) {
        if(_activeInterrupts->accessData(0) & 0x1U << i) { //accessData(0) 0 because it's a 2D object, and we're accessing element 0,[0]. 
        //here also check enable
        //0x1U unsigned long, just the right most bit, shifted to the ith bit, grab that bit. 
          try {
            auto distributor = _distributors.at(i).lock();
            if(distributor) {
              distributor->distribute(nullptr, version);
            }
          }
          catch(std::out_of_range&) {
            _backend->setException("ERROR: DummyInterruptControllerHandler reports unknown active interrupt " + std::to_string(i));
          }
        }
      }
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
    }
  }

  /*****************************************************************************************************************/

  std::unique_ptr<DummyInterruptControllerHandler> DummyInterruptControllerHandler::create(InterruptControllerHandlerFactory* controllerHandlerFactory,
      std::vector<uint32_t> const& controllerID, std::string const& desrciption,
      boost::shared_ptr<TriggerDistributor> parent) {
    auto jdescription = nlohmann::json::parse(desrciption);
    auto module = jdescription["module"].get<std::string>();
    return std::make_unique<DummyInterruptControllerHandler>(controllerHandlerFactory, controllerID, std::move(parent), module);
  }

} // namespace ChimeraTK
