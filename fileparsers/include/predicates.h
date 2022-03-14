/**
 *      @brief          Provides set of predicates for stl algorithms
 */
#pragma once

#include "DeviceInfoMap.h"
#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK {

  /**
   *      @brief  Provides predicate to search device by name
   */
  class findDevByName_pred {
   private:
    std::string _name;

   public:
    findDevByName_pred(const std::string& name) : _name(name) {}

    bool operator()(const DeviceInfoMap::DeviceInfo& elem) {
      if(elem.deviceName == _name) return true;
      return false;
    }
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
