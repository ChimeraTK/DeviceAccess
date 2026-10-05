// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterInfoBase.h"
#include "NDRegisterAccessorDecorator.h"
#include "ScalarRegisterAccessor.h"
#include "TransferElementGateHandler.h"

#include <cstdint>
#include <memory>

namespace ChimeraTK {

  class NumericAddressedBackend;

  /********************************************************************************************************************/

  /**
   * Decorator implementing the runtime 'selectedBy' gate over a gated data accessor.
   *
   * Wraps a NDRegisterAccessor<UserType> that reads a register (or named channel slice, or
   * double-buffer) declared 'selectedBy' and adds the gating behaviour: while the selector register
   * does not equal the expected value the wrapped read is reported DataValidity::faulty (and, for the
   * double-buffer case, treated as not-new so the buffer is not swapped). The gate reports its decision
   * both through `doPostRead()` (validity level, used by the polled/validity read paths) and through
   * `check()` (wake/version level, used by the asynchronous interrupt path).
   *
   * The selector register is read through a cheap synchronous scalar accessor
   * (getSyncRegisterAccessor<int64_t>), owned by this decorator and read by the decorator itself
   * (see isGateOpen()/check()/doPostRead()) whenever the gate must be evaluated.
   */
  template<typename UserType>
  class SelectedByDecorator : public NDRegisterAccessorDecorator<UserType>, public TransferElementGateHandler {
   public:
    /**
     * Wrap 'target' with a self-owned selector read: the decorator creates and owns the scalar accessor
     * reading 'selectedBy.regPath' and reads it on each gate evaluation.
     */
    SelectedByDecorator(const boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
        const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy);

    /** Reset the gate cache and forward the pre-read to the wrapped accessor. */
    void doPreRead(TransferType type) override;

    /** Skip the physical transfer of the wrapped accessor while the gate is closed (polled semantics). */
    void doReadTransferSynchronously() override;

    /** Forward the read to the wrapped accessor and apply the gating validity. */
    void doPostRead(TransferType type, bool hasNewData) override;

    /**
     * Evaluate the current selector state and return whether the selector register currently selects this
     * gate. false means the gated data is DataValidity::faulty. Reads the owned selector accessor first so
     * the returned value is current.
     */
    bool check();

    /** Whether a selector register has been attached to this gate. */
    [[nodiscard]] bool hasSelection() const { return _selectorAccessor.get() != nullptr; }

    // Implementation of the TransferElementGateHandler interface: exposes the gated data elements to a
    // TransferGroup so their transfer can be suppressed while unselected. isGateOpen() reads the owned
    // selector itself.
    std::vector<boost::shared_ptr<TransferElement>> getGatedElements() override;
    bool isGateOpen() override;

   protected:
    using NDRegisterAccessorDecorator<UserType>::_target;
    using NDRegisterAccessorDecorator<UserType>::buffer_2D;

    /// The scalar accessor reading the selector register (owned by this decorator).
    boost::shared_ptr<ScalarRegisterAccessor<int64_t>> _selectorAccessor;
    int64_t _expectedValue{0};

    /// Cached gate decision from the most recent selector read during the current transfer cycle.
    bool _cachedGateOpen{false};
    /// Whether _cachedGateOpen is fresh for the current transfer cycle (set by isGateOpen()/check(),
    /// consumed and cleared by doPostRead()).
    bool _cacheValid{false};

    /// Evaluate the already-populated selector buffer against _expectedValue.
    bool evaluate();

    /// (Re)initialise the expected selector value.
    void configure(int64_t expectedValue);
  };

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(SelectedByDecorator);

  /********************************************************************************************************************/

} // namespace ChimeraTK
