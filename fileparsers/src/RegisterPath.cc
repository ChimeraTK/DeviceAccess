// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "RegisterPath.h"

#include <iostream>
#include <tuple>

namespace ChimeraTK {

  const char RegisterPath::separator[] = "/";

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath& leftHandSide, const RegisterPath& rightHandSide) {
    std::ignore =
        leftHandSide.getCommonAltSeparator(rightHandSide); // just to check compatibility of the two RegisterPaths
    RegisterPath ret(leftHandSide);
    ret.path += rightHandSide.path; // rightHandSide has a leading separator
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  std::string operator+(const std::string& leftHandSide, const RegisterPath& rightHandSide) {
    return leftHandSide + ((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator+(const RegisterPath& leftHandSide, const std::string& rightHandSide) {
    RegisterPath ret(leftHandSide);
    ret.path += rightHandSide;
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath& leftHandSide, int rightHandSide) {
    return leftHandSide / std::to_string(rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator*(const RegisterPath& leftHandSide, int rightHandSide) {
    RegisterPath ret(std::string(leftHandSide) + std::string("*") + std::to_string(rightHandSide));
    ret.setAltSeparator(leftHandSide.separator_alt);
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream& os, const RegisterPath& me) {
    return os << std::string(me);
  }

} /* namespace ChimeraTK */
