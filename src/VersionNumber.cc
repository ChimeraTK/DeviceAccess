// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "VersionNumber.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  std::atomic<uint64_t> VersionNumber::_lastGeneratedVersionNumber{0};

  /********************************************************************************************************************/

  VersionNumber::operator std::string() const {
    return std::string("v") + std::to_string(_value);
  }

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream& stream, const VersionNumber& version) {
    stream << std::string(version);
    return stream;
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
