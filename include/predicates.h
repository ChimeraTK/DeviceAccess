// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DeviceInfoMap.h"
#include "NumericAddressedRegisterCatalogue.h"

#include <utility>

namespace ChimeraTK {

  /**
   *      @brief  Provides predicate to search device by name
   */
  class findDevByName_pred {
   private:
    std::string _name;

   public:
    explicit findDevByName_pred(std::string name) : _name(std::move(name)) {}

    bool operator()(const DeviceInfoMap::DeviceInfo& elem) { return elem.deviceName == _name; }
  };

  /**
   *      @brief  Provides predicate to compare devices by device file by name
   */
  class copmaredRegisterInfosByName2_functor {
   public:
    bool operator()(const DeviceInfoMap::DeviceInfo& first, const DeviceInfoMap::DeviceInfo& second) {
      return first.deviceName < second.deviceName;
    }
  };

} // namespace ChimeraTK
