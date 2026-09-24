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

    /** Read the selector register and remember whether it currently selects this gate. */
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
    ScalarRegisterAccessor<int64_t> _accessor;
    int64_t _expectedValue{0};
    bool _matches{true};
    bool _forceFirstFaulty{false};
    bool _firstCheck{true};
  };

} // namespace ChimeraTK
