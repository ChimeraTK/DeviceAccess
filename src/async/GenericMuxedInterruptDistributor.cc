// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "async/GenericMuxedInterruptDistributor.h"

#include "async/SubDomain.h"
#include <nlohmann/json.hpp> // https://json.nlohmann.me/features/element_access/
#include <unordered_map>
#include <unordered_set>

#include <boost/bimap.hpp>

#include <algorithm>
#include <sstream>
#include <vector>

namespace ChimeraTK::async {
  struct JsonDescriptorKeysV1 {
    inline static constexpr const int jsonDescriptorVersion = 1;
    inline static constexpr const char* const VERSION_JSON_KEY = "version";
    inline static constexpr const char* const OPTIONS_JSON_KEY = "options";
    inline static constexpr const char* const PATH_JSON_KEY = "path";
  };

  using JdkV1 = JsonDescriptorKeysV1;

  /********************************************************************************************************************/

  /**
   * This is an initializer for a boost::bimap so that it can be produced using nice syntax.
   * Ex: static const auto OptionCodeMap = makeBimap({ {"SIE", SIE}, {"IER", IER}, ... })
   */
  boost::bimap<std::string, GmidOptionCode> makeBimap(
      std::initializer_list<typename boost::bimap<std::string, GmidOptionCode>::value_type> list) {
    return {list.begin(), list.end()};
  }

  /********************************************************************************************************************/

  static const auto OptionCodeMap = makeBimap({
      {"SIE", SIE},
      {"IER", IER},
      {"MER", MER},
      {"MIE", MIE},
      {"GIE", GIE},
      {"ISR", ISR},
      {"ICR", ICR},
      {"IAR", IAR},
      {"IPR", IPR},
      {"CIE", CIE},
      // Then the unacceptable ones
      {"IMR", IMaskR},
      {"IModeR", IModeR}, // REMOVE?
      {"ILR", ILR},       // REMOVE?
      {"IVR", IVR},       // REMOVE?
      {"IVAR", IVAR},     // REMOVE?
      {"IVEAR", IVEAR},   // REMOVE?
      {"INVALID_OPTION_CODE", INVALID_OPTION_CODE},
  });

  /********************************************************************************************************************/

  /**
   * Return a 32 bit mask with the ithInterrupt bit from the left set to 1 and all others 0
   */
  inline uint32_t iToMask(const uint32_t ithInterrupt) {
    return 0x1U << ithInterrupt;
  }

  /********************************************************************************************************************/

  /**
   * If the string is not a recognized option code, returns GmidOptionCode::INVALID_OPTION_CODE
   * It is not the job of this function to do any control logic on these code, just convert the strings
   * It uses ChimeraTK::async::GmidOptionCodeMap
   * Example: "ISR" -> GmidOptionCode::ISR
   * Example: "useless" -> GmidOptionCode::INVALID_OPTION_CODE
   */
  GmidOptionCode getOptionRegisterEnum(const std::string& opt) {
    auto it = OptionCodeMap.left.find(opt);
    if(it != OptionCodeMap.left.end()) { // If a valid code
      return it->second;                 // return the corresponding enum.
    }
    // invalid code
    return GmidOptionCode::INVALID_OPTION_CODE;
  }

  /********************************************************************************************************************/

  /**
   * Given the Register option code enum, returns the corresponding string.
   */
  std::string getOptionRegisterStr(GmidOptionCode optCode) {
    auto it = OptionCodeMap.right.find(optCode);
    if(it != OptionCodeMap.right.end()) { // If a valid code
      return it->second;                  // return the corresponding str.
    }
    // invalid code
    return "INVALID_OPTION_CODE";
  }

  /********************************************************************************************************************/

