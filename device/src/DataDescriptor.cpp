#include "DataDescriptor.h"
namespace ChimeraTK{
  DataDescriptor::DataDescriptor(DataType type) {
    if(type.isNumeric()) {
      _fundamentalType = FundamentalType::numeric;
    }
    else {
      _fundamentalType = FundamentalType::string;
    }
    _isIntegral = type.isIntegral();
    _isSigned = type.isSigned();

    // the defaults
    _nDigits = 0;           // invalid for non handled types
    _nFractionalDigits = 0; // 0 for integers

    if(type == DataType::int8) { // 8 bit signed
      _nDigits = 4;              // -127 .. 128
    }
    if(type == DataType::uint8) { // 8 bit unsigned
      _nDigits = 3;               // 0..256
    }
    else if(type == DataType::int16) { // 16 bit signed
      _nDigits = 6;                    // -32000 .. 32000 (approx)
    }
    else if(type == DataType::uint16) { // 16 bit unsigned
      _nDigits = 6;                     // 0 .. 65000 (approx)
    }
    else if(type == DataType::int32) { // 32 bit signed
      _nDigits = 11;                   // -2e9 .. 2e9 (approx)
    }
    else if(type == DataType::uint32) { // 32 bit unsigned
      _nDigits = 10;                    // 0 .. 4e9 (approx)
    }
    else if(type == DataType::int64) { // 64 bit signed
      _nDigits = 20;                   // -9e18 .. 9e18 (approx)
    }
    else if(type == DataType::uint64) { // 64 bit unsigned
      _nDigits = 20;                    // 0 .. 2e19 (approx)
    }
    else if(type == DataType::float32) { // 32 bit float IEEE 754
      _nDigits = 3 + 45;                 // todo. fix comments. see RegisterInfoMap::RegisterInfo::RegisterInfo
      _nFractionalDigits = 45;
    }
    else if(type == DataType::float64) { // 64 bit float IEEE 754
      _nDigits = 3 + 325;                //  todo. fix comments. see RegisterInfoMap::RegisterInfo::RegisterInfo
      _nFractionalDigits = 325;
    }
  }
}
