/*
 * ScalarRegisterAccessor.h
 *
 *  Created on: Mar 23, 2016
 *      Author: Martin Hierholzer <martin.hierholzer@desy.de>
 */

#ifndef CHIMERA_TK_SCALAR_REGISTER_ACCESSOR_H
#define CHIMERA_TK_SCALAR_REGISTER_ACCESSOR_H

#include "NDRegisterAccessorAbstractor.h"
#include <type_traits>

namespace ChimeraTK {

  /*********************************************************************************************************************/
  /** Accessor class to read and write scalar registers transparently by using the
   * accessor object like a variable of the type UserType. Conversion to and from
   * the UserType will be handled by a data converter matching the register
   * description in the map, if required. Obtain the accessor using the
   * Device::getScalarRegisterAccessor() function.
   *
   *  Note: Transfers between the device and the internal buffer need to be
   * triggered using the read() and write() functions before reading from resp.
   * after writing to the buffer using the operators.
   */
  template<typename UserType>
  class ScalarRegisterAccessor : public NDRegisterAccessorAbstractor<UserType> {
   public:
    /** Constructor. @attention Do not normally use directly.
     *  Users should call Device::getScalarRegisterAccessor() to obtain an
     * instance instead. */
    ScalarRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl)
    : NDRegisterAccessorAbstractor<UserType>(impl) {
      static_assert(!std::is_same<UserType, Void>::value,
          "You cannot create ScalarRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
    }

    /** Placeholder constructor, to allow late initialisation of the accessor,
     * e.g. in the open function.
     *  @attention Accessors created with this constructors will be dysfunctional,
     * calling any member function will throw an exception (by the
     * boost::shared_ptr)! */
    ScalarRegisterAccessor() {
      static_assert(!std::is_same<UserType, Void>::value,
          "You cannot create ScalarRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
    }

    /** Implicit type conversion to user type T to access the value as a
     * reference. */
    operator UserType&() { return get()->accessData(0, 0); }

    /** Implicit type conversion to user type T to access the const value. */
    operator const UserType&() const { return get()->accessData(0, 0); }

    /** Assignment operator, assigns the first element. */
    ScalarRegisterAccessor<UserType>& operator=(UserType rightHandSide) {
      get()->accessData(0, 0) = rightHandSide;
      return *this;
    }

    /** Pre-increment operator for the first element. */
    ScalarRegisterAccessor<UserType>& operator++() { return operator=(get()->accessData(0, 0) + 1); }

    /** Pre-decrement operator for the first element. */
    ScalarRegisterAccessor<UserType>& operator--() { return operator=(get()->accessData(0, 0) - 1); }

    /** Post-increment operator for the first element. */
    UserType operator++(int) {
      UserType v = get()->accessData(0, 0);
      operator=(v + 1);
      return v;
    }

    /** Post-decrement operator for the first element. */
    UserType operator--(int) {
      UserType v = get()->accessData(0, 0);
      operator=(v - 1);
      return v;
    }

    /** Get the cooked values in case the accessor is a raw accessor (which does
     * not do data conversion). This returns the converted data from the user
     * buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked() {
      return get()->template getAsCooked<COOKED_TYPE>(0, 0);
    }

    /** Set the cooked values in case the accessor is a raw accessor (which does
     * not do data conversion). This converts to raw and writes the data to the
     * user buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    void setAsCooked(COOKED_TYPE value) {
      return get()->template setAsCooked<COOKED_TYPE>(0, 0, value);
    }

    /**
     * Convenience function to set and write new value if it differes from the current value. The given version number
     * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
     * the write actually takes place.
     */
    void writeIfDifferent(UserType newValue, VersionNumber versionNumber = VersionNumber{nullptr}) {
      if(get()->accessData(0, 0) != newValue || this->getVersionNumber() == VersionNumber(nullptr)) {
        operator=(newValue);
        if(versionNumber == VersionNumber{nullptr}) versionNumber = {};
        this->write(versionNumber);
      }
    }

    friend class TransferGroup;

    using NDRegisterAccessorAbstractor<UserType>::get;
  };

  // Template specialisation for string. It does not have the ++ and -- operators
  template<>
  class ScalarRegisterAccessor<std::string> : public NDRegisterAccessorAbstractor<std::string> {
   public:
    inline ScalarRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<std::string>> impl)
    : NDRegisterAccessorAbstractor<std::string>(impl) {}

    inline ScalarRegisterAccessor() {}

    inline operator std::string&() {
      return boost::static_pointer_cast<NDRegisterAccessor<std::string>>(_impl)->accessData(0, 0);
    }

    inline ScalarRegisterAccessor<std::string>& operator=(std::string rightHandSide) {
      boost::static_pointer_cast<NDRegisterAccessor<std::string>>(_impl)->accessData(0, 0) = rightHandSide;
      return *this;
    }

    /**
     * Convenience function to set and write new value if it differes from the current value. The given version number
     * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
     * the write actually takes place.
     */
    void writeIfDifferent(const std::string& newValue, VersionNumber versionNumber = VersionNumber{nullptr}) {
      if(get()->accessData(0, 0) != newValue || this->getVersionNumber() == VersionNumber(nullptr)) {
        operator=(newValue);
        if(versionNumber == VersionNumber{nullptr}) versionNumber = {};
        this->write(versionNumber);
      }
    }

    friend class TransferGroup;
  };

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK

#endif /* CHIMERA_TK_SCALAR_REGISTER_ACCESSOR_H */
