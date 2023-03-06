// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "IEEE754_SingleConverter.h"

#include "Exception.h"

namespace ChimeraTK {

  template<>
  // sorry, linter. We can't change the signature here. This is a template specialisation for std::string.
  // NOLINTNEXTLINE(performance-unnecessary-value-param)
  uint32_t IEEE754_SingleConverter::toRaw(std::string cookedValue) const {
    // step 1: convert string to float
    float genericRepresentation;
    try {
      genericRepresentation = std::stof(cookedValue);
    }
    catch(std::exception& e) {
      // FIXME: VariableName
      // Note: We cannot do out of range limitations because we don't know if too
      // large or too small
      throw ChimeraTK::logic_error(e.what());
    }

    // step 2 as in the normal template: reinterpret cast
    void* warningAvoider = &genericRepresentation;
    // FIXME: Using byte-cast in C++20 should get rid of the linter warning
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

    return rawValue;
  }

} // namespace ChimeraTK
