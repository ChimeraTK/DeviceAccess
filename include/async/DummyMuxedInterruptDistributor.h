// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "../NDRegisterAccessor.h"
#include "../RegisterPath.h"
#include "MuxedInterruptDistributor.h"

namespace ChimeraTK::async {

  class DummyMuxedInterruptDistributor : public MuxedInterruptDistributor {
   public:
    explicit DummyMuxedInterruptDistributor(
        boost::shared_ptr<SubDomain<std::nullptr_t>> parent, RegisterPath const& module);
    ~DummyMuxedInterruptDistributor() override = default;

    void handle(VersionNumber version) override;

    static std::unique_ptr<DummyMuxedInterruptDistributor> create(
        std::string const& desrciption, boost::shared_ptr<SubDomain<std::nullptr_t>> parent);

   protected:
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _activeInterrupts;
    RegisterPath _module;
  };

} // namespace ChimeraTK::async