  /**
   * This returns strings explaining the option code acronyms for use in error messages.
   * Ex: explainOptCode(ISR) ==> "ISR (Interrupt Status Register)"
   */
  std::string explainOptCode(GmidOptionCode optCode) {
    // clang-format off
      static std::map<GmidOptionCode, std::string> mapOptCode2Message= {
          {ISR, "Interrupt Status Register"},
          {IER, "Interrupt Enable Register"},
          {MER, "Master Enable Register"},
          {MIE, "Master Interrupt Enable"},
          {GIE, "Global Interrupt Enable"},
          {ICR, "Interrupt Clear Register"},
          {IAR, "Interrupt Acknowledge Register"},
          {IPR, "Interrupt Pending Register"},
          {SIE, "Set Interrupt Enable"},
          {CIE, "Clear Interrupt Enable"},
          {IMaskR,
              "Interrupt Mask Register, not to be confused with Interrupt Mode Register"},
          {IModeR,
              "Interrupt Mode Register, what AXI INTC v4.1 calls this 'IMR', not to be confused with Interrupt Mask "
              "Register, which we call 'IMR'"},//REMOVE?
          {IVR, "Interrupt Vector Register"}, //REMOVE?
          {ILR, "Interrupt Level Register"},//REMOVE?
          {IVAR, "Interrupt Vector Address Register"},//REMOVE?
          {IVEAR, "Interrupt Vector Extended Address Register"},//REMOVE?
          {INVALID_OPTION_CODE, "No such option code is known"}
      };
    // clang-format on
    return getOptionRegisterStr(optCode) + " (" + mapOptCode2Message[optCode] + ")";
  }

  /********************************************************************************************************************/

  /**
   * The default delimiter is ','
   * TODO move this to some string helper library
   */
  std::string strSetToStr(const std::set<std::string>& strSet, char delimiter = ',') {
    std::string result;
    if(not strSet.empty()) {
      for(const auto& str : strSet) {
        result += str + delimiter;
      }
      result.pop_back();
    }
    return result;
  }

  /********************************************************************************************************************/

  /**
   * Return a string describing the intVec of the form "1,2,3"
   * The default delimiter is ','
   * TODO move this to some string helper library
   */
  std::string intVecToStr(const std::vector<size_t>& intVec, char delimiter = ',') {
    std::string result;
    if(not intVec.empty()) {
      for(const auto& i : intVec) {
        result += std::to_string(i) + delimiter;
      }
      result.pop_back();
    }
    return result;
  }

  /********************************************************************************************************************/

  /**
   * Return a string describing the controllerID of the form "[1,2,3]"
   */
  std::string controllerIDToStr(const std::vector<size_t>& controllerID) {
    return "[" + intVecToStr(controllerID, ',') + "]";
  }

  /********************************************************************************************************************/

