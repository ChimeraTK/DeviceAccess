#ifndef CHIMERA_TK_VERSION_NUMBER_H
#define CHIMERA_TK_VERSION_NUMBER_H

#include <atomic>
#include <cstdint>

namespace ChimeraTK {

  /**
   * Class for generating and holding version numbers without exposing a numeric representation
   *
   * Version numbers are used to resolve competing updates that are applied to the same process variable. For example,
   * it they can help in breaking an infinite update loop that might occur when two process variables are related and
   * update each other.
   * 
   * They are also used to determine the order of updates made to different process variables, e.g. to make sure that
   * TransferElement::readAny() always returns the oldest change first.
   * 
   *  @todo add unit test!
   */
  class VersionNumber {
    
    public:
      
      /** Default constructor: Generate new unique version number */
      VersionNumber() : _value(nextVersionNumber()) {}

      /** Copy constructor */
      VersionNumber(const VersionNumber &other) : _value(other._value) {}

      /** Copy the full state of another VersionNumber object. */
      VersionNumber& operator=(const VersionNumber &other) {
        _value = other._value;
        return *this;
      }
      
      /** Comparison operators */
      bool operator==(const VersionNumber &other) {
        return _value == other._value;
      }
      bool operator!=(const VersionNumber &other) {
        return _value != other._value;
      }
      bool operator>(const VersionNumber &other) {
        return _value > other._value;
      }
      bool operator<(const VersionNumber &other) {
        return _value < other._value;
      }
      bool operator>=(const VersionNumber &other) {
        return _value >= other._value;
      }
      bool operator<=(const VersionNumber &other) {
        return _value <= other._value;
      }
    
    private:
    
      /**
       * The version number held by this instance
       */
      std::uint64_t  _value;

      /**
      * Returns the next version number. The next version number is determined in an atomic way, so that it is
      * guaranteed that this method never returns the same version number twice (unless the counter overflows, which
      * is very unlikely). The first version number returned by this method is one. The version number that is
      * returned is guaranteed to be greater than the version numbers returned for earlier calls to this method. This
      * method may safely be called by any thread without any synchronization.
      */
      static uint64_t nextVersionNumber() {
        return ++_lastGeneratedVersionNumber;
      }

      /**
      * Global static atomic: Last version number that was generated.
      */
      static std::atomic<uint64_t> _lastGeneratedVersionNumber;

  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_VERSION_NUMBER_H
