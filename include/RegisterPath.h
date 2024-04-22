// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"

#include <string>
#include <vector>

namespace ChimeraTK {

  /** Class to store a register path name. Elements of the path are separated by a
   * "/" character, but an alternative separation character (e.g. ".") can
   * optionally be specified as well. Different equivalent notations will be
   *  converted into a standardised notation automatically. */
  class RegisterPath {
   public:
    RegisterPath() : path(separator) {}
    // Yes, we want implicit construction. Turn off the linter.
    // NOLINTNEXTLINE(hicpp-explicit-conversions, google-explicit-constructor)
    RegisterPath(const std::string& _path) : path(std::string(separator + _path)) { removeExtraSeparators(); }
    RegisterPath(const RegisterPath& _path) : path(_path.path), separator_alt(_path.separator_alt) {
      removeExtraSeparators();
    }
    // we can use the default assignment operator but have to declare this because we have an explicit copy constructor
    RegisterPath& operator=(const RegisterPath& _path) = default;
    // Yes, we want implicit construction. Turn off the linter.
    // NOLINTNEXTLINE(hicpp-explicit-conversions, google-explicit-constructor)
    RegisterPath(const char* _path) : path(std::string(separator) + _path) { removeExtraSeparators(); }

    /** type conversion operators into std::string */
    // This is an implicit conversion operator. Turn off the linter warning to make it explicit.
    // NOLINTNEXTLINE(hicpp-explicit-conversions, google-explicit-constructor)
    operator std::string() const { return getWithOtherSeparatorReplaced(separator_alt); }

    /** set alternative separator. */
    void setAltSeparator(const std::string& altSeparator) {
      if(altSeparator == separator) {
        separator_alt = "";
      }
      else {
        separator_alt = altSeparator;
      }
    }

    /** obtain path with alternative separator character instead of "/". The
     * leading separator will be omitted */
    [[nodiscard]] std::string getWithAltSeparator() const {
      if(separator_alt.length() == 0) return path.substr(1);
      std::size_t pos;
      std::string path_alt = path.substr(1);
      while((pos = path_alt.find(std::string(separator))) != std::string::npos) {
        path_alt.replace(pos, 1, separator_alt);
      }
      return removeExtraSeparators(path_alt, separator_alt);
    }

    /** /= operator: modify this object by adding a new element to this path */
    RegisterPath& operator/=(const std::string& rightHandSide) {
      path += std::string(separator) + rightHandSide;
      removeExtraSeparators();
      return *this;
    }

    /** += operator: just concatenate-assign like normal strings */
    RegisterPath& operator+=(const std::string& rightHandSide) {
      path += rightHandSide;
      removeExtraSeparators();
      return *this;
    }

    /** < operator: comparison used for sorting e.g.\ in std::map */
    bool operator<(const RegisterPath& rightHandSide) const {
      std::string sepalt = getCommonAltSeparator(rightHandSide);
      return getWithOtherSeparatorReplaced(sepalt) < rightHandSide.getWithOtherSeparatorReplaced(sepalt);
    }

    /** Cut-right operator, e.g.\ \c registerPath--
     *
     *  Remove the last element from the path and return the modified path.
     *  @attention In contrast to the \c xx-- operator on numeric types this is
     * NOT a post operator! It returns the already modified register path.
     */
    RegisterPath& operator--(int) {
      auto found = path.find_last_of(std::string(separator));
      if(separator_alt.length() != 0) {
        auto found_alt = path.find_last_of(std::string(separator_alt));
        if(found_alt != std::string::npos) found = std::max(found, found_alt);
      }
      if(found != std::string::npos && found > 0) { // don't find the leading separator...
        path = path.substr(0, found);
      }
      else {
        path = separator;
      }
      return *this;
    }

    /** Cut-left operator, e.g.\ \c --registerPath
     *
     *  Remove the first element form the path and return the modified path. */
    RegisterPath& operator--() {
      std::size_t found = path.find_first_of(std::string(separator), 1); // don't find the leading separator...
      if(separator_alt.length() != 0) {
        found = std::min(found, path.find_first_of(std::string(separator_alt)));
      }
      if(found != std::string::npos) {
        path = std::string(separator) + path.substr(found + 1);
      }
      else {
        path = separator;
      }
      return *this;
    }

    /** comparison with other RegisterPath */
    bool operator==(const RegisterPath& rightHandSide) const {
      std::string sepalt = getCommonAltSeparator(rightHandSide);
      return getWithOtherSeparatorReplaced(sepalt) == rightHandSide.getWithOtherSeparatorReplaced(sepalt);
    }

    /** comparison with std::string */
    bool operator==(const std::string& rightHandSide) const { return operator==(RegisterPath(rightHandSide)); }

    /** comparison with char* */
    bool operator==(const char* rightHandSide) const { return operator==(RegisterPath(rightHandSide)); }

    /** comparison with other RegisterPath */
    bool operator!=(const RegisterPath& rightHandSide) const { return !operator==(rightHandSide); }

    /** comparison with std::string */
    bool operator!=(const std::string& rightHandSide) const { return operator!=(RegisterPath(rightHandSide)); }

    /** comparison with char* */
    bool operator!=(const char* rightHandSide) const { return operator!=(RegisterPath(rightHandSide)); }

