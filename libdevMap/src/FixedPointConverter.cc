#include "FixedPointConverter.h"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace mtca4u{
  
  FixedPointConverter::FixedPointConverter(unsigned int nBits,
					   int fractionalBits,
					   bool isSigned)
    : _nBits(nBits), _fractionalBits(fractionalBits), _isSigned(isSigned),
      _fractionalBitsCoefficient(std::pow( 2., -fractionalBits)),
      _inverseFractionalBitsCoefficient(std::pow( 2., fractionalBits)){
  
    // some sanity checks
    if (nBits > 32){
      std::stringstream errorMessage;
      errorMessage << "The number of bits must be <= 32, but is " << nBits;
      throw std::invalid_argument(errorMessage.str());
    }
    
    if (nBits == 0){
      throw std::invalid_argument("A word with zero significant bits is not valid.");
    }

    if ( (fractionalBits > 1023-static_cast<int>(nBits)) || 
	 (fractionalBits < -1024+static_cast<int>(nBits)) ){
      std::stringstream errorMessage;
      errorMessage << "The number of fractional bits exceeds the dynamic"
		   << " range of a double.";
      throw std::invalid_argument(errorMessage.str());
    }

    _signBitMask = 1<<(nBits-1); // the highest valid bit is the sign
    _usedBitsMask = (1L << nBits) -1L; // by using 1L (64 bit) this also works for 32 bits
    _unusedBitsMask = ~_usedBitsMask;
  }

  double FixedPointConverter::toDouble(uint32_t fixedPointValue) const{
    // the value how the word looks like as integer in double representation,
    // i.e. before applying the fixed comma offset, but interpreting the
    // sign and the number of bits
    double unscaledValue;
    if (_isSigned){
      // for signed we have to fill the leading bits with 1 in case
      // the value is negative, so the cast to double gives correct results
      if (fixedPointValue & _signBitMask){ //negative branch
        unscaledValue = static_cast<double>( 
	                  static_cast<int32_t>(fixedPointValue | _unusedBitsMask));
      }else{ //positive branch
	// do not fill in leading 1s for positive values,
	// fist cast to signed32 so the leading bit is interpreted correctly when 
	// casting to double
	unscaledValue = static_cast<double>( static_cast<int32_t>(fixedPointValue) );
      }
    }else{ // unsigned
      // directly cast to double
      unscaledValue = static_cast<double>(fixedPointValue);      
    }
    
    return unscaledValue * _fractionalBitsCoefficient;
  }
  
  uint32_t FixedPointConverter::toFixedPoint(double floatingPointValue) const{
    // apply the coeffitient towards fractional bits
    double scaledValue = floatingPointValue * _inverseFractionalBitsCoefficient;
    
    return static_cast<uint32_t>(round(scaledValue)) & _usedBitsMask;    
  }

}// namespace mtca4u


