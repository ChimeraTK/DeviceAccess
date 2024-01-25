// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DataDescriptor.h"
namespace ChimeraTK {

  /********************************************************************************************************************/

  DataDescriptor::DataDescriptor(FundamentalType fundamentalType_, bool isIntegral_, bool isSigned_, size_t nDigits_,
      size_t nFractionalDigits_, DataType rawDataType_, DataType transportLayerDataType_)
  : _fundamentalType(fundamentalType_), _rawDataType(rawDataType_), _transportLayerDataType(transportLayerDataType_),
    _isIntegral(isIntegral_), _isSigned(isSigned_), _nDigits(nDigits_), _nFractionalDigits(nFractionalDigits_) {}

  /********************************************************************************************************************/

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
    else if(type == DataType::uint8) { // 8 bit unsigned
      _nDigits = 3;                    // 0..256
    }
    else if(type == DataType::int16) { // 16 bit signed
      _nDigits = 6;                    // -32000 .. 32000 (approx)
    }
    else if(type == DataType::uint16) { // 16 bit unsigned
      _nDigits = 5;                     // 0 .. 65000 (approx)
    }
    else if(type == DataType::int32) { // 32 bit signed
      _nDigits = 11;                   // -2e9 .. 2e9 (approx)
    }
    else if(type == DataType::uint32) { // 32 bit unsigned
      _nDigits = 10;                    // 0 .. 4e9 (approx)
    }
    else if(type == DataType::int64) { // 64 bit signed // NOLINT(bugprone-branch-clone)
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
    else if(type == DataType::Boolean) {
      _fundamentalType = FundamentalType::boolean;
    }
    else if(type == DataType::Void) {
      _fundamentalType = FundamentalType::nodata;
    }
  }

  /********************************************************************************************************************/

  DataDescriptor::DataDescriptor()
  : _fundamentalType(FundamentalType::undefined), _isIntegral(false), _isSigned(false), _nDigits(0),
    _nFractionalDigits(0) {}

  /********************************************************************************************************************/

  DataDescriptor::FundamentalType DataDescriptor::fundamentalType() const {
    return _fundamentalType;
  }

  /********************************************************************************************************************/

  bool DataDescriptor::isSigned() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isSigned;
  }

  /********************************************************************************************************************/

  bool DataDescriptor::isIntegral() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isIntegral;
  }

  /********************************************************************************************************************/

  size_t DataDescriptor::nDigits() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _nDigits;
  }

  /********************************************************************************************************************/

  size_t DataDescriptor::nFractionalDigits() const {
    assert(_fundamentalType == FundamentalType::numeric && !_isIntegral);
    return _nFractionalDigits;
  }

  /********************************************************************************************************************/

  DataType DataDescriptor::rawDataType() const {
    return _rawDataType;
  }

  void DataDescriptor::setRawDataType(const DataType& d) {
    _rawDataType = d;
  }

  /********************************************************************************************************************/

  DataType DataDescriptor::transportLayerDataType() const {
    return _transportLayerDataType;
  }

  /********************************************************************************************************************/

  bool DataDescriptor::operator==(const DataDescriptor& other) const {
    return _fundamentalType == other._fundamentalType && _rawDataType == other._rawDataType &&
        _transportLayerDataType == other._transportLayerDataType && _isIntegral == other._isIntegral &&
        _isSigned == other._isSigned && _nDigits == other._nDigits && _nFractionalDigits == other._nFractionalDigits;
  }

  /********************************************************************************************************************/

  bool DataDescriptor::operator!=(const DataDescriptor& other) const {
    return !operator==(other);
  }

  /********************************************************************************************************************/

  DataType DataDescriptor::minimumDataType() const {
    if(fundamentalType() == DataDescriptor::FundamentalType::numeric) {
      if(isIntegral()) {
        if(isSigned()) {
          if(nDigits() > 11) {
            return typeid(int64_t);
          }
          if(nDigits() > 6) {
            return typeid(int32_t);
          }
          if(nDigits() > 4) {
            return typeid(int16_t);
          }
          return typeid(int8_t);
        }
        if(nDigits() > 10) {
          return typeid(uint64_t);
        }
        if(nDigits() > 5) {
          return typeid(uint32_t);
        }
        if(nDigits() > 3) {
          return typeid(uint16_t);
        }
        return typeid(uint8_t);
      }
      // fractional:
      // Maximum number of decimal digits to display a float without loss in non-exponential display, including
      // sign, leading 0, decimal dot and one extra digit to avoid rounding issues (hence the +4).
      // This computation matches the one performed in the NumericAddressedBackend catalogue.
      size_t floatMaxDigits = 4 +
          size_t(std::max(
              std::log10(std::numeric_limits<float>::max()), -std::log10(std::numeric_limits<float>::denorm_min())));
      if(nDigits() > floatMaxDigits) {
        return typeid(double);
      }
      return typeid(float);
    }
    if(fundamentalType() == DataDescriptor::FundamentalType::boolean) {
      return typeid(ChimeraTK::Boolean);
    }
    if(fundamentalType() == DataDescriptor::FundamentalType::string) {
      return typeid(std::string);
    }
    if(fundamentalType() == DataDescriptor::FundamentalType::nodata) {
      return typeid(ChimeraTK::Void);
    }
    return {};
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream& stream, const DataDescriptor::FundamentalType& fundamentalType) {
    if(fundamentalType == DataDescriptor::FundamentalType::nodata) {
      stream << "nodata";
    }
    else if(fundamentalType == DataDescriptor::FundamentalType::boolean) {
      stream << "boolean";
    }
    else if(fundamentalType == DataDescriptor::FundamentalType::numeric) {
      stream << "numeric";
    }
    else if(fundamentalType == DataDescriptor::FundamentalType::string) {
      stream << "string";
    }
    else if(fundamentalType == DataDescriptor::FundamentalType::undefined) {
      stream << "undefined";
    }
    return stream;
  }
  /********************************************************************************************************************/

} // namespace ChimeraTK
