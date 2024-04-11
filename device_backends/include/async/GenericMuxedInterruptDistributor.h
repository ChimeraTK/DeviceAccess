// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "async/MuxedInterruptDistributor.h"

namespace ChimeraTK::async {

  class GenericMuxedInterruptDistributor : public MuxedInterruptDistributor {
   public:
    explicit GenericMuxedInterruptDistributor(boost::shared_ptr<SubDomain<std::nullptr_t>> parent)
    : MuxedInterruptDistributor(std::move(parent)) {}
    ~GenericMuxedInterruptDistributor() override = default;

    void handle(VersionNumber version) override;

    static std::unique_ptr<GenericMuxedInterruptDistributor> create(
        std::string const& desrciption, boost::shared_ptr<SubDomain<std::nullptr_t>> parent);
  };

} // namespace ChimeraTK::async
