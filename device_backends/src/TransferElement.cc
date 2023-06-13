// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "TransferElement.h"

namespace ChimeraTK {

  std::ostream& operator<<(std::ostream& os, const DataValidity& validity) {
    if(validity == DataValidity::ok) {
      os << "ok";
    }
    else {
      os << "faulty";
    }
    return os;
  }

} // namespace ChimeraTK