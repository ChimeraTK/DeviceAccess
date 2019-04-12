#include "FixedPointConverter.h"
#include "Exception.h"

namespace ChimeraTK {

  const int FixedPointConverter::zero = 0;

  FixedPointConverter::FixedPointConverter(
      std::string variableName, unsigned int nBits, int fractionalBits, bool isSignedFlag)
  : _variableName(variableName), _nBits(nBits), _fractionalBits(fractionalBits), _isSigned(isSignedFlag),
    _fractionalBitsCoefficient(pow(2., -fractionalBits)), _inverseFractionalBitsCoefficient(pow(2., fractionalBits)) {
    reconfigure(nBits, fractionalBits, isSignedFlag);
  }

  /**********************************************************************************************************************/

  void FixedPointConverter::reconfigure(unsigned int nBits, int fractionalBits, bool isSignedFlag) {
    _nBits = nBits;
    _fractionalBits = fractionalBits;
    _isSigned = isSignedFlag;
    _fractionalBitsCoefficient = pow(2., -fractionalBits);
    _inverseFractionalBitsCoefficient = pow(2., fractionalBits);

    _bitShiftMaskSigned = 0; // Fixme unused variable.
    // some sanity checks
    if(nBits > 32) {
      std::stringstream errorMessage;
      errorMessage << "The number of bits must be <= 32, but is " << nBits;
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    // For floating-point types: check if number of fractional bits are complying
    // with the dynamic range Note: positive fractional bits give us smaller
    // numbers and thus correspond to negative exponents!
    if((fractionalBits > -std::numeric_limits<double>::min_exponent - static_cast<int>(nBits)) ||
        (fractionalBits < -std::numeric_limits<double>::max_exponent + static_cast<int>(nBits))) {
      std::stringstream errorMessage;
      errorMessage << "The number of fractional bits exceeds the dynamic"
                   << " range of a double.";
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    // compute mask for the signed bit
    // keep the mask at 0 if unsigned to simplify further calculations
    if(nBits > 0) {
      _signBitMask = (_isSigned ? 1 << (nBits - 1) : 0x0); // the highest valid bit is the sign
    }
    else {
      _signBitMask = 0;
    }

    // compute masks of used and unused bits
    _usedBitsMask = (1L << nBits) - 1L; // by using 1L (64 bit) this also works for 32 bits
    _unusedBitsMask = ~_usedBitsMask;

    // compute bit shift mask, used to test if bit shifting for fractional bits
    // leads to an overflow
    _bitShiftMask = ~(0xFFFFFFFF >> abs(_fractionalBits));

    // compute minimum and maximum value in raw representation
    _maxRawValue = _usedBitsMask ^ _signBitMask; // bitwise xor: first bit is 0 if signed
    _minRawValue = _signBitMask;                 // if only the sign bit is on, it is the smallest
                                                 // possible value
    // (0 if unsigned)

    // fill all user type depending values: min and max cooked values and
    // fractional bit coefficients note: we loop over one of the maps only, but
    // initCoefficients() will fill all maps!
    boost::fusion::for_each(_minCookedValues, initCoefficients(this));
  }

  /**********************************************************************************************************************/

  template<>
  uint32_t FixedPointConverter::toRaw<std::string>(std::string cookedValue) const {
    if(_fractionalBits == 0) { // use integer conversion
      if(_isSigned) {
        return toRaw(std::stoi(cookedValue));
      }
      else {
        return toRaw(static_cast<uint32_t>(std::stoul(cookedValue))); // on some compilers, long might be a
                                                                      // different type than int...
      }
    }
    else {
      return toRaw(std::stod(cookedValue));
    }
  }

} // namespace ChimeraTK
