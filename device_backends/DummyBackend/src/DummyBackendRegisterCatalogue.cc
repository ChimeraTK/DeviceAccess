#include "DummyBackendRegisterCatalogue.h"

#include <regex>

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
    if(registerPathName.startsWith("DUMMY_INTERRUPT_") && hasRegister(registerPathName)) {
      NumericAddressedRegisterInfo info(registerPathName, 0 /*nElements*/, 0 /*address*/, 0 /*nBytes*/, 0 /*bar*/,
          0 /*width*/, 0 /*facBits*/, false /*signed*/, NumericAddressedRegisterInfo::Access::WRITE_ONLY,
          NumericAddressedRegisterInfo::Type::VOID);
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
    try {
      (void)extractControllerInterrupt(registerPathName);
      return true;
    }
    catch(ChimeraTK::logic_error&) {
      // Was no DUMMY_INTERRUPT register.
    }
    return NumericAddressedRegisterCatalogue::hasRegister(path);
  }

  /********************************************************************************************************************/

  std::pair<int, int> DummyBackendRegisterCatalogue::extractControllerInterrupt(
      const RegisterPath& registerPathName) const {
    static const std::string DUMMY_INTERRUPT_REGISTER_NAME{"^/DUMMY_INTERRUPT_([0-9]+)_([0-9]+)$"};

    const std::string regPathNameStr{registerPathName};
    const std::regex dummyInterruptRegex{DUMMY_INTERRUPT_REGISTER_NAME};
    std::smatch match;
    std::regex_search(regPathNameStr, match, dummyInterruptRegex);

    if(not match.empty()) {
      // FIXME: Ideally, this test and the need for passing in the lambda function should be done
      // in the constructor of the accessor. But passing down the base-class of the backend is very weird
      // due to the sort-of CRTP pattern used in this base class.
      auto controller = std::stoi(match[1].str());
      auto interrupt = std::stoi(match[2].str());
      try {
        auto& interruptsForController = _mapOfInterrupts.at(unsigned(controller));
        if(interruptsForController.find(unsigned(interrupt)) == interruptsForController.end())
          throw ChimeraTK::logic_error(
              "Invalid interrupt for controller (" + match[0].str() + ", " + match[1].str() + ": " + regPathNameStr);
      }
      catch(std::out_of_range&) {
        throw ChimeraTK::logic_error(
            "Invalid interrupt controller (" + match[0].str() + ", " + match[1].str() + ": " + regPathNameStr);
      }
      return std::make_pair(controller, interrupt);
    }
    throw ChimeraTK::logic_error("Invalid register path " + regPathNameStr);
  }
  /********************************************************************************************************************/

  std::unique_ptr<BackendRegisterCatalogueBase> DummyBackendRegisterCatalogue::clone() const {
    // FIXME: Copy of  NumericAddressedRegisterCatalogue. We don't have different data types
    // or additional data members, but need an instance of DummyBackendRegisterCatalogue
    // for the special functions.
    // This should go away once the pattern is changed to a CRTP, so the base classes know the
    // actual type they are.
    auto* c = new DummyBackendRegisterCatalogue;
    fillFromThis(c);
    c->_mapOfInterrupts = _mapOfInterrupts;
    return std::unique_ptr<BackendRegisterCatalogueBase>(c);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