  /**
   * This extracts and validates data from the json snippet 'descriptorJsonStr' that matches the version 1 format
   * Expect 'descriptionJsonStr' of the form  {"path":"APP.INTCB", "options":{"ICR", "IPR", "MER"...}, "version":1}
   * controllerID is used for error reporting only.
   * Returns a pair containing {option flags, registerPath}
   * options flags as a bitset indexed by the GmidOptionCode enum
   * registerPath takes the value keyed in the descriptionJson by JsonDescriptorStandardV0::PATH_JSON_KEY ("path")
   */
  std::pair<std::bitset<OPTION_CODE_COUNT>, std::string> parseAndValidateJsonDescriptionStrV0(
      const std::vector<size_t>& controllerID, const std::string& descriptionJsonStr) {
    /*
     * throws ChimeraTK::logic_error if there are any problems:
     *    throws if there are any unexpected keys
     *    throws if there the json snippet is unparsable
     *    throws if there the path is not in the snippet
     *    throws if the json version is != 1
     *    throws if invalid options given.
     * This does not validate the logic of the 'options' combination,
     * this only validates the json snippet.
     */

    static constexpr const int defaultJsonDescriptorVersion = JdkV1::jsonDescriptorVersion;
    static const std::vector<std::string> defaultOptionRegisterNames = {"ISR", "IER"};
    std::string registerPath;
    std::bitset<OPTION_CODE_COUNT> optionRegisterSettings(0);

    nlohmann::json descriptionJson;
    try { // **Parse jsonDescriptor and sanitize inputs**
      descriptionJson = nlohmann::json::parse(descriptionJsonStr);
    }
    catch(const nlohmann::json::parse_error& ex) {
      std::ostringstream oss;
      oss << "GenericMuxedInterruptDistributor " << controllerIDToStr(controllerID)
          << " was unable to parse map file json snippet " << descriptionJsonStr;
      throw ChimeraTK::logic_error(oss.str());
    }

    // Check that there are no unexpected json keys, throw if there are unexpected keys.
    for(auto& el : descriptionJson.items()) {
      if(el.key() != JdkV1::PATH_JSON_KEY and el.key() != JdkV1::OPTIONS_JSON_KEY and
          el.key() != JdkV1::VERSION_JSON_KEY) {
        std::ostringstream oss;
        oss << "Unknown JSON key '" << el.key() << "' provided to map file for GenericMuxedInterruptDistributor "
            << controllerIDToStr(controllerID);
        throw ChimeraTK::logic_error(oss.str());
      }
    }

    try { // to get registerPath
      descriptionJson[JdkV1::PATH_JSON_KEY].get_to(registerPath);
    }
    catch(const nlohmann::json::exception& e) {
      std::ostringstream oss;
      oss << "Map file json register path key '" << JdkV1::PATH_JSON_KEY
          << "' error for GenericMuxedInterruptDistributor " << controllerIDToStr(controllerID) << ": " << e.what();
      throw ChimeraTK::logic_error(oss.str());
    }

    try { // Get Version and check version
      if(descriptionJson.value("version", defaultJsonDescriptorVersion) != JdkV1::jsonDescriptorVersion) {
        // version: version of this json descriptor.
        // if version != 1, or whatever the expected version currently is, throw; if no version, assume 1
        std::ostringstream oss;
        oss << "GenericMuxedInterruptDistributor " << controllerIDToStr(controllerID) << " expects a "
            << JdkV1::VERSION_JSON_KEY << " " << JdkV1::jsonDescriptorVersion << " JSON descriptor, "
            << JdkV1::VERSION_JSON_KEY << " "
            << descriptionJson.value(JdkV1::VERSION_JSON_KEY, defaultJsonDescriptorVersion) << " was received.";
        throw ChimeraTK::logic_error(oss.str());
      }
    }
    catch(const nlohmann::json::exception& e) {
      std::ostringstream oss;
      oss << "Map file json " << JdkV1::VERSION_JSON_KEY << " key error for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": " << e.what();
      throw ChimeraTK::logic_error(oss.str());
    }

    std::vector<std::string> optionRegisterNames;
    try { // Get Options
      optionRegisterNames = descriptionJson.value(JdkV1::OPTIONS_JSON_KEY, defaultOptionRegisterNames);
      // Defaults options are used since options are optional
    }
    catch(const nlohmann::json::exception& e) {
      std::ostringstream oss;
      oss << "Map file json " << JdkV1::OPTIONS_JSON_KEY << " key error for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": " << e.what();
      throw ChimeraTK::logic_error(oss.str());
    }

    /*
     * Note that the case where the json option is provided but empty is covered by
     * turning on ISR in the constructor and turn on IER with a check below.
     */

    std::set<std::string> invalidOptionRegisterNamesEncountered;
    for(const auto& orn : optionRegisterNames) {
      if(GmidOptionCode ornCode = getOptionRegisterEnum(orn); ornCode != INVALID_OPTION_CODE) {
        optionRegisterSettings.set(ornCode);
      }
      else {
        invalidOptionRegisterNamesEncountered.insert(orn);
      }
    }
    if(!invalidOptionRegisterNamesEncountered.empty()) { // Throw if there are unknown options
      std::ostringstream oss;
      oss << "Invalid register options " << strSetToStr(invalidOptionRegisterNamesEncountered)
          << " supplied in the map file json descriptor (key = " << JdkV1::OPTIONS_JSON_KEY
          << ") for GenericMuxedInterruptDistributor " << controllerIDToStr(controllerID);
      throw ChimeraTK::logic_error(oss.str());
    }

    return std::make_pair(optionRegisterSettings, registerPath);
  } // parseAndValidateJsonDescriptionStrV0

  /********************************************************************************************************************/

