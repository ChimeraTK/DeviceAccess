// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <string>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Class for generating and holding version numbers without exposing a numeric representation
   *
   * Version numbers are used to resolve competing updates that are applied to the same process variable. For example,
   * it they can help in breaking an infinite update loop that might occur when two process variables are related and
   * update each other.
   *
   * They are also used to determine the order of updates made to different process variables.
   */
  class VersionNumber {
   public:
    /**
     * Default constructor: Generate new unique version number with current time as time stamp
     */
    VersionNumber() : _value(nextVersionNumber()), _time(std::chrono::system_clock::now()) {}

    /** Copy constructor */
    VersionNumber(const VersionNumber& other) = default;

    /** Copy the full state of another VersionNumber object. */
    VersionNumber& operator=(const VersionNumber& other) = default;

    /** Generate new unique version number with a given time stamp. */
    explicit VersionNumber(std::chrono::system_clock::time_point timestamp);

    /**
     * Create null version number, which is guaranteed to be smaller than all version numbers generated with the
     * default constructor. This should be used to initialse version numbers which are never actually used for data
     * transfers (e.g. at application start). The argument is a dummy argument to distinguish the contructor
     * signature.
     */
    explicit VersionNumber(std::nullptr_t) : _value(0) {}

    /** Return the time stamp associated with this version number */
    [[nodiscard]] std::chrono::time_point<std::chrono::system_clock> getTime() const { return _time; }

    /**
     * Comparison operators.
     *
     * Compare version number only, since they are ordered in time and atomically generated, so the result is logically
     * as expected. The time stamp is not precise and not atomically generated, so comparing it would not be precise.
     */
    bool operator==(const VersionNumber& other) const { return _value == other._value; }
    bool operator!=(const VersionNumber& other) const { return _value != other._value; }
    bool operator>(const VersionNumber& other) const { return _value > other._value; }
    bool operator<(const VersionNumber& other) const { return _value < other._value; }
    bool operator>=(const VersionNumber& other) const { return _value >= other._value; }
    bool operator<=(const VersionNumber& other) const { return _value <= other._value; }

    /**
     * Conversion into a human readable std::string to allow e.g. printing the version number on screen. Do not try to
     * parse the string in any way, the exact format is unspecified.
     */
    explicit operator std::string() const;

   private:
    /**
     * The version number held by this instance
     */
    std::uint64_t _value;

    /**
     * The time stamp held by this instance
     */
    std::chrono::time_point<std::chrono::system_clock> _time;

    /**
     * Returns the next version number. The next version number is determined in an atomic way, so that it is
     * guaranteed that this method never returns the same version number twice (unless the counter overflows, which is
     * very unlikely). The first version number returned by this method is one. The version number that is returned is
     * guaranteed to be greater than the version numbers returned for earlier calls to this method. This method may
     * safely be called by any thread without any synchronization.
     */
    static uint64_t nextVersionNumber() { return ++_lastGeneratedVersionNumber; }

    /**
     * Global static atomic: Last version number that was generated.
     */
    static std::atomic<uint64_t> _lastGeneratedVersionNumber;

    friend std::ostream& operator<<(std::ostream& stream, const VersionNumber& version);

    template<class T, class CharT>
    friend struct std::formatter;
  };

  /********************************************************************************************************************/

  inline VersionNumber::VersionNumber(std::chrono::system_clock::time_point timestamp)
  : _value(nextVersionNumber()), _time(timestamp) {}

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /** Stream operator passing the human readable representation to an ostream. */
  std::ostream& operator<<(std::ostream& stream, const VersionNumber& version);

  /********************************************************************************************************************/

} // namespace ChimeraTK

/**********************************************************************************************************************/

/** Formatter for C++20 std::format etc. */
template<class CharT>
struct std::formatter<ChimeraTK::VersionNumber, CharT> {
  bool printVersion{false};
  bool printTime{false};

  template<class ParseContext>
  constexpr ParseContext::iterator parse(ParseContext& ctx) {
    auto it = ctx.begin();
    if(it == ctx.end()) {
      return it;
    }
    if(*it == 'v') {
      printVersion = true;
      ++it;
    }
    if(*it == 't') {
      printTime = true;
      ++it;
    }
    if(it != ctx.end() && *it != '}') {
      throw std::format_error("Invalid format args for ChimeraTK::VersionNumber.");
    }
    if(!printVersion && !printTime) {
      // print version by default
      printVersion = true;
    }
    return it;
  }

  template<typename FormatContext>
  auto format(const ChimeraTK::VersionNumber& v, FormatContext& ctx) const {
    std::string fmt;
    if(printVersion) {
      fmt = "v{0}";
    }
    if(printTime) {
      fmt += "@{1}";
    }
    return std::vformat_to(ctx.out(), fmt, std::make_format_args(v._value, v._time));
  }
};

/**********************************************************************************************************************/
