// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>
#include <string>
#include <tuple>

namespace ChimeraTK {

  class AsyncDomainsContainerBase {
   public:
    virtual ~AsyncDomainsContainerBase() = default;
    bool isSendingExceptions() { return _isSendingExceptions; }
    virtual void sendExceptions(const std::string& exceptionMessage) { std::ignore = exceptionMessage; }

   protected:
    std::atomic_bool _isSendingExceptions{false};
  };

} // namespace ChimeraTK
