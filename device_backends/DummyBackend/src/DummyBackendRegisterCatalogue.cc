#include "DummyBackendRegisterCatalogue.h"

namespace ChimeraTK {

  static const std::string DUMMY_WRITEABLE_SUFFIX{"DUMMY_WRITEABLE"};

  /********************************************************************************************************************/

  NumericAddressedRegisterInfo DummyBackendRegisterCatalogue::getBackendRegister(
      const RegisterPath& registerPathName) const {
    auto path = registerPathName;
    path.setAltSeparator(".");
    if(path.endsWith(DUMMY_WRITEABLE_SUFFIX)) {
      auto basePath = path;
      basePath--;
      auto info = NumericAddressedRegisterCatalogue::getBackendRegister(basePath);
      info.registerAccess = NumericAddressedRegisterInfo::Access::READ_WRITE;
      return info;
    }
    return NumericAddressedRegisterCatalogue::getBackendRegister(path);
  }

  /********************************************************************************************************************/

  bool DummyBackendRegisterCatalogue::hasRegister(const RegisterPath& registerPathName) const {
    auto path = registerPathName;
    path.setAltSeparator(".");
    if(path.endsWith(DUMMY_WRITEABLE_SUFFIX)) {
      auto basePath = path;
      basePath--;
      return NumericAddressedRegisterCatalogue::hasRegister(basePath);
    }
    return NumericAddressedRegisterCatalogue::hasRegister(path);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
