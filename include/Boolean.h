// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <algorithm>
#include <istream>
#include <limits>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Wrapper Class to avoid vector<bool> problems
   */
  class Boolean {
   public:
    constexpr Boolean() : _value() {}
    // We want implicit construction and conversion. Turn off the linter warnings.
    // NOLINTBEGIN(hicpp-explicit-conversions, google-explicit-constructor)
    constexpr Boolean(bool value) : _value(value) {}

    constexpr operator const bool&() const { return _value; }
    constexpr operator bool&() { return _value; }
    // NOLINTEND(hicpp-explicit-conversions, google-explicit-constructor)
    // TODO: test user types to numeric etc

   private:
    bool _value;
  };

  /********************************************************************************************************************/

  inline std::istream& operator>>(std::istream& is, Boolean& value) {
    std::string data;
    is >> data;

    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });

    if(data == "false" || data == "0" || data.empty()) {
      value = false;
    }
    else {
      value = true;
    }
    return is;
  }

  /********************************************************************************************************************/

  template<typename T>
  constexpr bool isBoolean = std::is_same_v<T, bool> || std::is_same_v<T, Boolean>;

  /********************************************************************************************************************/

  // Define ChimeraTK::to_string to convert Boolean into string. We cannot define std::to_string(Boolean), as it would
  // violate the C++ standard. The right definition can be autoselected with
  // "using std::to_string; using ChimeraTK::to_string;".
  // NOLINTNEXTLINE(readability-identifier-naming) - name is fixed by C++ standard
  inline std::string to_string(Boolean& value) {
    if(value) {
      return "true";
    }
    return "false";
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

/********************************************************************************************************************/

namespace std {
  template<>
  class numeric_limits<ChimeraTK::Boolean> : public numeric_limits<bool> {};
} // namespace std

/********************************************************************************************************************/
