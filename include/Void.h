// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <istream>
#include <ostream>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Wrapper Class for void. return is always  0.
   */
  class Void {
   public:
    Void() = default;
    Void& operator=(const Void&) = default;
    Void(const Void&) = default;
  };

  /********************************************************************************************************************/

  inline std::istream& operator>>(std::istream& is, __attribute__((unused)) Void& value) {
    return is;
  }

  /********************************************************************************************************************/

  inline std::ostream& operator<<(std::ostream& os, __attribute__((unused)) Void& value) {
    os << 0;
    return os;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
