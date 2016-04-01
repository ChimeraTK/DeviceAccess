/*
 * RegisterPath.cc
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#include "RegisterPath.h"

namespace mtca4u {

  const char RegisterPath::separator[] = "/";
  const char RegisterPath::separator_alt[] = ".";

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath &leftHandSide, const RegisterPath &rightHandSide) {
    return ((std::string)leftHandSide)+((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  std::string operator+(const std::string &leftHandSide, const RegisterPath &rightHandSide) {
    return leftHandSide+((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  std::string operator+(const RegisterPath &leftHandSide, const std::string &rightHandSide) {
    return ((std::string)leftHandSide)+rightHandSide;
  }

  /********************************************************************************************************************/

  RegisterPath operator/(const RegisterPath &leftHandSide, int rightHandSide) {
    return leftHandSide/std::to_string(rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator*(const RegisterPath &leftHandSide, int rightHandSide) {
    return std::string(leftHandSide) + std::string("*") + std::to_string(rightHandSide);
  }

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream &os, const RegisterPath& me) {
    return os << std::string(me);
  }

} /* namespace mtca4u */
