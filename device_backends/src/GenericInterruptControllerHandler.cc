// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "GenericInterruptControllerHandler.h"
#include <nlohmann/json.hpp>
#include "TriggerDistributor.h"

namespace ChimeraTK {

  void GenericInterruptControllerHandler::handle(VersionNumber version) {
    // Stupid testing implementation that always triggers all children
    for(auto& distributorIter : _distributors) {
      auto distributor = distributorIter.second.lock();
      // The weak pointer might have gone.
      // FIXME: We need a cleanup function which removes the map entry. Otherwise we might
      // be stuck with a bad weak pointer wich is tried in each handle() call.
      if(distributor) {
        distributor->distribute(nullptr, version);
      }
    }
  }

  /*****************************************************************************************************************/

  std::unique_ptr<GenericInterruptControllerHandler> GenericInterruptControllerHandler::create(InterruptControllerHandlerFactory* controllerHandlerFactory,
      std::vector<uint32_t> const& controllerID, 
      [[maybe_unused]] std::string const& desrciption,
      boost::shared_ptr<TriggerDistributor> parent) {

    // Expect description of the form  {"path":"APP.INTCB", "options":{"ICR", "IPR", "MER"}, "version":1}
    try {
      nlohmann::json jsonDescription = nlohmann::json::parse(desrciption);

	    //only keys path, option, version.  if any other key, throw. 
      for (auto& el : jsonDescription.items()) {
        if( el.key() != "path" && el.key() != "options" && el.key() != "version"){
          throw; //TODO
        }
      }

      std::string registerPath;
      jsonDescription["path"].get_to(registerPath);
      // const std::string path = jsonDescription.at("path");
      // path is a register path -- a path to the register within the map file

      if( jsonDescription.value("version", 1) != 1 ){ 
        //if version != 1, throw. 
        //if no version, assume 1
        // can throw basic__json::type_error
        throw; //TODO
      }

      static const std::vector<std::string> defaultOptionRegisterNames = {"IER",""};
      std::vector<std::string> optionRegisterNames = jsonDescription.value("options", defaultOptionRegisterNames);
      jsonDescription["options"].get_to(optionRegisterNames);
      std::vector<std::string> optionRegisterNames;
		  //ignore IPR, no exception if it's there
      //if unknown option, throw

    }
    catch(nlohmann::json::parse_error& ex) {
      //parse error thrown by parse(description) 
    }
    catch(nlohmann::basic__json::type_error){
      //incorrect type given in json, either by value(version) or value(option)
    }
    catch(nlohmann::json.exception.type_error.302){
      //json value doesn't match default type for value(version) or value(option)
    }
    catch(nlohmann::json.exception.type_error.306){
      //json value doesn't match default type for value(version) or value(option)
    }
      //auto interruptId = jkey.get<std::vector<uint32_t>>();

      // https://json.nlohmann.me/features/element_access/

      /*
			//ICR is interupt clear register.  if present. 
				//after inteurp is distributed, write activeInterupts->accessData & mask into ICR to clear it. 
				if no ICR: do same thing into ISR. 

			

		if !path present, throw
		option is optionsl, if no option, assume IER and other required option. 
		
		version: version of this json descriptor.

*/

    return std::make_unique<Axi4_Intc>(controllerHandlerFactory, controllerID, std::move(parent));
  }

} // namespace ChimeraTK
