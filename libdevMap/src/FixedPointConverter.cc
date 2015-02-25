#include "FixedPointConverter.h"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace mtca4u{
  
  FixedPointConverter::FixedPointConverter(unsigned int nBits,
					   int fractionalBits,
					   bool isSignedFlag)
    : _nBits(nBits), _fractionalBits(fractionalBits), _isSigned(isSignedFlag),
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

    _signBitMask = ( _isSigned ? 1<<(nBits-1) : 0x0 ); // the highest valid bit is the sign
    // keep the mask at 0 if unsigned to simplify further calculations
    _usedBitsMask = (1L << nBits) -1L; // by using 1L (64 bit) this also works for 32 bits
    _unusedBitsMask = ~_usedBitsMask;

    _maxFixedPointValue = _usedBitsMask ^ _signBitMask; // bitwise xor: first bit is 0 if signed 
    _minFixedPointValue = _signBitMask; // if only the sign bit is on, it is the smallest possible value 
                                        // (0 if unsigned)

    _minDoubleValue = toDouble( _minFixedPointValue );
    _maxDoubleValue = toDouble( _maxFixedPointValue );
  }

  double FixedPointConverter::toDouble(uint32_t fixedPointValue) const{
    // leading, out of range bits are ignored. Just crop them.
    fixedPointValue &= _usedBitsMask;

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
    // do a range check first
    if (floatingPointValue <  _minDoubleValue){
      return _minFixedPointValue;
    }
    if (floatingPointValue >  _maxDoubleValue){
      return _maxFixedPointValue;
    }

    // apply the coeffitient towards fractional bits
    double scaledValue = floatingPointValue * _inverseFractionalBitsCoefficient;
    
    return static_cast<uint32_t>(round(scaledValue)) & _usedBitsMask;    
  }

  unsigned int FixedPointConverter::getNBits(){
    return _nBits;
  }

  int FixedPointConverter::getFractionalBits(){
    return _fractionalBits;
  }

  bool FixedPointConverter::isSigned(){
    return _isSigned;
  }

}// namespace mtca4u


