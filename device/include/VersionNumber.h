#ifndef CHIMERA_TK_VERSION_NUMBER_H
#define CHIMERA_TK_VERSION_NUMBER_H

#include <cstdint>
#include <stdexcept>

namespace ChimeraTK {

  /**
   *  Class holding a version number together with a flag whether this version number is valid.
   */
  class VersionNumber {
    
    public:
      
      typedef std::uint64_t UnderlyingDataType;
      
      /** Construct an invalid VersionNumber */
      VersionNumber() : _value(0), _isValid(false) {}
      
      /** Construct VersionNumber from a numeric value */
      VersionNumber(UnderlyingDataType value) : _value(value), _isValid(true) {}

      /** Copy constructor */
      VersionNumber(const VersionNumber &other) : _value(other._value), _isValid(other._isValid) {}

      /** Convert value of version number into uint64_t. If the version number is not valid, an std::invalid_argument
       *  exception will be thrown */
      operator UnderlyingDataType() const {
        if(!_isValid) {
          throw std::invalid_argument("This VersionNumber is invalid and thus cannot be converted to numeric data type.");
        }
        return _value;
      }
      
      /** Check whether the version number is valid */
      bool isValid() const {
        return _isValid;
      }

      /** Assign number to the version number. This will intrinsically flag the version number to be valid. */
      VersionNumber& operator=(UnderlyingDataType value) {
        _value = value;
        _isValid = true;
        return *this;
      }

      /** Copy the full state of another VersionNumber object. */
      VersionNumber& operator=(const VersionNumber &other) {
        _value = other._value;
        _isValid = other._isValid;
        return *this;
      }
    
    private:
    
      UnderlyingDataType _value;
      bool _isValid;
  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_VERSION_NUMBER_H
