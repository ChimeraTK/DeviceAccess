// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "GenericInterruptControllerHandler.h"

#include "TriggerDistributor.h"
#include <nlohmann/json.hpp> // https://json.nlohmann.me/features/element_access/
#include <unordered_map>
#include <unordered_set>

#include <algorithm>
#include <vector>

namespace ChimeraTK {
  namespace genIntC {

    //   /$$$$$$$$
    //  | $$_____/
    //  | $$       /$$$$$$$  /$$   /$$ /$$$$$$/$$$$
    //  | $$$$$   | $$__  $$| $$  | $$| $$_  $$_  $$
    //  | $$__/   | $$  \ $$| $$  | $$| $$ \ $$ \ $$
    //  | $$      | $$  | $$| $$  | $$| $$ | $$ | $$
    //  | $$$$$$$$| $$  | $$|  $$$$$$/| $$ | $$ | $$
    //  |________/|__/  |__/ \______/ |__/ |__/ |__/
    template<typename L, typename R>
    boost::bimap<L, R> _makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
      return boost::bimap<L, R>(list.begin(), list.end());
    }

    static auto optionCodeMap = _makeBimap<std::string, optionCode>({
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
        {"IMR", IMaskR},
        //Then the unacceptable ones
        {"IModeR", IModeR},
        {"ILR", ILR},
        {"IVR", IVR},
        {"IVAR", IVAR},
        {"IVEAR", IVEAR},
    });

    /*****************************************************************************************************************/

    optionCode getOptionRegisterEnum(std::string opt) {
      /*
       * If the string is not a recognized option code, returns optionCode::INVALID_OPTION_CODE
       * It is not the job of this function to do any control logic on these code, just convert the strings
       * Example: "ISR" -> optionCode::ISR
       * Example: "useless" -> optionCode::INVALID_OPTION_CODE
       * uses ChimeraTK::genIntC::optionCodeMap
       */
      auto it = optionCodeMap.left.find(opt);
      if(it != optionCodeMap.left.end()) { // If a valid code
        return it->second;                // return the corresponding enum.
      }
      else { // invalid code
        return optionCode::INVALID_OPTION_CODE;
      }
    }

    /*****************************************************************************************************************/

    std::string getOptionRegisterStr(optionCode optCode) {
      /*
       * Given the Register option code enum, returns the corresponding string.
       */
      auto it = optionCodeMap.right.find(optCode);
      if(it != optionCodeMap.right.end()) { // If a valid code
        return it->second;                 // return the corresponding str.
      }
      else { // invalid code
        return "INVALID_OPTION_CODE";
      }
    }

    /*****************************************************************************************************************/
    //    /$$$$$$   /$$               /$$
    //   /$$__  $$ | $$              |__/
    //  | $$  \__//$$$$$$    /$$$$$$  /$$ /$$$$$$$   /$$$$$$   /$$$$$$$
    //  |  $$$$$$|_  $$_/   /$$__  $$| $$| $$__  $$ /$$__  $$ /$$_____/
    //   \____  $$ | $$    | $$  \__/| $$| $$  \ $$| $$  \ $$|  $$$$$$
    //   /$$  \ $$ | $$ /$$| $$      | $$| $$  | $$| $$  | $$ \____  $$
    //  |  $$$$$$/ |  $$$$/| $$      | $$| $$  | $$|  $$$$$$$ /$$$$$$$/
    //   \______/   \___/  |__/      |__/|__/  |__/ \____  $$|_______/
    //                                              /$$  \ $$
    //                                             |  $$$$$$/
    //                                              \______/
    // TODO move to some string helper library
    std::string strSet2Str(const std::set<std::string>& strSet, char delimiter) {
      // default delimiter is ','
      std::string result = "";
      if(not strSet.empty()) {
        for(const auto& str : strSet) {
          result += str + delimiter;
        }
        result.pop_back();
      }
      return result;
    }

    /*****************************************************************************************************************/

    std::string intVec2Str(const std::vector<uint32_t>& intVec, char delimiter) {
      // default delimiter is ','
      //  Return a string describing the intVec of the form "1,2,3"
      std::string result = "";
      if(not intVec.empty()) {
        for(const auto& i : intVec) {
          result += std::to_string(i) + delimiter;
        }
        result.pop_back();
      }
      return result;
    }

