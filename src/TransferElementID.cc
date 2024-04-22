// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TransferElementID.h"

#include <atomic>
#include <cassert>
#include <sstream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void TransferElementID::makeUnique() {
    static std::atomic<size_t> nextId{0};
    assert(_id == 0);
    ++nextId;
    assert(nextId != 0);
    _id = nextId;
  }

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream& os, const TransferElementID& me) {
    std::stringstream ss;
    ss << std::hex << std::showbase << me._id;
    os << ss.str();
    return os;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
