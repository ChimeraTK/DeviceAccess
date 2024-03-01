// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"
#include "NDRegisterAccessor.h"
#include "RegisterPath.h"

namespace ChimeraTK {

  class DummyIntc : public InterruptControllerHandler {
   public:
    explicit DummyIntc(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent,
        RegisterPath const& module);
    ~DummyIntc() override = default;

    void handle(VersionNumber version) override;

    static std::unique_ptr<DummyIntc> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, std::string const& desrciption,
        boost::shared_ptr<TriggerDistributor> parent);

   protected:
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _activeInterrupts;
    RegisterPath _module;
  };

} // namespace ChimeraTK