    /*****************************************************************************************************************/

    std::string controllerID2Str(const std::vector<uint32_t>& controllerID) {
      // Return a string describing the controllerID of the form "[1,2,3]"
      return "[" + intVec2Str(controllerID, ',') + "]";
    }


    /****************************************************************************************************************/
    //      /$$$$$  /$$$$$$   /$$$$$$  /$$   /$$
    //     |__  $$ /$$__  $$ /$$__  $$| $$$ | $$
    //        | $$| $$  \__/| $$  \ $$| $$$$| $$
    //        | $$|  $$$$$$ | $$  | $$| $$ $$ $$
    //   /$$  | $$ \____  $$| $$  | $$| $$  $$$$
    //  | $$  | $$ /$$  \ $$| $$  | $$| $$\  $$$
    //  |  $$$$$$/|  $$$$$$/|  $$$$$$/| $$ \  $$
    //   \______/  \______/  \______/ |__/  \__/
    std::bitset<OPTION_CODE_COUNT> parseAndValidateJsonDescriptionV1(
        std::vector<uint32_t> const& controllerID, // for error reporting only.
        std::string const& description,            // THE JSON SNIPPET
        std::string& registerPath) {               // a return value

      /* Extracts and validates data from the json snippet 'descriptor' that matches the version 1 format
       * Expect description of the form  {"path":"APP.INTCB", "options":{"ICR", "IPR", "MER"...}, "version":1}
       * Extracts the value associated with the "path" json flag to registerPath
       * Returns options flags as a bitset indexed by the optionCode enum
       * throws ChimeraTK::logic_error if there are any problems:
       *    throws if there are any unexpected keys 
       *    throws if there the json snippet is unparsable
       *    throws if there the path is not in the snippet
       *    throws if the json version is != 1
       *    throws if invalid options given.
       * This does not validate the logic of the 'options' combination, 
       * this only validates the json snippet.
       */

      const jsonDescriptorStandardV1 jdv1;
      static const int defaultJsonDescriptorVersion = jdv1.jsonDescriptorVersion;
      static const std::vector<std::string> defaultOptionRegisterNames = {"ISR", "IER", "MER"};

      nlohmann::json jsonDescription;
      try { // **Parse jsonDescriptor and sanitize inputs**
        jsonDescription = nlohmann::json::parse(description);
      }
      catch(const nlohmann::json::parse_error& ex) {
        throw ChimeraTK::logic_error("GenericInterruptControllerHandler " + controllerID2Str(controllerID) +
            " was unable to parse map file json snippet " + description);
      }

      // Check that there are no unexpected json keys, else throw
      for(auto& el : jsonDescription.items()) {
        if(el.key() != jdv1.PATH_JSON_KEY and el.key() != jdv1.OPTIONS_JSON_KEY and el.key() != jdv1.VERSION_JSON_KEY) {
          throw ChimeraTK::logic_error("Unknown JSON key '" + el.key() +
              "' provided to map file for GenericInterruptControllerHandler " + controllerID2Str(controllerID));
        }
      }

      try {                     // Get registerPath
        jsonDescription[jdv1.PATH_JSON_KEY].get_to(registerPath);
      }
      catch(const nlohmann::json::exception& e) {
        throw ChimeraTK::logic_error("Map file json register path key '" + jdv1.PATH_JSON_KEY +
            "' error for GenericInterruptControllerHandler " + controllerID2Str(controllerID) + ": " + e.what());
      }

      try { // Get Version and check version
        if(jsonDescription.value("version", defaultJsonDescriptorVersion) != jdv1.jsonDescriptorVersion) {
          // version: version of this json descriptor.
          // if version != 1, throw; if no version, assume 1
          throw ChimeraTK::logic_error("GenericInterruptControllerHandler " + controllerID2Str(controllerID) +
              " expects a " + jdv1.VERSION_JSON_KEY + " " + std::to_string(jdv1.jsonDescriptorVersion) + " JSON descriptor, " +
              jdv1.VERSION_JSON_KEY + " " + std::to_string(jsonDescription.value(jdv1.VERSION_JSON_KEY, defaultJsonDescriptorVersion)) +
              " was received.");
        }
      }
      catch(const nlohmann::json::exception& e) {
        throw ChimeraTK::logic_error("Map file json " + jdv1.VERSION_JSON_KEY +
            " key error for GenericInterruptControllerHandler " + controllerID2Str(controllerID) + ": " + e.what());
      }

      std::vector<std::string> optionRegisterNames;
      try { // Get Options
        optionRegisterNames =
            jsonDescription.value(jdv1.OPTIONS_JSON_KEY, defaultOptionRegisterNames); // option are optional
      }
      catch(const nlohmann::json::exception& e) {
        throw ChimeraTK::logic_error("Map file json " + jdv1.OPTIONS_JSON_KEY +
            " key error for GenericInterruptControllerHandler " + controllerID2Str(controllerID) + ": " + e.what());
      }
      /*
       * The case where the json option is provided but empty is covered by
       * turning on ISR immediately and turn on IER with a check below.
       */

      std::bitset<OPTION_CODE_COUNT> optionRegisterSettings(0);
      std::set<std::string> invalidOptionRegisterNamesEncountered;
      for(const auto& orn : optionRegisterNames) {
        if(optionCode ornCode = getOptionRegisterEnum(orn); ornCode != INVALID_OPTION_CODE) {
          optionRegisterSettings.set(ornCode);
        } 
        else {
          invalidOptionRegisterNamesEncountered.insert(orn);
        }
      }
      if(invalidOptionRegisterNamesEncountered.size() > 0) { // Throw if there are unknown options
        throw ChimeraTK::logic_error("Invalid register options " + strSet2Str(invalidOptionRegisterNamesEncountered) +
            " supplied in the map file json descriptor (key = " + jdv1.OPTIONS_JSON_KEY +
            ") for GenericInterruptControllerHandler " + controllerID2Str(controllerID));
      }

      return optionRegisterSettings;
    } // parseAndValidateJsonDescriptionV1

