// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "VersionNumber.h"

namespace ChimeraTK {
  std::atomic<uint64_t> VersionNumber::_lastGeneratedVersionNumber{0};

  /** Stream operator passing the human readable representation to an ostream. */
  std::ostream& operator<<(std::ostream& stream, const VersionNumber& version) {
    stream << std::string(version);
    return stream;
  }
} /* namespace ChimeraTK */
