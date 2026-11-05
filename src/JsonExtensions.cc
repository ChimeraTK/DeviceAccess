// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "JsonExtensions.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  std::pair<std::string, std::string> interruptControllerNameAndDescription(nlohmann::json const& descriptor) {
    auto controllerType = descriptor.begin().key();
    auto controllerDescription = descriptor.front().dump();
    return {controllerType, controllerDescription};
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
