#pragma once

#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class DummyBackendRegisterCatalogue : public NumericAddressedRegisterCatalogue {
   public:
    [[nodiscard]] NumericAddressedRegisterInfo getBackendRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;

    // Helper function to get x and y from DUMMY_INTERRUPT_x_y. Throws if
    // x, y is not in the list of known interrupts.
    std::pair<int, int> extractControllerInterrupt(const RegisterPath& registerPathName) const;

    std::unique_ptr<BackendRegisterCatalogueBase> clone() const override;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