  /**
   * Ensures permissible combinations of option registers
   * by throwing ChimeraTK::logic_error if there are any problems.
   */
  void steriliseOptionRegisterSettings(
      std::bitset<OPTION_CODE_COUNT> const& optionRegisterSettings, std::vector<size_t> const& controllerID) {
    /*
     * This enforces the following rules:
     *   throw if ISR not set
     *   throw if neither IER nor IMaskR are set.
     *   throw if IMaskR and IER are both set.
     *   throw if SIE xor CIE is set
     *   throw if IMaskR is set as well as SIE or CIE
     *   throw if ICR and IAR are both set.
     *   throw if more than 0 of MIE, GIE, MER are set.
     *   Temporary: throw if IMaskR is set
     */

    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if only SIE or CIE is there, but not both
    if(optionRegisterSettings.test(SIE) != optionRegisterSettings.test(CIE)) {
      std::ostringstream oss;
      oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
          << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": Only " << explainOptCode(SIE) << " or " << explainOptCode(CIE)
          << " is set, but not both.";
      throw ChimeraTK::logic_error(oss.str());
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if IMaskR is set as well as SIE or CIE
    if(optionRegisterSettings.test(IMaskR)) {
      if(optionRegisterSettings.test(SIE)) {
        std::ostringstream oss;
        oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
            << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
            << controllerIDToStr(controllerID) << ": " << explainOptCode(SIE) << " and " << explainOptCode(IMaskR)
            << " cannot not both be set.";
        throw ChimeraTK::logic_error(oss.str());
      }
      if(optionRegisterSettings.test(CIE)) {
        std::ostringstream oss;
        oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
            << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
            << controllerIDToStr(controllerID) << ": " << explainOptCode(CIE) << " and " << explainOptCode(IMaskR)
            << " cannot not both be set.";
        throw ChimeraTK::logic_error(oss.str());
      }
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if both ICR and IAR are there
    if(optionRegisterSettings.test(ICR) and optionRegisterSettings.test(IAR)) {
      std::ostringstream oss;
      oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
          << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": " << explainOptCode(ICR) << " and " << explainOptCode(IAR)
          << " cannot not both be set.";
      throw ChimeraTK::logic_error(oss.str());
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if both IMaskR and IER are there
    if(optionRegisterSettings.test(IMaskR) and optionRegisterSettings.test(IER)) {
      std::ostringstream oss;
      oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
          << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": Only " << explainOptCode(IMaskR) << " and " << explainOptCode(IER)
          << " cannot not both be set.";
      throw ChimeraTK::logic_error(oss.str());
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if neither IER nor IMaskR is set. This should be impossible.
    if(not(optionRegisterSettings.test(IMaskR) or optionRegisterSettings.test(IER))) {
      std::ostringstream oss;
      oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY << " for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": Neither " << explainOptCode(IMaskR) << " nor " << explainOptCode(IER)
          << " are set, one of the two is required.";
      throw ChimeraTK::logic_error(oss.str());
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if more than one entry of [MIE, GIE, MER] is there (test all combinations)
    int nMieGieMer = static_cast<int>(optionRegisterSettings.test(MIE)) +
        static_cast<int>(optionRegisterSettings.test(GIE)) + static_cast<int>(optionRegisterSettings.test(MER));
    if(nMieGieMer > 1) {
      std::ostringstream oss;
      oss << "Invalid register " << JdkV1::OPTIONS_JSON_KEY
          << " combination specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": Only one out of " << explainOptCode(MIE) << ", "
          << explainOptCode(GIE) << ", and " << explainOptCode(MER) << " can be set; " << std::to_string(nMieGieMer)
          << " are set.";
      throw ChimeraTK::logic_error(oss.str());
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    // Throw if unsupported options options received
    if(optionRegisterSettings.test(IModeR)) { // REMOVE?
      std::ostringstream oss;
      oss << "Unsupported register " << JdkV1::OPTIONS_JSON_KEY
          << " specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": While " << explainOptCode(IModeR)
          << " is a defined options in the AXI IntC v4.1 register space, it is not currently an allowed options in"
             " the GenericMuxedInterruptDistributor";
      throw ChimeraTK::logic_error(oss.str());
    }

    if(optionRegisterSettings.test(IVEAR) or optionRegisterSettings.test(IVAR) or optionRegisterSettings.test(IVR) or
        optionRegisterSettings.test(ILR) or optionRegisterSettings.test(IModeR)) { // REMOVE?
      std::ostringstream oss;
      oss << "Unsupported register " << JdkV1::OPTIONS_JSON_KEY
          << " specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": While " << explainOptCode(ILR) << ", " << explainOptCode(IVR) << ", "
          << explainOptCode(IVAR) << ", and " << explainOptCode(IVEAR)
          << "are defined options in the AXI IntC v4.1 register space, they are not currently allowed options in the "
             "GenericMuxedInterruptDistributor";
      throw ChimeraTK::logic_error(oss.str());
    }

    if(optionRegisterSettings.test(IMaskR)) { // Temporary, this throw should be removed in a later version. TODO
      std::ostringstream oss;
      oss << "Unsupported register " << JdkV1::OPTIONS_JSON_KEY
          << " specified in map file json descriptor for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID) << ": " << explainOptCode(IMaskR)
          << " is not currently an allowed options in the GenericMuxedInterruptDistributor, but should be supported "
             "in a later version. ";
      throw ChimeraTK::logic_error(oss.str());
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    // throw if ISR is not set. This should be impossible.
    if(not optionRegisterSettings.test(ISR)) {
      std::ostringstream oss;
      oss << explainOptCode(ISR) << " is required but is not enabled for GenericMuxedInterruptDistributor "
          << controllerIDToStr(controllerID);
      throw ChimeraTK::logic_error(oss.str());
    }
  } // steriliseOptionRegisterSettings

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  GenericMuxedInterruptDistributor::GenericMuxedInterruptDistributor(
      const boost::shared_ptr<SubDomain<std::nullptr_t>>& parent, const std::string& registerPath,
      std::bitset<GmidOptionCode::OPTION_CODE_COUNT> optionRegisterSettings)
  : MuxedInterruptDistributor(parent), _path(registerPath.c_str()) {
    // Set required registers
    optionRegisterSettings.set(ISR);              // Ensure that the required option ISR is always set.
    if(not optionRegisterSettings.test(IMaskR)) { // Ensure IMaskR or IER is on
      optionRegisterSettings.set(IER);
    }
    // Note that we currently don't allowing IMaskR, but use of it gets caught later.

    // Here we could explicitly note that we're ignoring IPR with optionRegisterSettings.reset(IPR)

    /*----------------------------------------------------------------------------------------------------------------*/
    steriliseOptionRegisterSettings(optionRegisterSettings, parent->getId());

    /*----------------------------------------------------------------------------------------------------------------*/
    _isr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ISR), 1, 0, {});

    _ierIsReallyImaskr = optionRegisterSettings.test(IMaskR);
    _ier = _backend->getRegisterAccessor<uint32_t>(
        _path / getOptionRegisterStr(_ierIsReallyImaskr ? IMaskR : IER), 1, 0, {});

    // Set the clear/acknowledge register {ICR, IAR, ISR}
    if(optionRegisterSettings.test(ICR)) { // Use ICR as the clear register
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ICR), 1, 0, {});
    }
    else if(optionRegisterSettings.test(IAR)) { // Use IAR as the clear register
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(IAR), 1, 0, {});
    }
    else { // Use ISR to clear interrupts using its write 1 to clear feature
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ISR), 1, 0, {});
    }

    _hasMer = optionRegisterSettings.test(MER) or optionRegisterSettings.test(MIE) or optionRegisterSettings.test(GIE);
    if(_hasMer) {
      GmidOptionCode _optionMerMieGie =
          optionRegisterSettings.test(MIE) ? MIE : (optionRegisterSettings.test(GIE) ? GIE : MER);
      _mer = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(_optionMerMieGie), 1, 0, {});
    }

