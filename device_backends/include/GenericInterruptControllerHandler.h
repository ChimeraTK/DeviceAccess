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

  /*https://redmine.msktools.desy.de/issues/12890
  - Only enable those interrupts for which there are accessors or nested controllers (i.e. there is a valid interrupt
  distributor)
  - Clear the enabled interrupts in the destructor.
  - IMPORTANT: Nested interrupt handlers must clear their active interrupt flag first, then the parent interrupt flags
  are cleared.
  - Suppress all the runtime_error exceptions from internal accessors.
      The interrupt controller handler is called from the internal interrupt handler thread,
      and the accessor is calling setException() of the backend already.
  - IMPORTANT: keep a local copy of the IER.
      This only contains the bits for the accessors of THIS software instance.
      There might be another process which is accessing other interrupts on the same (primary) controller.
  */

  namespace genIntC {

    struct jsonDescriptorStandardV1 {
      static constexpr int jsonDescriptorVersion = 1;
      inline static const std::string VERSION_JSON_KEY = "version";
      inline static const std::string OPTIONS_JSON_KEY = "options";
      inline static const std::string PATH_JSON_KEY = "path";
    };

    enum optionCode {
      MER = 0, // Master Interrupt Enable,
      MIE,
      GIE, // Global Interrupt Enable 
      // json name in the snippet names the register as specified in the map file
      // but mer/mie/gie are functionally equivalent. internally, pick one.
      IER,
      ICR,
      SIE,
      ISR,
      IPR,
      IMR,
      CIE,
      IAR,
      IVR,
      ILR,
      IVAR,
      IVEAR,             // defined in the standard but not allowed
      OPTION_CODE_COUNT, // used for counting how many valid enums there are here & setting list lengths
      INVALID_OPTION_CODE,
    };

    template<typename L, typename R>
    boost::bimap<L, R> makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
      return boost::bimap<L, R>(list.begin(), list.end());
    }

    auto optionCodeMap = makeBimap<std::string, optionCode>({{"MER", MER}, {"MIE", MIE}, {"GIE", GIE}, {"ISR", ISR},
        {"IER", IER}, {"ICR", ICR}, {"SIE", SIE},

        {"IPR", IPR}, {"IMR", IMR}, {"CIE", CIE}, {"IAR", IAR},

        {"ILR", ILR}, {"IVR", IVR}, {"IVAR", IVAR}, {"IVEAR", IVEAR}});

    optionCode getOptionRegisterEnum(std::string opt);
    std::string getOptionRegisterStr(optionCode optCode);

    std::string strSet2Str(const std::set<std::string>& strSet, char delimiter = ',');
    std::string intVec2Str(const std::vector<uint32_t>& intVec, char delimiter = ',');
    std::string controllerID2Str(const std::vector<uint32_t>& controllerID);

    std::bitset<OPTION_CODE_COUNT> parseAndValidateJsonDescriptionV1(std::vector<uint32_t> const& controllerID,
        std::string const& description, // THE JSON SNIPPET
        std::string& registerPath);     // a return value

    void steriliseOptionRegisterSettings(
        std::bitset<OPTION_CODE_COUNT> const& optionRegisterSettings, std::vector<uint32_t> const& controllerID);
  } // namespace genIntC

  using namespace genIntC;

  class GenericInterruptControllerHandler : public InterruptControllerHandler {
   public:
    explicit GenericInterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, boost::shared_ptr<TriggerDistributor> parent,
        std::string registerPath, std::bitset<(ulong)optionCode::OPTION_CODE_COUNT> optionRegisterSettings);
    ~GenericInterruptControllerHandler() override;

    void handle(VersionNumber version) override;
    /* When a trigger comes in, InterruptControllerHandler.handle gets called on the interrupt*/

    static std::unique_ptr<GenericInterruptControllerHandler> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID,
        std::string const& description, // THE JSON SNIPPET
        boost::shared_ptr<TriggerDistributor> parent);
    /*
    knows the signature of the constructor.
    a factory. it creates interuptcontrollerhandlers.

    //parse in creator. check version.
    */

   protected:
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _isr;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _ier; // or Imr //require either ier or imr
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _icr; // or iar
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _mer; // or MIE or GIE, all act identically. can have at most 1 of {MIE, GIE, MER}
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _sie; // we either have both SIE or CIE or neither.
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> _cie; //

    bool _ierIsReallyImr;
    bool _icrIsReallyIar;
    bool _haveSieAndCie;
    optionCode _optionMerMieGie;

    uint32_t _activeInterrupts; // a local copy of IER

    RegisterPath _path; // just a string path, with the added overwritten / operator etc.
  };

} // namespace ChimeraTK