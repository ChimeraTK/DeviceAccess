// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

namespace ChimeraTK {

  /********************************************************************************************************************/

  /**
   * Non-template interface through which the gate result of an accessor can be queried without knowing its UserType or
   * concrete type. It is brought in by GatedByDecorator, so the asynchronous distributor can ask whether the gate of
   * the accessor it polls is closed. It is kept in its own header so the asynchronous distributor can see it without
   * pulling in the (heavy) accessor decorator, which would create an include cycle.
   */
  class ClosedGateQuery {
   public:
    virtual ~ClosedGateQuery() = default;

    /** True while the gate of the decorated accessor is closed. */
    [[nodiscard]] virtual bool isGateClosed() const = 0;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
