// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ClosedGateQuery.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedBackendRegisterAccessor.h"

#include <functional>

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Transparent decorator carrying the gate query of a gated register. NumericAddressedBackend::getSyncRegisterAccessor
   * puts it on the synchronous accessor of a gated register, so it is the outermost object and the accessor the
   * asynchronous distributor polls. It delegates the read and forwards the value, the validity and the version number
   * of its target unchanged; it is not an enforcement point (the gate is enforced where the consumer's raw data is
   * converted, see the low-level element).
   *
   * The decorator resolves the consumer's gate result query once at construction and reads the result through it on
   * each isGateClosed(). The query reads the gate result from the base accessor's read registration, whose address is
   * stable while the accessor lives: a transfer-group merge resets and re-seats the registration in place (it is a
   * member of the base accessor), so the query always sees the current gate result rather than a stale one.
   */
  template<typename UserType>
  class GatedByDecorator : public NDRegisterAccessorDecorator<UserType, UserType>, public ClosedGateQuery {
   public:
    /**
     * Build the decorator on a target whose getInternalElements() chain contains the base accessor of this UserType,
     * and resolve the gate result query from that base accessor's read registration.
     */
    explicit GatedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target);

    /**
     * Build the decorator on a target whose base accessor is not of this UserType (a bit-range target's base accessor
     * is a NumericAddressedBackendRegisterAccessor<uint64_t, false>): the caller passes the already resolved gate
     * result query, taken from the base accessor it attached the gate to.
     */
    GatedByDecorator(
        const boost::shared_ptr<NDRegisterAccessor<UserType>>& target, std::function<bool()> gateClosedQuery);

    [[nodiscard]] bool isGateClosed() const override;

   protected:
    using NDRegisterAccessorDecorator<UserType>::_target;

    /** The consumer's gate result query, resolved once at construction; never empty. */
    std::function<bool()> _gateClosedQuery;
  };

  /********************************************************************************************************************/

  namespace detail {

    /**
     * Locate the base register accessor of a given UserType anywhere in the getInternalElements() chain and return a
     * query for its gate result, or an empty function. Both raw and cooked base accessors are searched, since a gated
     * register may be read in either mode.
     */
    template<typename UserType>
    std::function<bool()> findBaseRegisterGateQuery(const boost::shared_ptr<TransferElement>& element) {
      if(!element) {
        return {};
      }
      if(auto cooked = boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, false>>(element)) {
        return [cooked] { return cooked->readRegistration().gateClosed(); };
      }
      if(auto raw = boost::dynamic_pointer_cast<NumericAddressedBackendRegisterAccessor<UserType, true>>(element)) {
        return [raw] { return raw->readRegistration().gateClosed(); };
      }
      for(const auto& inner : element->getInternalElements()) {
        auto found = findBaseRegisterGateQuery<UserType>(inner);
        if(found) {
          return found;
        }
      }
      return {};
    }

  } // namespace detail

  /********************************************************************************************************************/

  template<typename UserType>
  GatedByDecorator<UserType>::GatedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target)
  : NDRegisterAccessorDecorator<UserType, UserType>(target) {
    // Resolve the gate result query once; this runs only at construction, not on any read path.
    _gateClosedQuery = detail::findBaseRegisterGateQuery<UserType>(_target);
    assert(_gateClosedQuery);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  GatedByDecorator<UserType>::GatedByDecorator(
      const boost::shared_ptr<NDRegisterAccessor<UserType>>& target, std::function<bool()> gateClosedQuery)
  : NDRegisterAccessorDecorator<UserType, UserType>(target), _gateClosedQuery(std::move(gateClosedQuery)) {
    assert(_gateClosedQuery);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool GatedByDecorator<UserType>::isGateClosed() const {
    return _gateClosedQuery();
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(GatedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
