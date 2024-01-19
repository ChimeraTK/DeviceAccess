// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorAbstractor.h"
#include <type_traits>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Accessor class to read and write scalar registers transparently by using the accessor object like a variable of the
   * type UserType. Conversion to and from the UserType will be handled by a data converter matching the register
   * description in the map, if required. Obtain the accessor using the Device::getScalarRegisterAccessor() function.
   *
   * Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   * functions before reading from resp. after writing to the buffer using the operators.
   *
   * The second template argument TAG is only used to implement the template specialisation for ChimeraTK::Boolean
   * without duplicating all code.
   */
  template<typename UserType, typename TAG = std::nullptr_t>
  class ScalarRegisterAccessor : public NDRegisterAccessorAbstractor<UserType> {
   public:
    /**
     * Create instance from pointer to implementation.
     *
     * @attention Do not normally use directly. Users should call Device::getScalarRegisterAccessor() to
     * obtain an instance instead.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    ScalarRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl);

    /**
     * Placeholder constructor, to allow late initialisation of the accessor, e.g. in the open function.
     *
     * @attention Accessors created with this constructors will be dysfunctional, calling any member function will
     * throw an exception (by the boost::shared_ptr)!
     */
    ScalarRegisterAccessor();

    /** Implicit type conversion to UserType& to access the value as a reference. */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator UserType&() { return get()->accessData(0, 0); }

    /** Implicit type conversion to const UserType& to access the const reference. */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator const UserType&() const { return get()->accessData(0, 0); }

    /**
     * Allow free conversion between different TAG template arguments (which is merely used to avoid code duplication
     * in the template specialisation for ChimeraTK::Boolean).
     */
    template<typename OTHER_TAG>
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator ScalarRegisterAccessor<UserType, OTHER_TAG>&();

    /**
     * Allow free conversion between different TAG template arguments (which is merely used to avoid code duplication
     * in the template specialisation for ChimeraTK::Boolean).
     */
    template<typename OTHER_TAG>
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator const ScalarRegisterAccessor<UserType, OTHER_TAG>&() const {
      return *this;
    }

    /** Assignment operator, assigns the first element. */
    ScalarRegisterAccessor& operator=(UserType rightHandSide);

    /** Pre-increment operator for the first element. */
    ScalarRegisterAccessor& operator++() { return operator=(get()->accessData(0, 0) + 1); }

    /** Pre-decrement operator for the first element. */
    ScalarRegisterAccessor& operator--() { return operator=(get()->accessData(0, 0) - 1); }

    /** Post-increment operator for the first element. */
    UserType operator++(int);

    /** Post-decrement operator for the first element. */
    UserType operator--(int);

    /**
     * Get the cooked values in case the accessor is a raw accessor (which does
     * not do data conversion). This returns the converted data from the user
     * buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked();

    /**
     * Set the cooked values in case the accessor is a raw accessor (which does
     * not do data conversion). This converts to raw and writes the data to the
     * user buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    void setAsCooked(COOKED_TYPE value);

    /**
     * Convenience function to set and write new value if it differes from the current value. The given version number
     * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
     * the write actually takes place.
     */
    void writeIfDifferent(UserType newValue, VersionNumber versionNumber = VersionNumber{nullptr},
        DataValidity validity = DataValidity::ok);

    /**
     * Convenience function to set and write new value. The given version number.
     *  If versionNumber == {}, a new version number is generated.
     */
    void setAndWrite(UserType newValue, VersionNumber versionNumber = {});

    /**
     * Convenience function to read and return a value of UserType.
     */
    UserType readAndGet();

    friend class TransferGroup;

    using NDRegisterAccessorAbstractor<UserType>::get;
  };

  /********************************************************************************************************************/

  // Template specialisation for string. It does not have the ++ and -- operators
  template<>
  class ScalarRegisterAccessor<std::string> : public NDRegisterAccessorAbstractor<std::string> {
   public:
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    ScalarRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<std::string>> impl);

    ScalarRegisterAccessor() = default;

    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator std::string&();

    ScalarRegisterAccessor<std::string>& operator=(std::string rightHandSide);

    /**
     * Convenience function to set and write new value if it differes from the current value. The given version number
     * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
     * the write actually takes place.
     */
    void writeIfDifferent(const std::string& newValue, VersionNumber versionNumber = VersionNumber{nullptr},
        DataValidity dataValidity = DataValidity::ok);

    /**
     * Convenience function to set and write new value. The given version number.
     *  If versionNumber == {}, a new version number is generated.
     */
    void setAndWrite(const std::string& newValue, VersionNumber versionNumber = {});

    /**
     * Convenience function to read and return a value of UserType.
     */
    std::string readAndGet();

    friend class TransferGroup;
  };

  /********************************************************************************************************************/

  /**
   * Template specialisation for ChimeraTK::Boolean. Simply add implicit type conversions to references of bool, since
   * C++ does not do this conversion on its own as it involves two steps.
   */
  template<>
  class ScalarRegisterAccessor<ChimeraTK::Boolean, std::nullptr_t>
  : public ScalarRegisterAccessor<ChimeraTK::Boolean, void> {
   public:
    using ScalarRegisterAccessor<ChimeraTK::Boolean, void>::ScalarRegisterAccessor;
    using ScalarRegisterAccessor<ChimeraTK::Boolean, void>::operator=;

    /**
     * Implicit type conversion to const bool&.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator const bool&() const { return get()->accessData(0, 0); }

    /**
     * Implicit type conversion to bool&.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator bool&() { return get()->accessData(0, 0); }
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  ScalarRegisterAccessor<UserType, TAG>::ScalarRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl)
  : NDRegisterAccessorAbstractor<UserType>(impl) {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create ScalarRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  ScalarRegisterAccessor<UserType, TAG>::ScalarRegisterAccessor() {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create ScalarRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  template<typename OTHER_TAG>
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  ScalarRegisterAccessor<UserType, TAG>::operator ScalarRegisterAccessor<UserType, OTHER_TAG>&() {
    return *this;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  ScalarRegisterAccessor<UserType, TAG>& ScalarRegisterAccessor<UserType, TAG>::operator=(UserType rightHandSide) {
    get()->accessData(0, 0) = rightHandSide;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  UserType ScalarRegisterAccessor<UserType, TAG>::operator++(int) {
    UserType v = get()->accessData(0, 0);
    operator=(v + 1);
    return v;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  UserType ScalarRegisterAccessor<UserType, TAG>::operator--(int) {
    UserType v = get()->accessData(0, 0);
    operator=(v - 1);
    return v;
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  template<typename COOKED_TYPE>
  COOKED_TYPE ScalarRegisterAccessor<UserType, TAG>::getAsCooked() {
    return get()->template getAsCooked<COOKED_TYPE>(0, 0);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  template<typename COOKED_TYPE>
  void ScalarRegisterAccessor<UserType, TAG>::setAsCooked(COOKED_TYPE value) {
    return get()->template setAsCooked<COOKED_TYPE>(0, 0, value);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  void ScalarRegisterAccessor<UserType, TAG>::writeIfDifferent(
      UserType newValue, VersionNumber versionNumber, DataValidity validity) {
    if(get()->accessData(0, 0) != newValue || this->getVersionNumber() == VersionNumber(nullptr) ||
        this->dataValidity() != validity) {
      operator=(newValue);
      if(versionNumber == VersionNumber{nullptr}) {
        versionNumber = {};
      }
      this->setDataValidity(validity);
      this->write(versionNumber);
    }
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  void ScalarRegisterAccessor<UserType, TAG>::setAndWrite(UserType newValue, VersionNumber versionNumber) {
    operator=(newValue);
    this->write(versionNumber);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TAG>
  UserType ScalarRegisterAccessor<UserType, TAG>::readAndGet() {
    this->read();
    return get()->accessData(0, 0);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  inline ScalarRegisterAccessor<std::string>::ScalarRegisterAccessor(
      boost::shared_ptr<NDRegisterAccessor<std::string>> impl)
  : NDRegisterAccessorAbstractor<std::string>(std::move(impl)) {}

  /********************************************************************************************************************/

  inline ScalarRegisterAccessor<std::string>::operator std::string&() {
    return boost::static_pointer_cast<NDRegisterAccessor<std::string>>(_impl)->accessData(0, 0);
  }

  /********************************************************************************************************************/

  inline ScalarRegisterAccessor<std::string>& ScalarRegisterAccessor<std::string>::operator=(
      std::string rightHandSide) {
    boost::static_pointer_cast<NDRegisterAccessor<std::string>>(_impl)->accessData(0, 0) = std::move(rightHandSide);
    return *this;
  }

  /********************************************************************************************************************/

  inline void ScalarRegisterAccessor<std::string>::writeIfDifferent(
      const std::string& newValue, VersionNumber versionNumber, DataValidity validity) {
    if(get()->accessData(0, 0) != newValue || this->getVersionNumber() == VersionNumber(nullptr) ||
        this->dataValidity() != validity) {
      operator=(newValue);
      if(versionNumber == VersionNumber{nullptr}) versionNumber = {};
      this->setDataValidity(validity);
      this->write(versionNumber);
    }
  }

  /********************************************************************************************************************/

  inline void ScalarRegisterAccessor<std::string>::setAndWrite(
      const std::string& newValue, VersionNumber versionNumber) {
    operator=(newValue);
    this->write(versionNumber);
  }

  /********************************************************************************************************************/

  inline std::string ScalarRegisterAccessor<std::string>::readAndGet() {
    this->read();
    return get()->accessData(0, 0);
  }

  /********************************************************************************************************************/
  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK
