// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TypeChangingDecorator.h"

namespace ChimeraTK {

  // The global instance of the map
  std::map<DecoratorMapKey, boost::shared_ptr<ChimeraTK::TransferElement>> globalDecoratorMap;

  std::map<DecoratorMapKey, boost::shared_ptr<ChimeraTK::TransferElement>>& getGlobalDecoratorMap() {
    return globalDecoratorMap;
  }

  // the implementations of the full template specialisations
  namespace csa_helpers {

    template<>
    int8_t stringToT<int8_t>(std::string const& input) {
      std::stringstream s;
      s << input;
      int32_t t;
      s >> t;
      return t;
    }

    template<>
    uint8_t stringToT<uint8_t>(std::string const& input) {
      std::stringstream s;
      s << input;
      uint32_t t;
      s >> t;
      return t;
    }

    template<>
    std::string T_ToString<uint8_t>(uint8_t input) {
      std::stringstream s;
      s << static_cast<uint32_t>(input);
      std::string output;
      s >> output;
      return output;
    }

    template<>
    std::string T_ToString<int8_t>(int8_t input) {
      std::stringstream s;
      s << static_cast<int32_t>(input);
      std::string output;
      s >> output;
      return output;
    }
  } // namespace csa_helpers

} // namespace ChimeraTK
