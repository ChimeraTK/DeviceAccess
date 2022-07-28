// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorAbstractor.h"

namespace ChimeraTK {

  // We cannot use a typedef because the constructor of NDRegisterAccessorAbstractor is intentionally
  // protected.
  class VoidRegisterAccessor : public NDRegisterAccessorAbstractor<ChimeraTK::Void> {
   public:
    VoidRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<Void>> impl);
    VoidRegisterAccessor() {}
    bool isReadOnly() const;
    bool isReadable() const;
    void read();
    bool readNonBlocking();
    bool readLatest();
  };

} // namespace ChimeraTK
