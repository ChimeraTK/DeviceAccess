/*
 * RebotBackendException.cc
 *
 *  Created on: May 29, 2015
 *      Author: adagio
 */

#include "RebotBackendException.h"

namespace mtca4u {

  RebotBackendException::RebotBackendException(const std::string &_exMessage,
      unsigned int _exID)
  : DeviceBackendException(_exMessage, _exID) {}

} // namespace mtca4u
