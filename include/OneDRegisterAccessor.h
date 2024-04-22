// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Accessor class to read and write registers transparently by using the accessor object like a vector of the type
   * UserType. Conversion to and from the UserType will be handled by a data converter matching the register description
   * in the map (if applicable). Obtain the accessor using the Device::getBufferingRegisterAccessor() function.
   *
   * Note: Transfers between the device and the internal buffer need to be triggered using the read() and write()
   * functions before reading from resp. after writing to the buffer using the operators.
   */
  template<typename UserType>
  class OneDRegisterAccessor : public NDRegisterAccessorAbstractor<UserType> {
   public:
    /**
     * Create accessor from pointer to implementation.
     *
     * @attention Do not normally use directly.  Users should call Device::getOneDRegisterAccessor() to obtain an
     * instance instead.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    OneDRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl);

    /**
     * Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
     *
     * @attention Accessors created with this constructors will be dysfunctional, calling any member function will throw
     * an exception (by the boost::shared_ptr)!
     */
    OneDRegisterAccessor();

    /**
     * Get or set buffer content by [] operator.
     *
     * @attention No bounds checking is performed, use getNumberOfElements() to obtain the number of elements in the
     * register.
     * Note: Using the iterators is slightly more efficient than using this operator!
     */
    UserType& operator[](unsigned int element) { return get()->accessData(0, element); }

    /**
     * Return number of elements/samples in the register
     */
    unsigned int getNElements() { return get()->getNumberOfSamples(); }

    /* Access data with std::vector-like iterators */
    using iterator = typename std::vector<UserType>::iterator;
    using const_iterator = typename std::vector<UserType>::const_iterator;
    using reverse_iterator = typename std::vector<UserType>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<UserType>::const_reverse_iterator;
    iterator begin() { return get()->accessChannel(0).begin(); }
    const_iterator begin() const { return get()->accessChannel(0).cbegin(); }
    const_iterator cbegin() const { return get()->accessChannel(0).cbegin(); }
    iterator end() { return get()->accessChannel(0).end(); }
    const_iterator end() const { return get()->accessChannel(0).cend(); }
    const_iterator cend() const { return get()->accessChannel(0).cend(); }
    reverse_iterator rbegin() { return get()->accessChannel(0).rbegin(); }
    const_reverse_iterator rbegin() const { return get()->accessChannel(0).crbegin(); }
    const_reverse_iterator crbegin() const { return get()->accessChannel(0).crbegin(); }
    reverse_iterator rend() { return get()->accessChannel(0).rend(); }
    const_reverse_iterator rend() const { return get()->accessChannel(0).crend(); }
    const_reverse_iterator crend() const { return get()->accessChannel(0).crend(); }

    /**
     * Swap content of (cooked) buffer with std::vector
     */
    void swap(std::vector<UserType>& x) noexcept;

    /**
     * Copy content of (cooked) buffer from std::vector
     */
    OneDRegisterAccessor<UserType>& operator=(const std::vector<UserType>& x);

    /**
     * Convert content of (cooked) buffer into std::vector
     *
     * Implicit conversion is allowed so one can pass a OneDRegisterAccessor directly when a std::vector is expected.
     */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    operator const std::vector<UserType>&() { return get()->accessChannel(0); }

    /**
     * Return a direct pointer to the memory buffer storng the elements.
     *
     * @attention Note that this pointer will be invalidated during read(), write() and swap(). If this accessor is part
     * of a TransferGroup, any call to one of these functions on any element of the TransferGroup or the TransferGroup
     * itself may invalidate the pointer!
     */
    UserType* data() { return get()->accessChannel(0).data(); }

    /**
     * Get the cooked values in case the accessor is a raw accessor (which does not do data conversion). This returns
     * the converted data from the use buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked(unsigned int sample);

    /**
     * Set the cooked values in case the accessor is a raw accessor (which does not do data conversion). This converts
     * to raw and writes the data to the user buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    void setAsCooked(unsigned int sample, COOKED_TYPE value);

    /**
     * Convenience function to set and write new value if it differes from the current value. The given version number
     * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
     * the write actually takes place.
     */
    void writeIfDifferent(const std::vector<UserType>& newValue, VersionNumber versionNumber = VersionNumber{nullptr},
        DataValidity validity = DataValidity::ok);

    friend class TransferGroup;

    using NDRegisterAccessorAbstractor<UserType>::get;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename UserType>
  OneDRegisterAccessor<UserType>::OneDRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl)
  : NDRegisterAccessorAbstractor<UserType>(impl) {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create OneDRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");

    if(get()->getNumberOfChannels() != 1) {
      throw ChimeraTK::logic_error(std::string("The OneDRegisterAccessor has a too low ") +
          "dimension to access the register " + impl->getName());
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  OneDRegisterAccessor<UserType>::OneDRegisterAccessor() {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create OneDRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
  }

  /********************************************************************************************************************/

  /**
   * Swap content of (cooked) buffer with std::vector
   */
  template<typename UserType>
  void OneDRegisterAccessor<UserType>::swap(std::vector<UserType>& x) noexcept {
    if(x.size() != get()->accessChannel(0).size()) {
      // Do not throw in swap() and rather terminated. See CPP core guidlines C.85
      // https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c85-make-swap-noexcept
      std::cerr << "Swapping with a buffer of a different size is not allowed." << std::endl;
      std::exit(1);
    }
    get()->accessChannel(0).swap(x);
  }

  /********************************************************************************************************************/

  /**
   * Copy content of (cooked) buffer from std::vector
   */
  template<typename UserType>
  OneDRegisterAccessor<UserType>& OneDRegisterAccessor<UserType>::operator=(const std::vector<UserType>& x) {
    if(x.size() != get()->accessChannel(0).size()) {
      throw ChimeraTK::logic_error("Copying in a buffer of a different size is not allowed.");
    }
    get()->accessChannel(0) = x;
    return *this;
  }

  /********************************************************************************************************************/

  /**
   * Get the cooked values in case the accessor is a raw accessor (which does not do data conversion). This returns
   * the converted data from the use buffer. It does not do any read or write transfer.
   */
  template<typename UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE OneDRegisterAccessor<UserType>::getAsCooked(unsigned int sample) {
    return get()->template getAsCooked<COOKED_TYPE>(0, sample);
  }

  /********************************************************************************************************************/

  /**
   * Set the cooked values in case the accessor is a raw accessor (which does not do data conversion). This converts
   * to raw and writes the data to the user buffer. It does not do any read or write transfer.
   */
  template<typename UserType>
  template<typename COOKED_TYPE>
  void OneDRegisterAccessor<UserType>::setAsCooked(unsigned int sample, COOKED_TYPE value) {
    return get()->template setAsCooked<COOKED_TYPE>(0, sample, value);
  }

  /********************************************************************************************************************/

  /**
   * Convenience function to set and write new value if it differes from the current value. The given version number
   * is only used in case the value differs. If versionNumber == {nullptr}, a new version number is generated only if
   * the write actually takes place.
   */
  template<typename UserType>
  void OneDRegisterAccessor<UserType>::writeIfDifferent(
      const std::vector<UserType>& newValue, VersionNumber versionNumber, DataValidity validity) {
    if(!std::equal(newValue.begin(), newValue.end(), get()->accessChannel(0).begin()) ||
        this->getVersionNumber() == VersionNumber(nullptr) || this->dataValidity() != validity) {
      operator=(newValue);
      if(versionNumber == VersionNumber{nullptr}) versionNumber = {};
      this->setDataValidity(validity);
      this->write(versionNumber);
    }
  }

  /********************************************************************************************************************/

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK
