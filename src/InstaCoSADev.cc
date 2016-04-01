/*
 * InstaCoSADev.cc
 *
 *  Created on: Mar 29, 2016
 *      Author: Martin Hierholzer
 */

#include <map>
#include <string>

#include <mtca4u/Device.h>
#include <ControlSystemAdapter/ProcessScalar.h>
#include <ControlSystemAdapter/ProcessArray.h>

#include "InstaCoSADev.h"

namespace mtca4u {

  /*******************************************************************************************************************/

  class InstaCoSADev_impl {
    // only allow access by InstaCoSADev
    private:
      friend class InstaCoSADev;

      InstaCoSADev_impl() {};

      // pointer to the process variable manager
      boost::shared_ptr<mtca4u::DevicePVManager> processVariableManager;

      // map for double-typed scalars
      std::map< std::string, std::pair< ScalarRegisterAccessor<double>, boost::shared_ptr< ProcessScalar<double> > > > scalarDoubleMap;

      // map for int-typed scalars
      std::map< std::string, std::pair< ScalarRegisterAccessor<int>, boost::shared_ptr< ProcessScalar<int> > > > scalarIntMap;

  };

  /*******************************************************************************************************************/

  InstaCoSADev::InstaCoSADev(boost::shared_ptr<mtca4u::DevicePVManager> const &processVariableManager) {
    impl.reset(new InstaCoSADev_impl());
    impl->processVariableManager = processVariableManager;
  }

  /*******************************************************************************************************************/

  void InstaCoSADev::addModule(mtca4u::Device &device, const mtca4u::RegisterPath &module, const std::string &pvBaseName) {
    auto catalogue = device.getRegisterCatalogue();
    for(auto &reg : catalogue) {
      mtca4u::RegisterPath regName = reg.getRegisterName();
      if(!regName.startsWith(std::string(module)+"/")) continue;  // wrong module
      std::string baseName = std::string(regName).substr(module.length());
      std::string pvName = std::string(mtca4u::RegisterPath(pvBaseName) / baseName).substr(1);
      if(reg.getNumberOfDimensions() == 0) {
        ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>(reg.getRegisterName());
        boost::shared_ptr< ProcessScalar<int> > pv = impl->processVariableManager->createProcessScalar<int>(mtca4u::deviceToControlSystem, pvName);
        impl->scalarIntMap[baseName] = std::make_pair(accessor,pv);
      }
      else if(reg.getNumberOfDimensions() == 1) {
        throw std::string("not yet implemented.");
      }
      else {
        throw std::string("2D register found in module "+module+" but not supported by InstaCoSADev."); // TODO @todo replace by proper exception
      }
    }
  }

  /*******************************************************************************************************************/

  void InstaCoSADev::transferData(mtca4u::SynchronizationDirection direction) {
    for(auto &entry : impl->scalarIntMap) {
      auto &accessor = entry.second.first;
      auto &pv = entry.second.second;
      if(direction == mtca4u::deviceToControlSystem && pv->isSender()) {        // on the device side, a sender variable is device -> control system
        accessor.read();
        *pv = int(accessor);
      }
      else if(direction == mtca4u::controlSystemToDevice && pv->isReceiver()) {
        accessor = int(*pv);
        accessor.write();
      }
    }
  }

} /* namespace mtca4u */
