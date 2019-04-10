#include "IEEE754_SingleConverter.h"

namespace ChimeraTK {

  template<>
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
    int32_t rawValue = *(reinterpret_cast<int32_t*>(warningAvoider));

    return rawValue;
  }

} // namespace ChimeraTK
