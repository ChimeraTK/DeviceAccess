// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"
#include "NDRegisterAccessor.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  class DummyInterruptControllerHandler : public InterruptControllerHandler {
   public:
    explicit DummyInterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, 
        boost::shared_ptr<TriggerDistributor> parent,
        RegisterPath const& module);
    ~DummyInterruptControllerHandler() override = default;

    void handle(VersionNumber version) override;

    static std::unique_ptr<DummyInterruptControllerHandler> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, 
        std::string const& desrciption, //THIS IS THE JSON SNIPPET
        boost::shared_ptr<TriggerDistributor> parent);

   protected:
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _activeInterrupts;
    RegisterPath _module;
  };

} // namespace ChimeraTK

/*********************************************************************************************************************/

void DummyInterruptControllerHandler::handle(VersionNumber version) {
  try {
    _active_Interrupts->read();
    for(uint32_t i = 0; i< 32; ++i) {
      if(_activeInterrupts->accessData(0) & 0x1U << i) {
        try {
          auto distributor = _distributors.at(i).lock();
          if(distributor) {
            distributor->distribute(nullptr, version);
          }
        }
        catch(std::out-of_range&){
          _backend->setException("Error: DUmmyIntc reports unknown activit interrupt "+ std::to_string(i));
        }
      }
    }
  }
  catch(ChimeraTK::runtime_error&) {
    //Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
  }
}