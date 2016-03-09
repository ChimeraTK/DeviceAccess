/*
 * RegisterPath.h
 *
 *  Created on: Mar 1, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_PATH_H
#define MTCA4U_REGISTER_PATH_H

#include <string>

namespace mtca4u {

  /** Class to store a register path name. Elements of the path are separated by a "/" character, but a "."
   *  separation character can be used as well. Different equivalent notations will be converted into a
   *  standardised notation automatically. */
  class RegisterPath {
    public:
      RegisterPath() : path(separator) {}
      RegisterPath(const std::string &_path) : path(separator+_path) {removeExtraSeparators();}
      RegisterPath(const RegisterPath &_path) : path(_path.path) {removeExtraSeparators();}
      RegisterPath(const char *_path) : path(std::string(separator)+_path) {removeExtraSeparators();}

      /** type conversion operators into std::string */
      operator const std::string&() const { return path; }
      
      /** obtain path with alternative separator character "." instead of "/". The leading separator will be omitted */
      std::string getWithAltSeparator() {
        std::size_t pos;
        std::string path_alt = path.substr(1);
        while( (pos = path_alt.find(std::string(separator))) != std::string::npos ) {
          path_alt.replace(pos,1,separator_alt);
        }
        return path_alt;
      }

      /** /= operator: modify this object by adding a new element to this path */
      RegisterPath& operator/=(const std::string &rightHandSide) {
        path += separator+rightHandSide;
        removeExtraSeparators();
        return *this;
      }

      /** += operator: just concatenate-assign like normal strings */
      RegisterPath& operator+=(const std::string &rightHandSide) {
        path += rightHandSide;
        return *this;
      }

      /** < operator: comparison used for sorting e.g. in std::map */
      bool operator<(const RegisterPath &rightHandSide) const {
        return path < rightHandSide.path;
      }

      /** Post-decrement operator, e.g.: registerPath--
       *  Remove the last element from the path */
      RegisterPath& operator--(int) {
        std::size_t found = path.find_last_of(separator);
        if(found != std::string::npos && found > 0) {               // don't find the leading separator...
          path = path.substr(0,found);
        }
        else {
          path = separator;
        }
        return *this;
      }

      /** Pre-decrement operator, e.g.: --registerPath
       *  Remove the first element form the path */
      RegisterPath& operator--() {
        std::size_t found = path.find_first_of(separator,1);        // don't find the leading separator...
        if(found != std::string::npos) {
          path = separator + path.substr(found+1);
        }
        else {
          path = separator;
        }
        return *this;
      }

      /** comparison with other RegisterPath */
      bool operator==(const RegisterPath &rightHandSide) const {
        return *this == rightHandSide.path;
      }

      /** comparison with std::string */
      bool operator==(const std::string &rightHandSide) const {
        RegisterPath temp(rightHandSide);
        return path == temp.path;
      }

      /** comparison with char* */
      bool operator==(const char *rightHandSide) const {
        RegisterPath temp(rightHandSide);
        return path == temp.path;
      }

    protected:

      /** path in standardised notation */
      std::string path;
      
      /** separator character to separate the elements in the path */
      static const char separator[];
      
      /** altenative separator character */
      static const char separator_alt[];

      /** Search for duplicate separators (e.g. "//") and remove one of them. Also removes a trailing separator,
       *  if present. Occurrences of the alternative separator character are replaced with the primary separator. */
      void removeExtraSeparators() {
        std::size_t pos;
        while( (pos = path.find(std::string(separator_alt))) != std::string::npos ) {
          path.replace(pos,1,separator);
        }
        while( (pos = path.find(std::string(separator)+separator)) != std::string::npos ) {
          path.erase(pos,1);
        }
        if(path.length() > 1 && path.substr(path.length()-1,1) == separator) path.erase(path.length()-1,1);
      }
  };

  /** non-member / operator: add a new element to the path.
   *  Must be a non-member operator to allow implicit type conversions also on the leftHandSide. */
  RegisterPath operator/(const RegisterPath &leftHandSide, const RegisterPath &rightHandSide);

  /** non-member + operator for RegisterPath: concatenate with and like normal strings. */
  std::string operator+(const std::string &leftHandSide, const RegisterPath &rightHandSide);
  std::string operator+(const RegisterPath &leftHandSide, const std::string &rightHandSide);

} /* namespace mtca4u */

#endif /* MTCA4U_REGISTER_PATH_H */
