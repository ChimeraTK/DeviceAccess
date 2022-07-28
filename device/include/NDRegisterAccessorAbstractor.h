// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CopyRegisterDecorator.h"
#include "ForwardDeclarations.h"
#include "NDRegisterAccessor.h"
#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /** Base class for the register accessor abstractors (ScalarRegisterAccessor,
   * OneDRegisterAccessor and TwoDRegisterAccessor). Provides a private
   * implementation of the TransferElement interface to allow the bridges to be
   * added to a TransferGroup. Also stores the shared pointer to the
   * NDRegisterAccessor implementation. */
  template<typename UserType>
  class NDRegisterAccessorAbstractor : public TransferElementAbstractor {
   public:
    /** Create an uninitialised abstractor - just for late initialisation */
    NDRegisterAccessorAbstractor() {}

    /** Assign a new accessor to this NDRegisterAccessorAbstractor. Since another
     * NDRegisterAccessorAbstractor is passed as argument, both
     * NDRegisterAccessorAbstractors will then point to the same accessor and thus
     * are sharing the same buffer. To obtain a new copy of the accessor with a
     * distinct buffer, the corresponding getXXRegisterAccessor() function of
     * Device must be called. */
    void replace(const NDRegisterAccessorAbstractor<UserType>& newAccessor) { _impl = newAccessor._impl; }

    /** Alternative signature of replace() with the same functionality, used when a
     * pointer to the implementation has been obtained directly (instead of a
     * NDRegisterAccessorAbstractor). */
    void replace(boost::shared_ptr<NDRegisterAccessor<UserType>> newImpl) {
      _impl = boost::static_pointer_cast<TransferElement>(newImpl);
    }

    /** prevent copying by operator=, since it will be confusing (operator= may
     * also be overloaded to access the content of the buffer!) */
    const NDRegisterAccessorAbstractor& operator=(const NDRegisterAccessorAbstractor& rightHandSide) const = delete;

   protected:
    /** Obtain the plain pointer to the implementation. Use the pointer carefully
     * only inside this class, since it is not a shared pointer! */
    NDRegisterAccessor<UserType>* get() {
      return static_cast<NDRegisterAccessor<UserType>*>(TransferElementAbstractor::_impl.get());
    }
    const NDRegisterAccessor<UserType>* get() const {
      return static_cast<NDRegisterAccessor<UserType>*>(TransferElementAbstractor::_impl.get());
    }

    NDRegisterAccessorAbstractor(boost::shared_ptr<NDRegisterAccessor<UserType>> impl)
    : TransferElementAbstractor(impl) {}
  };

  // Do not declare the template for all user types as extern here.
  // This could avoid optimisation of the inline code.

} // namespace ChimeraTK
