// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class DummyBackendRegisterCatalogue : public NumericAddressedRegisterCatalogue {
   public:
    [[nodiscard]] NumericAddressedRegisterInfo getBackendRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;

    // Helper function to get x from DUMMY_INTERRUPT_x.
    // The first parameter is "true" if an according interrupt is in the catalogue. If the registerPathName
    // is not DUMMY_INTERRUPT_x or the interrupt is not in the catalogue, the first parameter is "false", and the second
    // parameter is invalid.
    [[nodiscard]] std::pair<bool, int> extractControllerInterrupt(const RegisterPath& registerPathName) const;

    [[nodiscard]] std::unique_ptr<BackendRegisterCatalogueBase> clone() const override;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
