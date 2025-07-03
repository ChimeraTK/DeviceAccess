// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Base class for the register accessor abstractors (ScalarRegisterAccessor, OneDRegisterAccessor and
   * TwoDRegisterAccessor). Provides a private implementation of the TransferElement interface to allow the bridges to
   * be added to a TransferGroup. Also stores the shared pointer to the NDRegisterAccessor implementation.
   */
  template<user_type UserType>
  class NDRegisterAccessorAbstractor : public TransferElementAbstractor {
   public:
    /**
     * Create an uninitialised abstractor - just for late initialisation
     */
    NDRegisterAccessorAbstractor() = default;

    /**
     * Declare that we want the default copy constructor although we delete the assigmnent operator.
     */
    NDRegisterAccessorAbstractor(const NDRegisterAccessorAbstractor&) = default;

    /**
     * Assign a new accessor to this NDRegisterAccessorAbstractor. Since another  NDRegisterAccessorAbstractor is passed
     * as argument, both NDRegisterAccessorAbstractors will then point to the same accessor and thus are sharing the
     * same buffer. To obtain a new copy of the accessor with a distinct buffer, the corresponding
     * getXXRegisterAccessor() function of Device must be called.
     */
    void replace(const NDRegisterAccessorAbstractor<UserType>& newAccessor);

    /**
     * Alternative signature of replace() with the same functionality, used when a pointer to the implementation has
     * been obtained directly (instead of a NDRegisterAccessorAbstractor).
     */
    void replace(boost::shared_ptr<NDRegisterAccessor<UserType>> newImpl);

    /**
     * Prevent copying by operator=, since it will be confusing (operator= may also be overloaded to access the
     * content of the buffer!)
     */
    const NDRegisterAccessorAbstractor& operator=(const NDRegisterAccessorAbstractor& rightHandSide) const = delete;

    boost::shared_ptr<NDRegisterAccessor<UserType>> getImpl();

    using value_type = UserType;

   protected:
    explicit NDRegisterAccessorAbstractor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl);

    /*
     * Obtain the plain pointer to the implementation. Use the pointer carefully only inside this class, since it is not
     * a shared pointer!
     */
    NDRegisterAccessor<UserType>* get();
    const NDRegisterAccessor<UserType>* get() const;
  };

  /********************************************************************************************************************/

  template<user_type UserType>
  void NDRegisterAccessorAbstractor<UserType>::replace(const NDRegisterAccessorAbstractor<UserType>& newAccessor) {
    _impl = newAccessor._impl;
  }
  /********************************************************************************************************************/

  template<user_type UserType>
  void NDRegisterAccessorAbstractor<UserType>::replace(boost::shared_ptr<NDRegisterAccessor<UserType>> newImpl) {
    _impl = boost::static_pointer_cast<TransferElement>(newImpl);
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  NDRegisterAccessor<UserType>* NDRegisterAccessorAbstractor<UserType>::get() {
    return static_cast<NDRegisterAccessor<UserType>*>(TransferElementAbstractor::_impl.get());
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  const NDRegisterAccessor<UserType>* NDRegisterAccessorAbstractor<UserType>::get() const {
    return static_cast<NDRegisterAccessor<UserType>*>(TransferElementAbstractor::_impl.get());
  }

  /********************************************************************************************************************/

  template<user_type UserType>
  NDRegisterAccessorAbstractor<UserType>::NDRegisterAccessorAbstractor(
      boost::shared_ptr<NDRegisterAccessor<UserType>> impl)
  : TransferElementAbstractor(impl) {}

  /********************************************************************************************************************/

  template<user_type UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> NDRegisterAccessorAbstractor<UserType>::getImpl() {
    return boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(_impl);
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  /**
   * Concept requiring a type to be an NDRegisterAccessorAbstractor-derived type
   */
  template<typename T>
  concept accessor_abstractor = requires {
    typename T::value_type;
    requires std::is_base_of<NDRegisterAccessorAbstractor<typename T::value_type>, T>::value;
    requires user_type<typename T::value_type>;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK
