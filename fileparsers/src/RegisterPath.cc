/*
 * RegisterPath.cc
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPath.h"

#include <iostream>

namespace ChimeraTK {

  const char RegisterPath::separator[] = "/";

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath &leftHandSide, const RegisterPath &rightHandSide) {
    leftHandSide.getCommonAltSeparator(rightHandSide); // just to check compatibility of the two RegisterPaths
    RegisterPath ret(leftHandSide);
    ret.path += rightHandSide.path;     // rightHandSide has a leading separator
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  std::string operator+(const std::string &leftHandSide, const RegisterPath &rightHandSide) {
    return leftHandSide+((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator+(const RegisterPath &leftHandSide, const std::string &rightHandSide) {
    RegisterPath ret(leftHandSide);
    ret.path += rightHandSide;
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath &leftHandSide, int rightHandSide) {
    return leftHandSide/std::to_string(rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator*(const RegisterPath &leftHandSide, int rightHandSide) {
    RegisterPath ret(std::string(leftHandSide) + std::string("*") + std::to_string(rightHandSide));
    ret.setAltSeparator(leftHandSide.separator_alt);
    ret.removeExtraSeparators();
    return ret;
  }

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream &os, const RegisterPath& me) {
    return os << std::string(me);
  }

} /* namespace ChimeraTK */
