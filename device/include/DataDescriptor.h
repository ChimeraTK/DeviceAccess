// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "SupportedUserTypes.h"

#include <cassert>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Class describing the actual payload data format of a register in an abstract manner. It gives information about the
   * underlying data type without fully describing it, to prevent a loss of abstraction on the application level. The
   * returned information always refers to the data type and thus is completely independent of the current value of the
   * register.
   */
  class DataDescriptor {
   public:
    /**
     * Enum for the fundamental data types. This is only used inside the DataDescriptor class but defined outside to
     * prevent too long fully qualified names.
     */
    enum class FundamentalType { numeric, string, boolean, nodata, undefined };

    /**
     * Constructor setting all members.
     */
    explicit DataDescriptor(FundamentalType fundamentalType_, bool isIntegral_ = false, bool isSigned_ = false,
        size_t nDigits_ = 0, size_t nFractionalDigits_ = 0, DataType rawDataType_ = DataType::none,
        DataType transportLayerDataType_ = DataType::none);

    /**
     * Construct from DataType object - the DataDescriptor will then describe the passed DataType (with no raw type).
     */
    explicit DataDescriptor(DataType type);

    /**
     * Default constructor sets fundamental type to "undefined"
     */
    DataDescriptor();

    /** Get the fundamental data type */
    [[nodiscard]] FundamentalType fundamentalType() const;

    /** Return whether the data is signed or not. May only be called for numeric data types. */
    [[nodiscard]] bool isSigned() const;

    /** Return whether the data is integral or not (e.g. int vs. float). May only be called for numeric data types. */
    [[nodiscard]] bool isIntegral() const;

    /**
     * Return the approximate maximum number of digits (of base 10) needed to represent the value (including a decimal
     * dot, if not an integral data* type, and the sign). May only be called for numeric data types.
     *
     * This number shall only be used for displaying purposes, e.g. to decide how much space for displaying the register
     * value should be reserved. Beware that for some data types this might become a really large number (e.g. 300),
     * which indicates that you need to choose a different representation than just a plain decimal number.
     */
    [[nodiscard]] size_t nDigits() const;

    /**
     * Approximate maximum number of digits after decimal dot (of base 10) needed to represent the value (excluding the
     * decimal dot itself). May only be called for non-integral numeric data types.
     *
     * Just like in case of nDigits(), this number should only be used for displaying purposes. There is no guarantee
     * that the full precision of the number can be displayed with the given number of digits. Again beware that this
     * number might be rather large (e.g. 300).
     */
    [[nodiscard]] size_t nFractionalDigits() const;

    /**
     * Get the raw data type. This is the data conversion from 'cooked' to the raw  data type on the device. This
     * conversion does not change the shape of the data but descibes the data type of a single data point.
     *
     * \li Example 1:<br>
     * If the raw data on the transport layer is multiplexed with fixed point conversion, this only describes what the
     * raw type of the fixed point conversion is, but not the multiplexing.
     *
     * \li Example 2: (possible, currently not implemented scenario)<br>
     * If the raw data on the transport layer is text and the data words have to be interpreted from the received
     * string, the raw data will only be the text snippet representing the one data point.
     *
     * Most backends will have type none, i.e. no raw data conversion available. At the moment only the
     * NumericalAddressedBackend has int32_t raw transfer with raw/cooked conversion. Can be extended if needed, but
     * this partily breaks abstraction because it exposes details of the (transport) layer below. It should be  avoided
     * if possible.
     */
    [[nodiscard]] DataType rawDataType() const;

    /**
     * Set the raw data type. This is useful e.g. when a decorated register should no longer allow
     * raw access, in which case you should set DataType::none
     */
    void setRawDataType(const DataType& d);

    /**
     * Get the data type on the transport layer. This is always a 1D array of the specific data type. This raw transfer
     * might contain data for more than one register.
     *
     * Examples:
     * \li The multiplexed data of a 2D array
     * \li A text string containing data for multiple scalars which are mapped to different registers
     * \li The byte sequence of a "struct" with data for multiple registers of different data types
     *
     * Notice: Currently all implementations return 'none'. From the interface there is no way to access the transport
     * layer data (yet). The function is put here for conceputal completeness.
     */
    [[nodiscard]] DataType transportLayerDataType() const;

    /**
     * Get the minimum data type required to represent the described data type in the host CPU
     */
    [[nodiscard]] DataType minimumDataType() const;

    bool operator==(const DataDescriptor& other) const;
    bool operator!=(const DataDescriptor& other) const;

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

    /**
     * Numeric types only: approximate maximum number of digits (of base 10) needed to represent the value (including a
     * decimal dot, if not an integral data type)
     */
    size_t _nDigits;

    /**
     * Non-integer numeric types only: Approximate maximum number of digits after decimal dot (of base 10) needed to
     * represent the value (excluding the decimal dot itself)
     */
    size_t _nFractionalDigits;
  };

  /********************************************************************************************************************/

  std::ostream& operator<<(std::ostream& stream, const DataDescriptor::FundamentalType& fundamentalType);

  /********************************************************************************************************************/

} // namespace ChimeraTK
