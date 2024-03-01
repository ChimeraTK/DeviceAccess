// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"

namespace ChimeraTK {

  class Axi4_Intc : public InterruptControllerHandler {
   public:
    explicit Axi4_Intc(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent)
    : InterruptControllerHandler(controllerHandlerFactory, controllerID, std::move(parent)) {}
    ~Axi4_Intc() override = default;

    void handle(VersionNumber version) override;

    static std::unique_ptr<Axi4_Intc> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, std::string const& desrciption,
        boost::shared_ptr<TriggerDistributor> parent);
  };

} // namespace ChimeraTK
