#pragma once

#include "NumericAddressedRegisterCatalogue.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  class DummyBackendRegisterCatalogue : public NumericAddressedRegisterCatalogue {
   public:
    [[nodiscard]] NumericAddressedRegisterInfo getBackendRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