    /** return the length of the path (including leading slash) */
    [[nodiscard]] size_t length() const { return path.length(); }

    /** check if the register path starts with the given path */
    [[nodiscard]] bool startsWith(const RegisterPath& compare) const {
      std::string sepalt = getCommonAltSeparator(compare);
      std::string pathConverted(getWithOtherSeparatorReplaced(sepalt));
      std::string otherPathConverted(compare.getWithOtherSeparatorReplaced(sepalt));
      return pathConverted.substr(0, compare.length()) == otherPathConverted;
    }

    /** check if the register path ends with the given path component(s) */
    [[nodiscard]] bool endsWith(const RegisterPath& compare) const {
      std::string sepalt = getCommonAltSeparator(compare);
      std::string pathConverted(getWithOtherSeparatorReplaced(sepalt));
      std::string otherPathConverted(compare.getWithOtherSeparatorReplaced(sepalt));
      if(pathConverted.size() < compare.length()) return false;
      return pathConverted.substr(pathConverted.size() - compare.length()) == otherPathConverted;
    }

    /** split path into components */
    [[nodiscard]] std::vector<std::string> getComponents() const {
      std::vector<std::string> components;
      if(path.length() <= 1) return components;
      size_t pos = 0;
      while(pos != std::string::npos) {
        size_t npos = path.find_first_of(separator, pos + 1);
        if(separator_alt.length() != 0) {
          size_t npos_alt = path.find_first_of(separator_alt, pos + 1);
          if(npos_alt != std::string::npos && npos_alt < npos) npos = npos_alt;
        }
        components.push_back(path.substr(pos + 1, npos - pos - 1));
        pos = npos;
      }
      return components;
    }

    friend RegisterPath operator/(const RegisterPath& leftHandSide, const RegisterPath& rightHandSide);
    friend RegisterPath operator+(const RegisterPath& leftHandSide, const std::string& rightHandSide);
    friend RegisterPath operator/(const RegisterPath& leftHandSide, int rightHandSide);
    friend RegisterPath operator*(const RegisterPath& leftHandSide, int rightHandSide);
    friend std::ostream& operator<<(std::ostream& os, const RegisterPath& me);

   protected:
    /** path in standardised notation */
    std::string path;

    /** separator character to separate the elements in the path */
    static const char separator[];

    /** altenative separator character */
    std::string separator_alt;

    /** Search for duplicate separators (e.g. "//") and remove one of them. Also
     * removes a trailing separator, if present. */
    void removeExtraSeparators() { path = removeExtraSeparators(path); }

    /** Search for duplicate separators (e.g. "//") and remove one of them. Also
     * removes a trailing separator, if present. The second optional argument
     * allows to search for other separators instead of the default. */
    [[nodiscard]] static std::string removeExtraSeparators(std::string string, const std::string& sep = separator) {
      std::size_t pos;
      while((pos = string.find(std::string(sep) + sep)) != std::string::npos) {
        string.erase(pos, 1);
      }
      if(string.length() > 1 && string.substr(string.length() - 1, 1) == sep) string.erase(string.length() - 1, 1);
      return string;
    }

    /** Return the path after replacing the given otherSeparator with the standard
     * separator "/". */
    [[nodiscard]] std::string getWithOtherSeparatorReplaced(const std::string& otherSeparator) const {
      // no replacement needed, of other separator is empty
      if(otherSeparator.length() == 0) return path;
      // replace all occurrences of otherSeparator with separator
      std::string path_other = path;
      size_t pos = 0;
      while((pos = path_other.find(otherSeparator)) != std::string::npos) {
        path_other.replace(pos, 1, std::string(separator));
      }
      // replace duplicate separators and remove one of them. Also remove a
      // trailing separator, if present.
      return removeExtraSeparators(path_other);
    }

    /** return common alternative separator for this RegisterPath and the
     * specified other RegisterPath objects. */
    [[nodiscard]] std::string getCommonAltSeparator(const RegisterPath& otherPath) const {
      std::string sepalt = separator_alt;
      if(otherPath.separator_alt.length() > 0) {
        if(sepalt.length() == 0) {
          sepalt = otherPath.separator_alt;
        }
        else {
          if(sepalt != otherPath.separator_alt) {
            throw ChimeraTK::logic_error("RegisterPath objects do not compare "
                                         "when both have different alternative "
                                         "separators set.");
          }
        }
      }
      return sepalt;
    }
  };

  /** non-member / operator: add a new element to the path.
   *  Must be a non-member operator to allow implicit type conversions also on the
   * leftHandSide. */
  RegisterPath operator/(const RegisterPath& leftHandSide, const RegisterPath& rightHandSide);

  /** non-member + operator for RegisterPath: concatenate with normal strings. */
  std::string operator+(const std::string& leftHandSide, const RegisterPath& rightHandSide);
  RegisterPath operator+(const RegisterPath& leftHandSide, const std::string& rightHandSide);

  /** operators used to build numeric addresses from integers */
  RegisterPath operator/(const RegisterPath& leftHandSide, int rightHandSide);
  RegisterPath operator*(const RegisterPath& leftHandSide, int rightHandSide);

  /** streaming operator */
  std::ostream& operator<<(std::ostream& os, const RegisterPath& me);

} /* namespace ChimeraTK */