    // steriliseOptionRegisterSettings ensures that either SIE and CIE are both enabled or neither are
    _haveSieAndCie = optionRegisterSettings.test(SIE);
    if(_haveSieAndCie) {
      _sie = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(SIE), 1, 0, {});
      _cie = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(CIE), 1, 0, {});
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    // Check register readability/writeability
    // We have to check the catalogue, because this is the expected behaviour, so we
    // are allowed to throw a logic error here.
    // If the accessor actually behaves differently, it will cause a runtime error when used (which is expected
    // behaviour and OK.) For map-file based backends the behaviour of catalogue and accessor should always be
    // consistent.
    auto catalogue = _backend->getRegisterCatalogue();

    auto checkRedable = [catalogue](auto& accessor) {
      auto description = catalogue.getRegister(accessor->getName());
      if(!description.isReadable()) {
        throw ChimeraTK::logic_error(
            "GenericMuxedInterruptDistributor: Handshake register not readable: " + accessor->getName());
      }
    };
    auto checkWriteable = [catalogue](auto& accessor) {
      auto description = catalogue.getRegister(accessor->getName());
      if(!description.isWriteable()) {
        throw ChimeraTK::logic_error(
            "GenericMuxedInterruptDistributor: Handshake register not writeable: " + accessor->getName());
      }
    };

    checkRedable(_isr);   // we only read from it
    checkWriteable(_ier); // we only write to ier as we are the only user (otherwise we need sie and cie)
    checkWriteable(_icr); // usually write only
    if(_mer) {
      // we only write, never read
      checkWriteable(_mer);
    }
    if(_sie) {
      checkWriteable(_sie); // usually write only
    }
    if(_cie) {
      checkWriteable(_cie); // usually write only
    }
  } // constructor

  /********************************************************************************************************************/
  GenericMuxedInterruptDistributor::~GenericMuxedInterruptDistributor() {
    if(_backend->isFunctional()) {
      try {
        disableInterruptsFromMask(_activeInterrupts);
      }
      catch(ChimeraTK::logic_error& e) {
        // This try/catch is just to silence the linter. ChimeraTK logic errors can always be avoided by checking the
        // according pre-condition. We did so by checking isFunctional, so there should be no exception here.
        std::cerr << "Logic error in ~GenericMuxedInterruptDistributor: " << e.what() << " TERMINATING!" << std::endl;
        std::terminate();
      }
    }

  } // destructor

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::clearInterruptsFromMask(uint32_t mask) {
    try {
      _icr->accessData(0) = mask;
      _icr->write();
    }
    catch(ChimeraTK::runtime_error&) {
    }
  }

  /********************************************************************************************************************/
  inline void GenericMuxedInterruptDistributor::clearOneInterrupt(uint32_t ithInterrupt) {
    clearInterruptsFromMask(iToMask(ithInterrupt));
  }

  /********************************************************************************************************************/
  inline void GenericMuxedInterruptDistributor::clearAllInterrupts() {
    clearInterruptsFromMask(0xFFFFFFFF);
  }

  /********************************************************************************************************************/
  inline void GenericMuxedInterruptDistributor::clearAllEnabledInterrupts() {
    clearInterruptsFromMask(_activeInterrupts);
  }

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::disableInterruptsFromMask(uint32_t mask) {
    _activeInterrupts &= ~mask;
    try {
      if(_ierIsReallyImaskr) {
        // IMaskR is used, so SIE and CIE are not defined.
        _ier->accessData(0) = ~_activeInterrupts;
        _ier->write();
      }
      else {
        if(_haveSieAndCie) {
          _cie->accessData(0) = mask;
          _cie->write();
        }
        else {
          _ier->accessData(0) = _activeInterrupts;
          _ier->write();
        }
      }
      clearInterruptsFromMask(mask);
    }
    catch(ChimeraTK::runtime_error&) {
    }
  }

  /********************************************************************************************************************/
  inline void GenericMuxedInterruptDistributor::disableOneInterrupt(uint32_t ithInterrupt) {
    disableInterruptsFromMask(iToMask(ithInterrupt));
  }

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::enableInterruptsFromMask(uint32_t mask) {
    /*
     * - When creating an accessor to "!0:N" (or a nested interrupt "!0:N:M") //MIR = IMR = IMaskR.
     *   - if SIE and CIE are there, it writes ``1<<N`` to SIE and _clears_ with ``1<<N``
     *   - if IMR is there, it writes ~(``1<<N``) to IMR and _clears_ with ``1<<N``
     *   - if neither (SIE and CIE) nor MIR are present, or only IER is there, it writes ``1<<N`` to IER
     * and _clears_ with ``1<<N``
     * - When creating accessor "!0:L" while still holding "!0:N"
     *   - if SIE and CIE are there, it writes `1<<L` to SIE and to CIE
     *   - if IMR is there, it writes ~( (``1<<N``) | (`1<<L`) ) to MIR and _clears_ with `1<<L`
     *   - if neither (SIE and CIE) nor IMR are present, or only IER is there, it writes ( ``1<<N``)|(`1<<L`)
     * to IER and _clears_ with `1<<L`
     */
    _activeInterrupts |= mask;
    try {
      if(_ierIsReallyImaskr) { // Set IMaskR in the form of IER
        _ier->accessData(0) = ~_activeInterrupts;
        _ier->write();
        // When IMaskR is used, SIE and CIE cannot be defined.
      }
      else {
        if(_haveSieAndCie) { // Set SIE
          _sie->accessData(0) = _activeInterrupts;
          _sie->write();
        }
        else { // Set IER, which actually is IER and not IMaskR
          _ier->accessData(0) = _activeInterrupts;
          _ier->write();
        }
      }
    }
    catch(ChimeraTK::runtime_error&) {
    }
  } // enableInterruptFromMask

  /********************************************************************************************************************/
  inline void GenericMuxedInterruptDistributor::enableOneInterrupt(uint32_t ithInterrupt) {
    enableInterruptsFromMask(iToMask(ithInterrupt));
  }

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::handle(VersionNumber version) {
    try {
      _isr->read();
      uint32_t ipr = _activeInterrupts & _isr->accessData(0);

      for(auto const& [i, subDomainWeakPtr] : _subDomains) {
        // i is the bit index of the subDomain
        if(ipr & iToMask(i)) {
          if(auto subDomain = subDomainWeakPtr.lock(); subDomain) {
            //  The weak pointer might have gone.
            //  TODO FIXME: We need a cleanup function which removes the map entry.
            //  Otherwise we might be stuck with a bad weak pointer which is tried in each handle() call.

            subDomain->distribute(nullptr, version);

            // Requirement: nested interrupt handlers must clear their active interrupt flag first,
            // then the parent interrupt flags are cleared.
            clearOneInterrupt(i);
          }
        }
      } // for
    }
    catch(ChimeraTK::runtime_error&) {
      // There's nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
    }
  } // handle

  /********************************************************************************************************************/
  std::unique_ptr<GenericMuxedInterruptDistributor> GenericMuxedInterruptDistributor::create(
      [[maybe_unused]] std::string const& description, const boost::shared_ptr<SubDomain<std::nullptr_t>>& parent) {
    /*
     *  This is a factory function. It parses the json, and calls the constructor.
     *  It returns an initalized GenericMuxedInterruptDistributor.
     *  'description' is a JSON snippet containing configuration data
     */

    auto parseResult = parseAndValidateJsonDescriptionStrV0(parent->getId(), description);
    std::bitset<OPTION_CODE_COUNT> optionRegisterSettings = parseResult.first;
    std::string registerPath = parseResult.second;

    return std::make_unique<GenericMuxedInterruptDistributor>(parent, registerPath, optionRegisterSettings);
  } // create

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::activate(VersionNumber version) {
    if(_hasMer) { // Set MER: turn on the Master Enable and HIE (hardware interrupt enable) bits
      try {
        _mer->accessData(0) = 0x00000003;
        _mer->write();
      }
      catch(ChimeraTK::runtime_error&) {
      }
    }

    // Disable any interrupts for which there is no valid subDomain.
    uint32_t activeInterrupts = 0;
    for(auto const& [i, subDomainWeakPtr] : _subDomains) {
      try {
        if(subDomainWeakPtr.lock()) {
          activeInterrupts |= iToMask(i);
        }
      }
      catch(ChimeraTK::runtime_error&) {
      }
    } // for
    enableInterruptsFromMask(activeInterrupts);

    clearAllEnabledInterrupts();

    // Activate all existing sub-domains. We have to implement a loop here because the parent activate is calling
    // activateSubDomain() internally, which is not necessary because we already wrote to the hardware to do the
    // handshake.
    for(auto& subDomainIter : _subDomains) {
      auto subDomain = subDomainIter.second.lock();
      if(subDomain) {
        subDomain->activate(nullptr, version);
      }
    }
  } // activate

  /********************************************************************************************************************/
  void GenericMuxedInterruptDistributor::activateSubDomain(
      SubDomain<std::nullptr_t>& subDomain, VersionNumber const& version) {
    auto index = subDomain.getId().back();

    enableInterruptsFromMask(iToMask(index));
    clearInterruptsFromMask(iToMask(index));

    subDomain.activate(nullptr, version);
  }

  /********************************************************************************************************************/
} // namespace ChimeraTK::async
