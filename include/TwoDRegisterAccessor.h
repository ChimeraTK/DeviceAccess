// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorAbstractor.h"

#include <boost/smart_ptr.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Accessor class to read and write 2D registers.
   */
  template<user_type UserType>
  class TwoDRegisterAccessor : public NDRegisterAccessorAbstractor<UserType> {
   public:
    /** Do not use this constructor directly. Instead call Device::getTwoDRegisterAccessor(). */
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    TwoDRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> _accessor);

    /**
     * Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
     *
     * @attention Accessors created with this constructors will be dysfunctional,  calling any member function will
     * throw an exception (by the boost::shared_ptr)!
     */
    TwoDRegisterAccessor();

    /** Operator to access individual sequences/channels. */
    std::vector<UserType>& operator[](size_t channel) { return get()->accessChannel(channel); }

    /** Const operator to access individual sequences/channels. */
    const std::vector<UserType>& operator[](size_t channel) const { return get()->accessChannel(channel); }

    /** Assignment operator to assign the entire 2D array */
    TwoDRegisterAccessor<UserType>& operator=(const std::vector<std::vector<UserType>>& other);

    /** Return the number of channels (formerly called sequences) */
    [[nodiscard]] size_t getNChannels() const { return get()->getNumberOfChannels(); }

    /** Return number of elements/samples per channel */
    [[nodiscard]] size_t getNElementsPerChannel() const { return get()->getNumberOfSamples(); }

    /**
     * Get the cooked values in case the accessor is a raw accessor (which does not do data conversion). This returns
     * the converted data from the use buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked(unsigned int channel, unsigned int sample) const;

    /**
     * Set the cooked values in case the accessor is a raw accessor (which does not do data conversion). This converts
     * to raw and writes the data to the user buffer. It does not do any read or write transfer.
     */
    template<typename COOKED_TYPE>
    void setAsCooked(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    friend class TransferGroup;

    using NDRegisterAccessorAbstractor<UserType>::get;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<user_type UserType>
  TwoDRegisterAccessor<UserType>::TwoDRegisterAccessor(boost::shared_ptr<NDRegisterAccessor<UserType>> _accessor)
  : NDRegisterAccessorAbstractor<UserType>(_accessor) {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create TwoDRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  TwoDRegisterAccessor<UserType>::TwoDRegisterAccessor() {
    static_assert(!std::is_same<UserType, Void>::value,
        "You cannot create TwoDRegisterAccessor<ChimeraTK::Void>! Use VoidRegisterAccessor instead.");
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  TwoDRegisterAccessor<UserType>& TwoDRegisterAccessor<UserType>::operator=(
      const std::vector<std::vector<UserType>>& other) {
    get()->accessChannels() = other;
    return *this;
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  template<typename COOKED_TYPE>
  COOKED_TYPE TwoDRegisterAccessor<UserType>::getAsCooked(unsigned int channel, unsigned int sample) const {
    return get()->template getAsCooked<COOKED_TYPE>(channel, sample);
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  template<typename COOKED_TYPE>
  void TwoDRegisterAccessor<UserType>::setAsCooked(unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    return get()->template setAsCooked<COOKED_TYPE>(channel, sample, value);
  }

  /********************************************************************************************************************/

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK
