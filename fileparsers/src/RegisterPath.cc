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

  std::string operator+(const RegisterPath &leftHandSide, const std::string &rightHandSide) {
    return ((std::string)leftHandSide)+rightHandSide;
  }

  /********************************************************************************************************************/

  std::string operator+(const std::string &leftHandSide, const RegisterPath &rightHandSide) {
    return leftHandSide+((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  std::string operator+(const RegisterPath &leftHandSide, const RegisterPath &rightHandSide) {
    return ((std::string)leftHandSide)+((std::string)rightHandSide);
  }

  /********************************************************************************************************************/

  RegisterPath operator/(const std::string &leftHandSide, const RegisterPath &rightHandSide) {
    RegisterPath temp(leftHandSide);
    return temp+rightHandSide;
  }

} /* namespace mtca4u */
