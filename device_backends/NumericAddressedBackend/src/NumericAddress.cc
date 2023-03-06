// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "NumericAddress.h"

namespace ChimeraTK::numeric_address {

  RegisterPath BAR() {
    static const RegisterPath bar{"#"};
    return bar;
  }

} // namespace ChimeraTK::numeric_address