    /****************************************************************************************************************/
    //    /$$$$$$   /$$                         /$$ /$$ /$$
    //   /$$__  $$ | $$                        |__/| $$|__/
    //  | $$  \__//$$$$$$    /$$$$$$   /$$$$$$  /$$| $$ /$$  /$$$$$$$  /$$$$$$
    //  |  $$$$$$|_  $$_/   /$$__  $$ /$$__  $$| $$| $$| $$ /$$_____/ /$$__  $$
    //   \____  $$ | $$    | $$$$$$$$| $$  \__/| $$| $$| $$|  $$$$$$ | $$$$$$$$
    //   /$$  \ $$ | $$ /$$| $$_____/| $$      | $$| $$| $$ \____  $$| $$_____/
    //  |  $$$$$$/ |  $$$$/|  $$$$$$$| $$      | $$| $$| $$ /$$$$$$$/|  $$$$$$$
    //   \______/   \___/   \_______/|__/      |__/|__/|__/|_______/  \_______/
    void steriliseOptionRegisterSettings(
        std::bitset<OPTION_CODE_COUNT> const& optionRegisterSettings, std::vector<uint32_t> const& controllerID) {
      /* Ensures permissible combinations of option registers 
       * by throwing ChimeraTK::logic_error if there are any problems. 
       * It enforces the following rules:
       *    Throw if ISR not set
       *    Throw if neither IER nor IMaskR are set.
       *    Throw if SIE xor CIE is set
       *    Throw if IMaskR is set as well as SIE or CIE
       *    Throw if ICR and IAR are both set.
       *    Throw if IMaskR and IER are both set.
       *    Throw if more than 1 of MIE, GIE, MER are set.
       *    Throw if invalid options are set.
       */

      const jsonDescriptorStandardV1 jdv1;

      // throw if only SIE or CIE is there, but not both
      if(optionRegisterSettings.test(SIE) != optionRegisterSettings.test(CIE)) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only SIE or SIE can be set, but not both.");
      }

