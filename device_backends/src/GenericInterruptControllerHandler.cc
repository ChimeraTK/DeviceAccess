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

    /****************************************************************************************************************/

    std::bitset<OPTION_CODE_COUNT> parseAndValidateJsonDescriptionV1(
        std::vector<uint32_t> const& controllerID, // for error reporting only.
        std::string const& description,            // THE JSON SNIPPET
        std::string& registerPath) {               // a return value

      /*
       * Extracts and validates data from the json snippet 'descriptor' that matches the version 1 format
       * Expect description of the form  {"path":"APP.INTCB", "options":{"ICR", "IPR", "MER"...}, "version":1}
       * Extracts path to registerPath
       * Returns options flags as a bitset indexed by the optionCode enum
       * throws ChimeraTK::logic_error if there are any problems.
       * throws if there are any keys besides
       * Does not validate 'options' combination logic only json names.
       */

      //static const std::string VERSION_JSON_KEY = "version"; //TODO move these out of constants and into an enum or something.
      //static const std::string OPTIONS_JSON_KEY = "options";
      //static const std::string PATH_JSON_KEY = "path";
      //static const int requiredJsonDescriptorVersion = 1;
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

      // Check that there are not unexpected keys, else throw
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
        optionCode ornCode = getOptionRegisterEnum(orn);
        if(ornCode == INVALID_OPTION_CODE) {
          invalidOptionRegisterNamesEncountered.insert(orn);
        }
        else {
          optionRegisterSettings.set(ornCode);
        }
      }
      if(invalidOptionRegisterNamesEncountered.size() > 0) { // Throw if there are unknown options
      //error: invalid initialization of reference of type 
      //‘const std::vector<std::__cxx11::basic_string<char> >&’ from expression of type 
      //‘      std::set<std::__cxx11::basic_string<char> >’
        throw ChimeraTK::logic_error("Invalid register options " + strSet2Str(invalidOptionRegisterNamesEncountered) +
            " supplied in the map file json descriptor (key = " + jdv1.OPTIONS_JSON_KEY +
            ") for GenericInterruptControllerHandler " + controllerID2Str(controllerID));
      }

      return optionRegisterSettings;
    } // parseAndValidateJsonDescriptionV1

    /****************************************************************************************************************/

    void steriliseOptionRegisterSettings(
        std::bitset<OPTION_CODE_COUNT> const& optionRegisterSettings, std::vector<uint32_t> const& controllerID) {
      /*
       * Ensure permissible combinations of option registers
       * If there are any problems, throws ChimeraTK::logic_error
       */

      const jsonDescriptorStandardV1 jdv1;

      // throw if only SIE or CIE is there, but not both
      if(optionRegisterSettings.test(SIE) != optionRegisterSettings.test(CIE)) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only SIE or SIE can be set, but not both.");
      }

      if(optionRegisterSettings.test(IMR)) {  
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

      // throw if both IMR and IER are there
      if(optionRegisterSettings.test(IMR) and optionRegisterSettings.test(IER)) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only IER and IMR cannot not both be set.");
      }

      // throw if more than one entry of [MIE, GIE, MER] is there (test all combinations)
      int nMieGieMer =
          ((int)optionRegisterSettings.test(MIE)) + optionRegisterSettings.test(GIE) + optionRegisterSettings.test(MER);
      if(nMieGieMer > 1) {
        throw ChimeraTK::logic_error("Invalid register " + jdv1.OPTIONS_JSON_KEY +
            " combination specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) + ": Only one out of MIE, GIE, and MER can be set; " + std::to_string(nMieGieMer) +
            " are set.");
      }

      if(optionRegisterSettings.test(IVEAR) or optionRegisterSettings.test(IVAR) or optionRegisterSettings.test(IVR) or
          optionRegisterSettings.test(ILR)) {
        throw ChimeraTK::logic_error("Forbidden register " + jdv1.OPTIONS_JSON_KEY +
            " specified in map file json descriptor for GenericInterruptControllerHandler " +
            controllerID2Str(controllerID) +
            ": While ILR, IVR, IVAR, and IVEAR are defined options in the Axi_intc register space, they are not "
            "currently allowed options the GenericInterruptControllerHandler");
      }

      assert(optionRegisterSettings.test(ISR)); // should never be false
    }                                           // steriliseOptionRegisterSettings

  } // namespace genIntC

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  using namespace ChimeraTK::genIntC;

  /******************************constructor************************************************************************/

  GenericInterruptControllerHandler::GenericInterruptControllerHandler(
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      boost::shared_ptr<TriggerDistributor> parent, std::string registerPath,
      std::bitset<optionCode::OPTION_CODE_COUNT> optionRegisterSettings)
  : InterruptControllerHandler(controllerHandlerFactory, controllerID, std::move(parent)), _path(registerPath.c_str()) {
    // Set required registers
    optionRegisterSettings.set(ISR); // Ensure that the required option ISR is always set.
    if(not(optionRegisterSettings.test(IMR) or optionRegisterSettings.test(IER))) { // Ensure IMR or IER is on
      optionRegisterSettings.set(IER);
    }
    //Here we could explicitly note we're ignoring IPR with optionRegisterSettings.reset(IPR)

    steriliseOptionRegisterSettings(optionRegisterSettings, controllerID);

    // Setup Register Accessors with logic
    _isr = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(ISR), 1, 0, {});

    _ierIsReallyImr = optionRegisterSettings.test(IMR);
    _ier = _backend->getRegisterAccessor<uint32_t>(_path / (_ierIsReallyImr ? IMR : IER), 1, 0, {});

    _icrIsReallyIar = optionRegisterSettings.test(IAR);
    _icr = _backend->getRegisterAccessor<uint32_t>(_path / (_icrIsReallyIar ? IAR : ICR), 1, 0, {});

    _optionMerMieGie = optionRegisterSettings.test(MIE) ? MIE : (optionRegisterSettings.test(GIE) ? GIE : MER);
    _mer = _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(_optionMerMieGie), 1, 0, {});

    _haveSieAndCie = optionRegisterSettings.test(SIE); // and/or optionRegisterSettings.test(CIE);
    // Should always either have SIE and CIE or neither, ensured by steriliseOptionRegisterSettings.

    if(_haveSieAndCie) {
      _sie= _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(SIE), 1, 0, {});
      _cie= _backend->getRegisterAccessor<uint32_t>(_path / getOptionRegisterStr(CIE), 1, 0, {});
    }

    // Check readability
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

    // Initialise register content
    //- [x] set MER = 3 if it exists.
    if(optionRegisterSettings[MER]) {
      _mer->accessData(0) = 3;
      _mer->write();
    }
    
    // - [x] Make use of the parsed result
    // what variation do we expect in this signature? Always dummy? Always 0th element.second["module"]?

    //- [x] set MER = 3 if it exists.
    // set class member copy of IER as a local copy

    // main registers to set: ISR, IER
    // optional registers:
    // ICR or IAR     interrupt clear register (acknowledge IAR), if present ISR can be cleared over this register,
    // usually atomic write (write one to clear bit), if not present then only over ISR write IPR     interrupt pending
    // register, this provides value of (IRS & IER) so that SW can get this value with no need to access two registers
    // and do & operation MIE or GIE     main/global interrupt enable, set all bits to 1 to enable globally HW
    // interrupts SIE     set interrupt enable register, usually atomic write 1 to set IER bit CIE     clear interrupt
    // enable register, usually atomic write 1 to clear IER bit

    // Note ILR: priority threshold to block low priority interrupts to support nested interrupt handling.
  }

  /******************************destructor*************************************************************************/

  GenericInterruptControllerHandler::~GenericInterruptControllerHandler() {
    // TODO: Clear the enabled interrupts in the destructor.
  }

  /******************************handle*****************************************************************************/

  void GenericInterruptControllerHandler::handle(VersionNumber version) {
    /* When a trigger comes in, InterruptControllerHandler.handle gets called on the interrupt*/
    // Stupid testing implementation that always triggers all children
    for(auto& distributorIter : _distributors) {
      auto distributor = distributorIter.second.lock();
      // The weak pointer might have gone.
      // FIXME: We need a cleanup function which removes the map entry. Otherwise we might
      // be stuck with a bad weak pointer which is tried in each handle() call.
      if(distributor) {
        distributor->distribute(nullptr, version);
      }
    }
  } //handle

    /*
      void GenericInterruptControllerHandler::handle(VersionNumber version) {
    // The interrupt handling functions implements the handshake with the interrupt controller.
    // It needs to be implemented individually for each interrupt controller.

  try {
    _activeInterrupts->read(); //retain 
    for(uint32_t i = 0; i< 32; ++i) {
      if(_activeInterrupts->accessData(0) & 0x1U << i) { //retain mask pattern
        try {
          boost::weak_ptr<TriggerDistributor> distributor = _distributors.at(i).lock(); //retain
          if(distributor) {
            distributor->distribute(nullptr, version); //retain
          }
        }
        catch(std::out_of_range&){
          _backend->setException("Error: GenIntC reports unknown active interrupt "+ std::to_string(i));
        }
      }
    }
  }
  catch(ChimeraTK::runtime_error&) {
    //Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
  }
}
    */

  /******************************create*****************************************************************************/

  std::unique_ptr<GenericInterruptControllerHandler> GenericInterruptControllerHandler::create(
      InterruptControllerHandlerFactory* controllerHandlerFactory, std::vector<uint32_t> const& controllerID,
      [[maybe_unused]] std::string const& description, // THE JSON SNIPPET
      boost::shared_ptr<TriggerDistributor> parent) {
    // A factory to creates and returns an initalized GenericInterruptControllerHandler

    std::string registerPath;
    std::bitset<OPTION_CODE_COUNT> optionRegisterSettings =
        parseAndValidateJsonDescriptionV1(controllerID, description, registerPath);

    return std::make_unique<GenericInterruptControllerHandler>(
        controllerHandlerFactory, controllerID, std::move(parent), registerPath, optionRegisterSettings);
  }

} // namespace ChimeraTK

// NOTES
/*
//ICR is interrupt clear register.  if present.
//after interrupt is distributed, write activeInterrupts->accessData & mask into ICR to clear
it.
 if no ICR: do same thing into ISR.

 -[x]  create the option class members,
 -[x] have to remember whether the option is present.
 -[x]  set default constructor in the constructor initializer list
*/