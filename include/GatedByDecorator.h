// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ClosedGateQuery.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedBackendRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Transparent decorator carrying the gate query of a gated register. NumericAddressedBackend::getSyncRegisterAccessor
   * puts it on the synchronous accessor of a gated register, so it is the outermost object and the accessor the
   * asynchronous distributor polls. It delegates the read and forwards the value, the validity and the version number
   * of its target unchanged; it is not an enforcement point (the gate is enforced where the consumer's raw data is
   * converted, see the low-level element).
   *
   * The decorator resolves the base accessor's read registration once at construction (a dynamic_cast over the
   * getInternalElements() chain, not on any read path) and reads the consumer's gate result from it. A transfer-group
   * merge re-seats the registration in place when it replaces the low-level element, so the reference stays valid while
   * the accessor lives.
   */
  template<typename UserType>
  class GatedByDecorator : public NDRegisterAccessorDecorator<UserType, UserType>, public ClosedGateQuery {
   public:
    explicit GatedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target);

    [[nodiscard]] bool isGateClosed() const override;

   protected:
    using NDRegisterAccessorDecorator<UserType>::_target;

    /** The read registration of the base accessor, resolved once at construction; never null. */
    detail::ReadCoordinator::Registration<UserType>* _readRegistration{nullptr};
  };

  /********************************************************************************************************************/

  namespace detail {

    /**
     * Locate the read registration of the base register accessor of a given UserType anywhere in the
     * getInternalElements() chain, or return null. Both raw and cooked base accessors are searched, since a gated
     * register may be read in either mode.
     */
    template<typename UserType>
    ReadCoordinator::Registration<UserType>* findBaseRegisterRegistration(
        const boost::shared_ptr<TransferElement>& element) {
      if(!element) {
        return nullptr;
      }
      if(auto cooked = boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, false>>(element)) {
        return &cooked->readRegistration();
      }
      if(auto raw = boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, true>>(element)) {
        return &raw->readRegistration();
      }
      for(const auto& inner : element->getInternalElements()) {
        auto found = findBaseRegisterRegistration<UserType>(inner);
        if(found) {
          return found;
        }
      }
      return nullptr;
    }

  } // namespace detail

  /********************************************************************************************************************/

  template<typename UserType>
  GatedByDecorator<UserType>::GatedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target)
  : NDRegisterAccessorDecorator<UserType, UserType>(target) {
    // Resolve the base accessor's registration once; this runs only at construction, not on any read path.
    _readRegistration = detail::findBaseRegisterRegistration<UserType>(_target);
    assert(_readRegistration);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool GatedByDecorator<UserType>::isGateClosed() const {
    return _readRegistration->gateClosed();
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(GatedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
