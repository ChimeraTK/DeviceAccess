// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <compare>
#include <cstdint>

namespace ChimeraTK::async {

  /********************************************************************************************************************/

  class DataConsistencyKey {
   public:
    /// Numeric data type for the key. Must be a valid UserType for the register accessors.
    using BaseType = uint64_t;

    /// Copy constructor
    DataConsistencyKey(const DataConsistencyKey& other) = default;

    /// Construct from numeric value
    explicit DataConsistencyKey(BaseType value) : _value(value) {}

    /// Convert into numeric value
    explicit operator BaseType() const { return _value; }

    /// Comparison operators
    std::strong_ordering operator<=>(const DataConsistencyKey&) const = default;
    std::strong_ordering operator<=>(BaseType other) const { return operator<=>(DataConsistencyKey(other)); }

    /// Increment operator
    DataConsistencyKey& operator++() {
      ++_value;
      return *this;
    }

    /// Postfix increment operator
    DataConsistencyKey operator++(int) {
      DataConsistencyKey tmp = *this;
      ++_value;
      return tmp;
    }

    /// Addition operators
    BaseType operator+(const DataConsistencyKey& other) const { return _value + other._value; }
    BaseType operator+(const BaseType& other) const { return _value + other; }

    BaseType operator-(const DataConsistencyKey& other) const { return _value - other._value; }
    BaseType operator-(const BaseType& other) const { return _value - other; }

   private:
    BaseType _value;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK::async
