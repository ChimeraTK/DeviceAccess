// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterInfoBase.h"
#include "ScalarRegisterAccessor.h"

#include <cstdint>
#include <optional>

namespace ChimeraTK {

  class NumericAddressedBackend;

  /**
   * Runtime gate for a register or channel declared 'selectedBy': it decides whether the
   * register/channel is currently "active" by comparing a selector register against the
   * expected value. Reads of the selector register go through a cheap synchronous scalar
   * accessor (getSyncRegisterAccessor<int64_t>), so the gate is one narrow read per check.
   *
   * The gate reports validity per a configurable policy: while the selection is not met the
   * read is DataValidity::faulty; optionally (forceFirstFaulty) the very first check is
   * reported as faulty until a matching selector value has actually been observed, so that a
   * freshly-subscribed consumer does not spuriously see valid data before the selector has
   * been set.
   */
  class SelectorGate {
   public:
    SelectorGate() = default;

    /**
     * (Re)initialise the gate for a selector register. Creates (or replaces) the scalar
     * accessor reading 'selectedBy.regPath' and stores the value the register must equal to
     * be considered active. If 'forceFirstFaulty' is set, the first dataValidity() call after
     * (re)initialisation returns faulty until a check() observed a matching selector.
     */
    void replace(const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy,
        bool forceFirstFaulty);

    /**
     * Attach a selector accessor that is owned and read by an external TransferGroup instead
     * of by this gate. Multiple gates may share the same accessor, so the TransferGroup reads
     * the selector register at most once per poll (deduplicated). check() then evaluates the
     * already-read value rather than issuing its own read; the caller must ensure the accessor
     * has been read (via the TransferGroup) before check() is called.
     */
    void attach(const boost::shared_ptr<ScalarRegisterAccessor<int64_t>>& accessor, int64_t expectedValue,
        bool forceFirstFaulty);

    /**
     * Create a scalar accessor for the selector register, to be owned (and read) by a
     * TransferGroup and shared by several gates. Static so it can also act as a factory from
     * NumericAddressedBackend's friend context.
     */
    static boost::shared_ptr<ScalarRegisterAccessor<int64_t>> makeSharedAccessor(
        const boost::shared_ptr<NumericAddressedBackend>& backend, const SelectedBy& selectedBy);

    /**
     * Evaluate the current selector state and remember whether the selector register currently
     * selects this gate. For a self-owned gate (replace/{} constructor) this reads the selector
     * register; for a TransferGroup-managed gate (attach) it only reads the already-populated
     * buffer.
     */
    bool check();

    /**
     * Validity of the gated data. Returns DataValidity::faulty while the selection is not
     * met, and DataValidity::ok once the selector matches. With forceFirstFaulty the result
     * stays faulty until the first matching check().
     */
    DataValidity dataValidity() const { return _matches ? DataValidity::ok : DataValidity::faulty; }

    /** Whether a selector register has been attached to this gate. */
    explicit operator bool() const { return _accessor.get() != nullptr; }

   private:
    boost::shared_ptr<ScalarRegisterAccessor<int64_t>> _accessor;
    int64_t _expectedValue{0};
    bool _matches{true};
    bool _forceFirstFaulty{false};
    bool _firstCheck{true};
    /// True when the selector accessor is owned and read by an external TransferGroup; check()
    /// must not issue its own read in that case.
    bool _managedExternally{false};
  };

} // namespace ChimeraTK
