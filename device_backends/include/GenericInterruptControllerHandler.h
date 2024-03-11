// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "InterruptControllerHandler.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ChimeraTK {

  /*https://redmine.msktools.desy.de/issues/12890
  - Only enable those interrupts for which there are accessors or nested controllers (i.e. there is a valid interrupt distributor)
  - Clear the enabled interrupts in the destructor.
  - IMPORTANT: Nested interrupt handlers must clear their active interrupt flag first, then the parent interrupt flags are cleared.
  - Suppress all the runtime_error exceptions from internal accessors. 
      The interrupt controller handler is called from the internal interrupt handler thread,
      and the accessor is calling setException() of the backend already.
  - IMPORTANT: keep a local copy of the IER. 
      This only contains the bits for the accessors of THIS software instance. 
      There might be another process which is accessing other interrupts on the same (primary) controller.
  */

  class GenericInterruptControllerHandler : public InterruptControllerHandler {
   public:
    explicit GenericInterruptControllerHandler(InterruptControllerHandlerFactory* controllerHandlerFactory,
        std::vector<uint32_t> const& controllerID, 
        boost::shared_ptr<TriggerDistributor> parent)
    : InterruptControllerHandler(controllerHandlerFactory, controllerID, std::move(parent)) {
      //identify json snippet string
      //presume you have a the map file string in mapJsonStr
      std::ifstream mapJsonFstream(mapJsonStr);
      json mapJson = json::parse(mapJsonFstream);
      //expect mapJsonStr to resemble '{"dummy":{"module":"/int_ctrls/controller4_8"} }'
      //or '{"dummy":{"module":"/int_ctrls/controller5"} }'
      //and the useful information are the 4 and 8
      //how deep does that go? I see INTERRUPT4:8:2, so maybe this can go 3 levels deep
      //Make use of the parsed result
      //what variation do we expect in this signature? Always dummy? Always 0th element.second["module"]?

      //set MER = 3 if it exists.
      //set class member copy of IER as a local copy

      //main registers to set: ISR, IER
      //optional registers: 
      //ICR or IAR     interrupt clear register (acknowledge IAR), if present ISR can be cleared over this register, usually atomic write (write one to clear bit), if not present then only over ISR write
      //IPR     interrupt pending register, this provides value of (IRS & IER) so that SW can get this value with no need to access two registers and do & operation
      //MIE or GIE     main/global interrupt enable, set all bits to 1 to enable globally HW interrupts
      //SIE     set interrupt enable register, usually atomic write 1 to set IER bit
      //CIE     clear interrupt enable register, usually atomic write 1 to clear IER bit

      //Note ILR: priority threshold to block low priority interupts to support nested interrupt handeling. 
    }
    ~GenericInterruptControllerHandler() override = default;

    void handle(VersionNumber version) override;
    /* When a trigger comes in, InterruptControllerHandler.handle gets called on the interrupt*/

    static std::unique_ptr<Axi4_Intc> create(InterruptControllerHandlerFactory*,
        std::vector<uint32_t> const& controllerID, 
        std::string const& desrciption, //THE JSON SNIPPET
        boost::shared_ptr<TriggerDistributor> parent);
        /*
        knows the signature of the constructor. 
        a factory. it creates interuptcontrollerhandlers.

        //parse in creator. check version. 
        */

    private:
    //local copy of IER

    protected:
      boost::shared_ptr<NDRegisterAccessor<uint32_t>> _activeInterrupts;
      RegisterPath _module;
  };

/*********************************************************************************************************************/

  void GenericInterruptControllerHandler::handle(VersionNumber version) {
    /** The interrupt handling functions implements the handshake with the interrupt controller. 
     * It needs to be implemented individually for each interrupt controller.
     */
  try {
    _activeInterrupts->read(); //retain #TODO what is this?
    for(uint32_t i = 0; i< 32; ++i) {
      if(_activeInterrupts->accessData(0) & 0x1U << i) { //retain mask pattern
        try {
          boost::weak_ptr<TriggerDistributor> distributor = _distributors.at(i).lock(); //retain
          if(distributor) {
            distributor->distribute(nullptr, version); //retain
          }
        }
        catch(std::out-of_range&){
          _backend->setException("Error: DUmmyIntc reports unknown activit interrupt "+ std::to_string(i));
        }
      }
    }
  }
  catch(ChimeraTK::runtime_error&) {
    //Nothing to do. The transferElement part of _activeInterrupts has already called the backend's setException
  }
}


} // namespace ChimeraTK
