// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"
#include "NDRegisterAccessor.h"
#include "RegisterPath.h"

#include <nlohmann/json.hpp>
#include <boost/bimap.hpp>

#include <fstream>
using json = nlohmann::json;

namespace ChimeraTK {
  namespace genIntC {

    struct jsonDescriptorStandardV1 {
      static constexpr int jsonDescriptorVersion = 1;
      inline static const std::string VERSION_JSON_KEY = "version";
      inline static const std::string OPTIONS_JSON_KEY = "options";
      inline static const std::string PATH_JSON_KEY = "path";
    };

    enum optionCode { 
      ISR = 0, //Interrupt Status Register
      IER, //Interrupt Enable Register
      MER, //Master Interrupt Register,
      MIE, //Master Interrupt Enable  Functionally equivalent to MER
      GIE, //Global Interrupt Enable, Functionally equivalent to MER
      ICR, //Interrupt Clear Register
      IAR, //Interrupt Acknowledge Register
      IPR, //Interrupt Pending Register =ISR & IER, a convenience feature for software. 
      SIE, //Set Interrupt Enable
      CIE, //Slear Interrupt Enable
      IMaskR, //Interrupt Mask Register. IMR Acronym Collision 
      IModeR, //Interrupt Mode Register. IMR Acronym Collision, defined in the standard but not allowed
      IVR,    // defined in the standard but not allowed
      ILR,    // defined in the standard but not allowed
      IVAR,   // defined in the standard but not allowed
      IVEAR,  // defined in the standard but not allowed
      OPTION_CODE_COUNT, // used for counting how many valid enums there are here & setting list lengths
      INVALID_OPTION_CODE,
    };

    optionCode getOptionRegisterEnum(std::string opt);
    std::string getOptionRegisterStr(optionCode optCode);

    std::string strSet2Str(const std::set<std::string>& strSet, char delimiter = ',');
    std::string intVec2Str(const std::vector<uint32_t>& intVec, char delimiter = ',');
    std::string controllerID2Str(const std::vector<uint32_t>& controllerID);
    inline uint32_t i2Mask(const uint32_t ithInterrupt);

    std::bitset<OPTION_CODE_COUNT> parseAndValidateJsonDescriptionV1(std::vector<uint32_t> const& controllerID,
        std::string const& description, // THE JSON SNIPPET
        std::string& registerPath);     // a return value

    void steriliseOptionRegisterSettings(
        std::bitset<OPTION_CODE_COUNT> const& optionRegisterSettings, std::vector<uint32_t> const& controllerID);
  } // namespace genIntC

  /********************************************************************************************************************/
  //    /$$$$$$                      /$$$$$$             /$$      /$$$$$$
  //   /$$__  $$                    |_  $$_/            | $$     /$$__  $$
  //  | $$  \__/  /$$$$$$  /$$$$$$$   | $$   /$$$$$$$  /$$$$$$  | $$  \__/
  //  | $$ /$$$$ /$$__  $$| $$__  $$  | $$  | $$__  $$|_  $$_/  | $$
  //  | $$|_  $$| $$$$$$$$| $$  \ $$  | $$  | $$  \ $$  | $$    | $$
  //  | $$  \ $$| $$_____/| $$  | $$  | $$  | $$  | $$  | $$ /$$| $$    $$
  //  |  $$$$$$/|  $$$$$$$| $$  | $$ /$$$$$$| $$  | $$  |  $$$$/|  $$$$$$/
  //   \______/  \_______/|__/  |__/|______/|__/  |__/   \___/   \______/
  /********************************************************************************************************************/
  class GenericInterruptControllerHandler : public InterruptControllerHandler {
   public:
    explicit GenericInterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, 
        boost::shared_ptr<TriggerDistributor> parent,
        std::string registerPath, 
        std::bitset<(ulong)genIntC::optionCode::OPTION_CODE_COUNT> optionRegisterSettings);
    ~GenericInterruptControllerHandler() override;

    void handle(VersionNumber version) override;
    void activate(VersionNumber version);
    static std::unique_ptr<GenericInterruptControllerHandler> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID,
        std::string const& description, // THE JSON SNIPPET
        boost::shared_ptr<TriggerDistributor> parent);

   protected:
    bool _ierIsReallyImaskr; 
    bool _haveSieAndCie; 
    bool _hasMer;
    uint32_t _activeInterrupts{0}; // like a local copy of IER

    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _isr;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _ier; // May point to IER or Imaskr 
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _icr; // May point to ICR, IAR, or ISR, which act identically
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _mer; // May point to MER, MIE, or GIE, all act identically. We can have at most 1 of {MIE, GIE, MER}
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _sie; // We either have both SIE or CIE or neither.
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _cie; 

    RegisterPath _path; // just a string path, with the added overwritten / operator etc.

    void _clearInterruptsFromMask(uint32_t mask);
    inline void _clearOneInterrupt(uint32_t ithInterrupt);
    inline void _clearAllInterrupts();
    inline void _clearAllEnabledInterrupts();

    void _disableInterruptsFromMask(uint32_t mask);
    inline void _disableOneInterrupt(uint32_t ithInterrupt);

    void _enableInterruptsFromMask(uint32_t mask);
    inline void _enableOneInterrupt(uint32_t ithInterrupt);
  };
  /********************************************************************************************************************/

} // namespace ChimeraTK
/**********************************************************************************************************************/
/**********************************************************************************************************************/


// https://redmine.msktools.desy.de/issues/12890