      if(optionRegisterSettings.test(IMaskR)) {  
        if(optionRegisterSettings.test(SIE)) {
          throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
              " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
              controllerID2Str(controllerID) + ": Only SIE and MIR cannot not both be set.");
        }
        else if(optionRegisterSettings.test(CIE)) {
          throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
              " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
              controllerID2Str(controllerID) + ": Only CIE and MIR cannot not both be set.");
        }
      }

      // throw if both ICR and IAR are there
      if(optionRegisterSettings.test(ICR) and optionRegisterSettings.test(IAR)) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only ICR and IAR cannot not both be set.");
      }

      // throw if both IMaskR and IER are there
      if(optionRegisterSettings.test(IMaskR) and optionRegisterSettings.test(IER)) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only IER and IMR/IMaskR cannot not both be set.");
      }

      // throw if more than one entry of [MIE, GIE, MER] is there (test all combinations)
      int nMieGieMer =
          static_cast<int>(optionRegisterSettings.test(MIE)) + 
          static_cast<int>(optionRegisterSettings.test(GIE)) +
          static_cast<int>(optionRegisterSettings.test(MER));
      if(nMieGieMer > 1) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only one out of MIE, GIE, and MER can be set; " + std::to_string(nMieGieMer) +
            " are set.");
      }

      //Throw if unsupported options options received
      if(optionRegisterSettings.test(IVEAR) or optionRegisterSettings.test(IVAR) or optionRegisterSettings.test(IVR) or
          optionRegisterSettings.test(ILR) or optionRegisterSettings.test(IModeR)) {
        throw ChimeraTK::logic_error("Unsupported register " + jdv1.OPTIONS_JSON_KEY +
            " specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) +
            ": While IModeR, ILR, IVR, IVAR, and IVEAR are defined options in the Axi_intc register space, they are not "
            "currently allowed options the GenericInterruptControllerHandler");
      }

      //throw if ISR is not set. This should be impossible.
      if( not optionRegisterSettings.test(ISR) ){
        throw ChimeraTK::logic_error("ISR is not enabled for GenericInterruptControllerHandler " + controllerID2Str(controllerID));
      }

      //throw if neither IER nor IMaskR is set. This should be impossible.
      if(not (optionRegisterSettings.test(IMaskR) or optionRegisterSettings.test(IER))) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " for GenericInterruptControllerHandler " + controllerID2Str(controllerID) +
            ": Neither IER nor IMaskR are set, one of the two is required.");
      }

    } // steriliseOptionRegisterSettings

    /*****************************************************************************************************************/

    inline uint32_t i2Mask(const uint32_t ithInterrupt) {
      // return a 32 bit mask with the ithInterrupt bit from the left set to 1 and all others 0
      return 0x1U << ithInterrupt;
    }

  } // namespace genIntC
  /********************************************************************************************************************/
  //   /$$        /$$$$$$                      /$$$$$$             /$$      /$$$$$$
  //  |  $$      /$$__  $$                    |_  $$_/            | $$     /$$__  $$
  //   \  $$    | $$  \__/  /$$$$$$  /$$$$$$$   | $$   /$$$$$$$  /$$$$$$  | $$  \__/
  //    \  $$   | $$ /$$$$ /$$__  $$| $$__  $$  | $$  | $$__  $$|_  $$_/  | $$
  //     \  $$  | $$|_  $$| $$$$$$$$| $$  \ $$  | $$  | $$  \ $$  | $$    | $$
  //      \  $$ | $$  \ $$| $$_____/| $$  | $$  | $$  | $$  | $$  | $$ /$$| $$    $$
  //       \  $$|  $$$$$$/|  $$$$$$$| $$  | $$ /$$$$$$| $$  | $$  |  $$$$/|  $$$$$$/
  //        \__/ \______/  \_______/|__/  |__/|______/|__/  |__/   \___/   \______/
  /********************************************************************************************************************/

  using namespace ChimeraTK::genIntC;

  /*****************************************************************************************************************/
  //    /$$$$$$                                  /$$                                     /$$
  //   /$$__  $$                                | $$                                    | $$
  //  | $$  \__/  /$$$$$$  /$$$$$$$   /$$$$$$$ /$$$$$$    /$$$$$$  /$$   /$$  /$$$$$$$ /$$$$$$    /$$$$$$   /$$$$$$
  //  | $$       /$$__  $$| $$__  $$ /$$_____/|_  $$_/   /$$__  $$| $$  | $$ /$$_____/|_  $$_/   /$$__  $$ /$$__  $$
  //  | $$      | $$  \ $$| $$  \ $$|  $$$$$$   | $$    | $$  \__/| $$  | $$| $$        | $$    | $$  \ $$| $$  \__/
  //  | $$    $$| $$  | $$| $$  | $$ \____  $$  | $$ /$$| $$      | $$  | $$| $$        | $$ /$$| $$  | $$| $$
  //  |  $$$$$$/|  $$$$$$/| $$  | $$ /$$$$$$$/  |  $$$$/| $$      |  $$$$$$/|  $$$$$$$  |  $$$$/|  $$$$$$/| $$
  //   \______/  \______/ |__/  |__/|_______/    \___/  |__/       \______/  \_______/   \___/   \______/ |__/
  //constructor
  GenericInterruptControllerHandler::GenericInterruptControllerHandler(
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      boost::shared_ptr<TriggerDistributor> parent, std::string registerPath,
      std::bitset<optionCode::OPTION_CODE_COUNT> optionRegisterSettings)
  : InterruptControllerHandler(controllerHandlerFactory, controllerID, std::move(parent)), _activeInterrupts(0), _path(registerPath.c_str()) {

    // Set required registers
    optionRegisterSettings.set(ISR); // Ensure that the required option ISR is always set.
    if(not optionRegisterSettings.test(IMaskR) ) { // Ensure IMaskR or IER is on
      optionRegisterSettings.set(IER);
    }
    //Here we could explicitly note we're ignoring IPR with optionRegisterSettings.reset(IPR)

    steriliseOptionRegisterSettings(optionRegisterSettings, controllerID);

    _isr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ISR), 1, 0, {});

    _ierIsReallyImaskr = optionRegisterSettings.test(IMaskR);
    _ier = _backend->getRegisterAccessor<uint32_t>(_path / (_ierIsReallyImaskr ? IMaskR : IER), 1, 0, {});

    // Set the clear/acknowledge register {ICR, IAR, ISR}
    if (optionRegisterSettings.test(ICR)) {  //Use ICR as the clear register
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ICR), 1, 0, {});
    }
    else if(optionRegisterSettings.test(IAR)) { //Use IAR as the clear register
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(IAR), 1, 0, {});
    }
    else{ //Use ISR to clear interrupts using its write 1 to clear feature 
      _icr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ISR), 1, 0, {});
    }

    _hasMer = optionRegisterSettings.test(MER) or optionRegisterSettings.test(MIE) or optionRegisterSettings.test(GIE);
    if(_hasMer ) {
      optionCode _optionMerMieGie =
          optionRegisterSettings.test(MIE) ? MIE : (optionRegisterSettings.test(GIE) ? GIE : MER);
      _mer = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(_optionMerMieGie), 1, 0, {});
    }

    //steriliseOptionRegisterSettings ensures that either SIE and CIE are both enabled or neither are
    _haveSieAndCie = optionRegisterSettings.test(SIE);
    if(_haveSieAndCie){ 
      _sie= _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(SIE), 1, 0, {});
      _cie= _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(CIE), 1, 0, {});
    }

    // Check register readability
    if(!_isr->isReadable()) {
      throw ChimeraTK::runtime_error(
          "DummyInterruptControllerHandler: Handshake register not readable: " + _isr->getName());
    }
    if(!_ier->isReadable()) {
      throw ChimeraTK::runtime_error(
          "DummyInterruptControllerHandler: Handshake register not readable: " + _ier->getName());
    }
    if(!_icr->isReadable()) {
      throw ChimeraTK::runtime_error(
          "DummyInterruptControllerHandler: Handshake register not readable: " + _icr->getName());
    }
    if(!_mer->isReadable()) {
      throw ChimeraTK::runtime_error(
          "DummyInterruptControllerHandler: Handshake register not readable: " + _mer->getName());
    }
    if(_haveSieAndCie) {
      if(!_sie->isReadable()) {
        throw ChimeraTK::runtime_error(
            "DummyInterruptControllerHandler: Handshake register not readable: " + _sie->getName());
      }
      if(!_cie->isReadable()) {
        throw ChimeraTK::runtime_error(
            "DummyInterruptControllerHandler: Handshake register not readable: " + _cie->getName());
      }
    }
  } //constructor

  /*****************************************************************************************************************/
  //   /$$$$$$$                        /$$                                     /$$
  //  | $$__  $$                      | $$                                    | $$
  //  | $$  \ $$  /$$$$$$   /$$$$$$$ /$$$$$$    /$$$$$$  /$$   /$$  /$$$$$$$ /$$$$$$
  //  | $$  | $$ /$$__  $$ /$$_____/|_  $$_/   /$$__  $$| $$  | $$ /$$_____/|_  $$_/
  //  | $$  | $$| $$$$$$$$|  $$$$$$   | $$    | $$  \__/| $$  | $$| $$        | $$
  //  | $$  | $$| $$_____/ \____  $$  | $$ /$$| $$      | $$  | $$| $$        | $$ /$$
  //  | $$$$$$$/|  $$$$$$$ /$$$$$$$/  |  $$$$/| $$      |  $$$$$$/|  $$$$$$$  |  $$$$/
  //  |_______/  \_______/|_______/    \___/  |__/       \______/  \_______/   \___/
  GenericInterruptControllerHandler::~GenericInterruptControllerHandler() {
    _clearAllEnabledInterrupts();
  } //destructor

  /*****************************************************************************************************************/
  //    /$$$$$$  /$$
  //   /$$__  $$| $$
  //  | $$  \__/| $$  /$$$$$$   /$$$$$$   /$$$$$$
  //  | $$      | $$ /$$__  $$ |____  $$ /$$__  $$
  //  | $$      | $$| $$$$$$$$  /$$$$$$$| $$  \__/
  //  | $$    $$| $$| $$_____/ /$$__  $$| $$
  //  |  $$$$$$/| $$|  $$$$$$$|  $$$$$$$| $$
  //   \______/ |__/ \_______/ \_______/|__/
  void GenericInterruptControllerHandler::_clearInterruptsFromMask(uint32_t mask){
    /* In mask, 1 bits clear the corresponding registers, 0 bits do nothing. 
    */
    try {
      _icr->accessData(0) = mask;
      _icr->write();
    }
    catch(ChimeraTK::runtime_error&) { }
  }

  /*****************************************************************************************************************/

  inline void GenericInterruptControllerHandler::_clearOneInterrupt(uint32_t ithInterrupt){
    _clearInterruptsFromMask(i2Mask(ithInterrupt));
  } 

  /*****************************************************************************************************************/
  
  inline void GenericInterruptControllerHandler::_clearAllInterrupts(){
    _clearInterruptsFromMask(0xFFFFFFFF);
  } 

  /*****************************************************************************************************************/

  inline void GenericInterruptControllerHandler::_clearAllEnabledInterrupts(){
    _clearInterruptsFromMask(_activeInterrupts);
  } 

  /*****************************************************************************************************************/
  //   /$$$$$$$  /$$                     /$$       /$$
  //  | $$__  $$|__/                    | $$      | $$
  //  | $$  \ $$ /$$  /$$$$$$$  /$$$$$$ | $$$$$$$ | $$  /$$$$$$
  //  | $$  | $$| $$ /$$_____/ |____  $$| $$__  $$| $$ /$$__  $$
  //  | $$  | $$| $$|  $$$$$$   /$$$$$$$| $$  \ $$| $$| $$$$$$$$
  //  | $$  | $$| $$ \____  $$ /$$__  $$| $$  | $$| $$| $$_____/
  //  | $$$$$$$/| $$ /$$$$$$$/|  $$$$$$$| $$$$$$$/| $$|  $$$$$$$
  //  |_______/ |__/|_______/  \_______/|_______/ |__/ \_______/
  void GenericInterruptControllerHandler::_disableInterruptsFromMask(uint32_t mask) {
    /* Disables each interrupt corresponding to the 1 bits in mask, and updates _activeInterrupts.
     * Depending on the options, may perform the disable using CIE, IER, or IMaskR.
     */
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
      _clearInterruptsFromMask(mask);
    }
    catch(ChimeraTK::runtime_error&) { }
  }

  /*****************************************************************************************************************/

  inline void GenericInterruptControllerHandler::_disableOneInterrupt(uint32_t ithInterrupt) {
    _disableInterruptsFromMask(i2Mask(ithInterrupt));
  } 

  /*****************************************************************************************************************/
  //   /$$$$$$$$                     /$$       /$$
  //  | $$_____/                    | $$      | $$
  //  | $$       /$$$$$$$   /$$$$$$ | $$$$$$$ | $$  /$$$$$$
  //  | $$$$$   | $$__  $$ |____  $$| $$__  $$| $$ /$$__  $$
  //  | $$__/   | $$  \ $$  /$$$$$$$| $$  \ $$| $$| $$$$$$$$
  //  | $$      | $$  | $$ /$$__  $$| $$  | $$| $$| $$_____/
  //  | $$$$$$$$| $$  | $$|  $$$$$$$| $$$$$$$/| $$|  $$$$$$$
  //  |________/|__/  |__/ \_______/|_______/ |__/ \_______/
  void GenericInterruptControllerHandler::_enableInterruptsFromMask(uint32_t mask) {
    /*
    * For each bit in mask that is a 1, the corresponding interrupt gets enabled
    * The internal copy of enabled registers is updated.
    * 
    * - When creating an accessor to "!0:N" (or a nested interrupt "!0:N:M") //MIR = IMR = IMaskR. 
    *   - [x] if SIE and CIE are there, it writes ``1<<N`` to SIE and _clears_ with ``1<<N``
    *   - [x] if IMR is there, it writes ~(``1<<N``) to IMR and _clears_ with ``1<<N`` 
    *   - [x] if neither (SIE and CIE) nor MIR are present, or only IER is there, it writes ``1<<N`` to IER and _clears_ with ``1<<N``
    * - When creating accessor "!0:L" while still holding "!0:N"
    *   - [x] if SIE and CIE are there, it writes `1<<L` to SIE and to CIE
    *   - [x] if IMR is there, it writes ~( (``1<<N``) | (`1<<L`) ) to MIR and _clears_ with `1<<L`
    *   - [x] if neither (SIE and CIE) nor IMR are present, or only IER is there, it writes ( ``1<<N``)|(`1<<L`) to IER and _clears_ with `1<<L`
    */
    _activeInterrupts |= mask;
    try {
      if(_ierIsReallyImaskr) { //Set IMaskR in the form of IER
        _ier->accessData(0) = ~_activeInterrupts;
        _ier->write();
        //When IMaskR is used, SIE and CIE cannot be defined.
      }
      else { 
        if(_haveSieAndCie) { //Set SIE
          _sie->accessData(0) = mask;
          _sie->write();
        }
        else { //Set IER, which actually is IER and not IMaskR
          _ier->accessData(0) = _activeInterrupts;
          _ier->write();
        }
      }
    }
    catch(ChimeraTK::runtime_error&) { }
  } //_enableInterruptFromMask

  /*****************************************************************************************************************/

  inline void GenericInterruptControllerHandler::_enableOneInterrupt(uint32_t ithInterrupt) {
    _enableInterruptsFromMask(i2Mask(ithInterrupt));
  } 

  /*****************************************************************************************************************/
  //   /$$   /$$                           /$$           /$$
  //  | $$  | $$                          | $$          | $$
  //  | $$  | $$  /$$$$$$  /$$$$$$$   /$$$$$$$  /$$$$$$ | $$
  //  | $$$$$$$$ |____  $$| $$__  $$ /$$__  $$ /$$__  $$| $$
  //  | $$__  $$  /$$$$$$$| $$  \ $$| $$  | $$| $$$$$$$$| $$
  //  | $$  | $$ /$$__  $$| $$  | $$| $$  | $$| $$_____/| $$
  //  | $$  | $$|  $$$$$$$| $$  | $$|  $$$$$$$|  $$$$$$$| $$
  //  |__/  |__/ \_______/|__/  |__/ \_______/ \_______/|__/
  void GenericInterruptControllerHandler::handle(VersionNumber version) {
    /* When a trigger comes in, handle gets called on the interrupt. 
    * It implements the handshake with the interrupt controller
    */

    try {
      _isr->read(); // retain
      uint32_t ipr = _activeInterrupts & _isr->accessData(0);
      for(uint32_t i = 0; i < 32; ++i) {
        if(ipr & i2Mask(i)) { 
          try {
            auto distributor = _distributors.at(i).lock(); // retain
            // The weak pointer might have gone.
            // TODO FIXME: We need a cleanup function which removes the map entry.
            // Otherwise we might be stuck with a bad weak pointer which is tried in each handle() call.

            if(distributor) {
              distributor->distribute(nullptr, version); // retain

              //Requirement: nested interrupt handlers must clear their active interrupt flag first, 
              //then the parent interrupt flags are cleared.
              _clearOneInterrupt(i); 
            }
          } 
          catch(std::out_of_range&) {
            _backend->setException(
                "Error: GenericInterruptControllerHandler reports unknown active interrupt " + std::to_string(i));
          }
        } 
      } //for
    }
    catch(ChimeraTK::runtime_error&) {
      // Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
    } 
  }   // handle

  /*****************************************************************************************************************/
  //    /$$$$$$                                  /$$
  //   /$$__  $$                                | $$
  //  | $$  \__/  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$    /$$$$$$
  //  | $$       /$$__  $$ /$$__  $$ |____  $$|_  $$_/   /$$__  $$
  //  | $$      | $$  \__/| $$$$$$$$  /$$$$$$$  | $$    | $$$$$$$$
  //  | $$    $$| $$      | $$_____/ /$$__  $$  | $$ /$$| $$_____/
  //  |  $$$$$$/| $$      |  $$$$$$$|  $$$$$$$  |  $$$$/|  $$$$$$$
  //   \______/ |__/       \_______/ \_______/   \___/   \_______/
  std::unique_ptr<GenericInterruptControllerHandler> GenericInterruptControllerHandler::create(
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      [[maybe_unused]] std::string const& description, // THE JSON SNIPPET
      boost::shared_ptr<TriggerDistributor> parent) {
    /* This is a factory function. It parses the json, and calls the constructor
     *  it returns an initalized GenericInterruptControllerHandler
     */

    std::string registerPath; //Gets set by parseAndValidateJsonDescriptionV1
    std::bitset<OPTION_CODE_COUNT> optionRegisterSettings =
        parseAndValidateJsonDescriptionV1(controllerID, description, registerPath);

    return std::make_unique<GenericInterruptControllerHandler>(
        controllerHandlerFactory, controllerID, std::move(parent), registerPath, optionRegisterSettings);
  } //create

  /*****************************************************************************************************************/
  //    /$$$$$$              /$$     /$$                       /$$
  //   /$$__  $$            | $$    |__/                      | $$
  //  | $$  \ $$  /$$$$$$$ /$$$$$$   /$$ /$$    /$$ /$$$$$$  /$$$$$$    /$$$$$$
  //  | $$$$$$$$ /$$_____/|_  $$_/  | $$|  $$  /$$/|____  $$|_  $$_/   /$$__  $$
  //  | $$__  $$| $$        | $$    | $$ \  $$/$$/  /$$$$$$$  | $$    | $$$$$$$$
  //  | $$  | $$| $$        | $$ /$$| $$  \  $$$/  /$$__  $$  | $$ /$$| $$_____/
  //  | $$  | $$|  $$$$$$$  |  $$$$/| $$   \  $/  |  $$$$$$$  |  $$$$/|  $$$$$$$
  //  |__/  |__/ \_______/   \___/  |__/    \_/    \_______/   \___/   \_______/
  void GenericInterruptControllerHandler::activate(VersionNumber version){

    if(_hasMer) { //Set MER: turn on the Master Enable and HIE (hardware interrupt enable) bits
      try { 
        _mer->accessData(0) = 0x00000003; 
        _mer->write();
      }
      catch(ChimeraTK::runtime_error&) { }
    }

    //Disable any interrupts for which there is no valid distributor.
    uint32_t activeInterrupts = 0;
    for(uint32_t i = 0; i < 32; ++i) {
        try {
          if(_distributors.at(i).lock()){
            activeInterrupts |= i2Mask(i); 
          }
        }
        catch(std::out_of_range&) {
          _backend->setException(
              "Error: GenericInterruptControllerHandler reports unknown interrupt distributor " + std::to_string(i));
        }
        catch(ChimeraTK::runtime_error&) { }
    } // for
    _enableInterruptsFromMask(activeInterrupts);

    _clearAllEnabledInterrupts();
    //Alternative: _clearAllInterrupts();
    
    InterruptControllerHandler::activate(version);
  } //activate
} // namespace ChimeraTK

/* NOTES
    ILR is the priority threshold to block low priority interrupts to support nested interrupt handling.
    There might be another process which is accessing other interrupts on the same (primary) controller.
*/