/*
 * RegisterInfo.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_REGISTER_INFO_H
#define CHIMERA_TK_REGISTER_INFO_H

#include "AccessMode.h"
#include "ForwardDeclarations.h"
#include "RegisterPath.h"
#include "SupportedUserTypes.h"

#include <boost/shared_ptr.hpp>

#include <cassert>
#include <iostream>

namespace ChimeraTK {

  /** Class describing the actual payload data format of a register in an
     * abstract manner. It gives information about the underlying data type
     * without fully describing it, to prevent a loss of abstraction on the
     *  application level. The returned information always refers to the data type
     * and thus is completely independent of the current value of the register. */
  class DataDescriptor {
   public:
    /** Enum for the fundamental data types. This is only used inside the
     * DataDescriptor class but defined outside to prevent too long fully
     * qualified names. */
    enum class FundamentalType { numeric, string, boolean, nodata, undefined };

    /** Get the fundamental data type */
    FundamentalType fundamentalType() const;

    /** Return whether the data is signed or not. May only be called for numeric
       * data types. */
    bool isSigned() const;

    /** Return whether the data is integral or not (e.g. int vs. float). May
       * only be called for numeric data types. */
    bool isIntegral() const;

    /** Return the approximate maximum number of digits (of base 10) needed to
       * represent the value (including a decimal dot, if not an integral data
       * type, and the sign). May only be called for numeric data types.
       *
       *  This number shall only be used for displaying purposes, e.g. to decide
       * how much space for displaying the register value should be reserved.
       * Beware that for some data types this might become a really large number
       * (e.g. 300), which indicates that you need to choose a different
       * representation than just a plain decimal number. */
    size_t nDigits() const;

    /** Approximate maximum number of digits after decimal dot (of base 10)
       * needed to represent the value (excluding the decimal dot itself). May
       * only be called for non-integral numeric data types.
       *
       *  Just like in case of nDigits(), this number should only be used for
       * displaying purposes. There is no guarantee that the full precision of the
       * number can be displayed with the given number of digits. Again beware
       * that this number might be rather large (e.g. 300). */
    size_t nFractionalDigits() const;

    /** Get the raw data type. This is the data conversion from 'cooked' to the
       raw
       *  data type on the device. This conversion does not change the shape of
       the data but
       *  descibes the data type of a single data point.
       *  \li Example 1:<br>
       *  If
       *  the raw data on the transport layer is multiplexed with fixed point
       conversion, this only describes
       *  what the raw type of the fixed point conversion is, but not the
       multiplexing.
       *  \li Example 2: (possible, currently not implemented scenario)<br>
       *  If the raw data on the transport layer is text and the data words have
       to be interpreted
       *  from the received string, the raw data will only be the text snippet
       representing the
       *  one data point.

    *  Most backends will have type none, i.e. no raw data conversion
                                            available. At the moment
                                                *  only the NumericalAddressedBackend has int32_t raw transfer with
                                                    raw/cooked conversion. Can be extended if needed,
        *  but this partily breaks abstraction because it exposes details of the
        (transport) layer below. It should be
            *  avoided if possible.
                */
    DataType rawDataType() const;

    /** Get the data type on the transport layer. This is always a 1D array of
       * the specific data type. This raw transfer might contain data for more
       * than one register.
       *
       *  Examples:
       *  \li The multiplexed data of a 2D array
       *  \li A text string containing data for multiple scalars which are mapped
       * to different registers \li The byte sequence of a "struct" with data for
       * multiple registers of different data types
       *
       *  Notice: Currently all implementations return 'none'. From the interface
       * there is no way to access the transport layer data (yet). The function is
       * put here for conceputal completeness.
       */
    DataType transportLayerDataType() const;

    /** Default constructor sets fundamental type to "undefined" */
    DataDescriptor();

    /** Constructor setting all members. */
    DataDescriptor(FundamentalType fundamentalType, bool isIntegral = false, bool isSigned = false, size_t nDigits = 0,
        size_t nFractionalDigits = 0, DataType rawDataType = DataType::none,
        DataType transportLayerDataType_ = DataType::none);

    /** Construct from DataType object - the DataDescriptor will then describe the passed DataType (with no raw
       *  type). */
    explicit DataDescriptor(DataType type);

   private:
    /** The fundamental data type */
    FundamentalType _fundamentalType;

    /** The raw data type.*/
    DataType _rawDataType;

    /** The transport layer data type.*/
    DataType _transportLayerDataType;

    /** Numeric types only: is the number integral or not */
    bool _isIntegral;

    /** Numeric types only: is the number signed or not */
    bool _isSigned;

    /** Numeric types only: approximate maximum number of digits (of base 10)
       * needed to represent the value (including a decimal dot, if not an
       * integral data type) */
    size_t _nDigits;

    /** Non-integer numeric types only: Approximate maximum number of digits
       * after decimal dot (of base 10) needed to represent the value (excluding
       * the decimal dot itself) */
    size_t _nFractionalDigits;
  };

  /*******************************************************************************************************************/

  /** DeviceBackend-independent register description. */
  class RegisterInfoImpl {
   public:

    /** Virtual destructor */
    virtual ~RegisterInfoImpl() {}

    /** Return full path name of the register (including modules) */
    virtual RegisterPath getRegisterName() const = 0;

    /** Return number of elements per channel */
    virtual unsigned int getNumberOfElements() const = 0;

    /** Return number of channels in register */
    virtual unsigned int getNumberOfChannels() const = 0;

    /** Return number of dimensions of this register */
    virtual unsigned int getNumberOfDimensions() const = 0;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    virtual const DataDescriptor& getDataDescriptor() const = 0;

    /** Return whether the register is readable. */
    virtual bool isReadable() const = 0;

    /** Return whether the register is writeable. */
    virtual bool isWriteable() const = 0;

    /** Return all supported AccessModes for this register */
    virtual AccessModeFlags getSupportedAccessModes() const = 0;
  };

  /*******************************************************************************************************************/

  class RegisterInfo {
   public:
    /** Return full path name of the register (including modules) */
    RegisterPath getRegisterName() const;

    /** Return number of elements per channel */
    unsigned int getNumberOfElements() const;

    /** Return number of channels in register */
    unsigned int getNumberOfChannels() const;

    /** Return number of dimensions of this register */
    unsigned int getNumberOfDimensions() const;

    /** Return desciption of the actual payload data for this register. See the
     * description of DataDescriptor for more information. */
    const DataDescriptor& getDataDescriptor() const;

    /** Return whether the register is readable. */
    bool isReadable() const;

    /** Return whether the register is writeable. */
    bool isWriteable() const;

    /** Return all supported AccessModes for this register */
    AccessModeFlags getSupportedAccessModes() const;

    boost::shared_ptr<RegisterInfoImpl> getImpl(); // find better name

   protected:
    boost::shared_ptr<RegisterInfoImpl> impl;
  };

  /*******************************************************************************************************************/
  /***** IMPELMENTATIONS **************/
  /*******************************************************************************************************************/

  inline DataDescriptor::FundamentalType DataDescriptor::fundamentalType() const { return _fundamentalType; }

  /*******************************************************************************************************************/

  inline DataType DataDescriptor::rawDataType() const { return _rawDataType; }

  inline DataType DataDescriptor::transportLayerDataType() const { return _transportLayerDataType; }

  /*******************************************************************************************************************/

  inline bool DataDescriptor::isSigned() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isSigned;
  }

  /*******************************************************************************************************************/

  inline bool DataDescriptor::isIntegral() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _isIntegral;
  }

  /*******************************************************************************************************************/

  inline size_t DataDescriptor::nDigits() const {
    assert(_fundamentalType == FundamentalType::numeric);
    return _nDigits;
  }

  /*******************************************************************************************************************/

  inline size_t DataDescriptor::nFractionalDigits() const {
    assert(_fundamentalType == FundamentalType::numeric && !_isIntegral);
    return _nFractionalDigits;
  }

  /*******************************************************************************************************************/

  inline DataDescriptor::DataDescriptor() : _fundamentalType(FundamentalType::undefined) {}

  /*******************************************************************************************************************/

  inline DataDescriptor::DataDescriptor(FundamentalType fundamentalType_, bool isIntegral_, bool isSigned_,
      size_t nDigits_, size_t nFractionalDigits_, DataType rawDataType_, DataType transportLayerDataType_)
  : _fundamentalType(fundamentalType_), _rawDataType(rawDataType_), _transportLayerDataType(transportLayerDataType_),
    _isIntegral(isIntegral_), _isSigned(isSigned_), _nDigits(nDigits_), _nFractionalDigits(nFractionalDigits_) {}

  /*******************************************************************************************************************/

  inline DataDescriptor::DataDescriptor(DataType type) {
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
      _nDigits = 3 + 45;                 // see RegisterInfoMap::RegisterInfo::RegisterInfo
      _nFractionalDigits = 45;
    }
    else if(type == DataType::float64) { // 64 bit float IEEE 754
      _nDigits = 3 + 325;                // see RegisterInfoMap::RegisterInfo::RegisterInfo
      _nFractionalDigits = 325;
    }
  }

  /*******************************************************************************************************************/

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_REGISTER_INFO_H */
