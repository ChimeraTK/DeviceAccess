/*
 * VariableNetwork.cc
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 */

#include "VariableNetwork.h"
#include "Accessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  bool VariableNetwork::hasAccessor(AccessorBase *a, AccessorBase *b) const {
    if(a == outputAccessor || (b != nullptr && b == outputAccessor) ) return true;

    // search for a and b in the inputAccessorList
    size_t n = count_if(inputAccessorList.begin(), inputAccessorList.end(),
        [a,b](const AccessorBase* e) { return a == e || ( b != nullptr && b == e ); });

    if(n > 0) return true;
    return false;
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasOutputAccessor() const {
    if(outputAccessor != nullptr) return true;
    if(publicCS2DevName.length() > 0) return true;
    if(deviceReadRegisterInfo.deviceAlias.length() > 0) return true;
    return false;
  }

  /*********************************************************************************************************************/

  size_t VariableNetwork::countInputAccessors() const {
    size_t count = 0;
    count += inputAccessorList.size();
    count += publicDev2CSNames.size();
    count += deviceWriteRegisterInfos.size();
    return count;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addAccessor(AccessorBase &a) {
    if(hasAccessor(&a)) return;  // already in the network
    if(a.isOutput()) {
      if(hasOutputAccessor()) {
        throw std::string("Trying to add an output accessor to a network already having an output accessor.");  // @todo TODO throw proper exception
      }
      outputAccessor = &a;
      valueType = &(a.getValueType());
    }
    else {
      inputAccessorList.push_back(&a);
    }
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addCS2DevPublication(AccessorBase &a, const std::string& name) {
    if(hasOutputAccessor()) {
      throw std::string("Trying to add control-system-to-device publication to a network already having an output accessor.");  // @todo TODO throw proper exception
    }
    publicCS2DevName = name;
    valueType = &(a.getValueType());
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addInputDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
      UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
    DeviceRegisterInfo info;
    info.deviceAlias = deviceAlias;
    info.registerName = registerName;
    info.mode = mode;
    deviceWriteRegisterInfos.push_back(info);
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addOutputDeviceRegister(const std::string &deviceAlias, const std::string &registerName,
      UpdateMode mode, size_t numberOfElements, size_t elementOffsetInRegister) {
    if(hasOutputAccessor()) {
      throw std::string("Trying to add control-system-to-device publication to a network already having an output accessor.");  // @todo TODO throw proper exception
    }
    deviceReadRegisterInfo.deviceAlias = deviceAlias;
    deviceReadRegisterInfo.registerName = registerName;
    deviceReadRegisterInfo.mode = mode;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::addDev2CSPublication(const std::string& name) {
    publicDev2CSNames.push_back(name);
  }

  /*********************************************************************************************************************/

  bool VariableNetwork::hasInputAccessorType(VariableNetwork::AccessorType type) const {
    if(type == AccessorType::Device) {
      if(deviceWriteRegisterInfos.size() > 0) return true;
    }
    else if(type == AccessorType::ControlSystem) {
      if(publicDev2CSNames.size() > 0) return true;
    }
    else if(type == AccessorType::Application) {
      if(inputAccessorList.size() > 0) return true;
    }
    return false;
  }

  /*********************************************************************************************************************/

  VariableNetwork::AccessorType VariableNetwork::getOutputAccessorType() const {
    if(outputAccessor != nullptr) return AccessorType::Application;
    if(publicCS2DevName.length() > 0) return AccessorType::ControlSystem;
    if(deviceReadRegisterInfo.deviceAlias.length() > 0) return AccessorType::Device;
    return AccessorType::Undefined;
  }

  /*********************************************************************************************************************/

  void VariableNetwork::dump() const {
    std::cout << "VariableNetwork {" << std::endl;
    auto type = getOutputAccessorType();
    if(type == AccessorType::Application) std::cout << "  outputType = Application" << std::endl;
    if(type == AccessorType::ControlSystem) std::cout << "  outputType = ControlSystem ('" << publicCS2DevName << "')" << std::endl;
    if(type == AccessorType::Device) std::cout << "  outputType = Device (" << deviceReadRegisterInfo.deviceAlias << ": "
        << deviceReadRegisterInfo.registerName << ")" << std::endl;
    std::cout << "  Application inputs: " << inputAccessorList.size() << std::endl;
    std::cout << "  ControlSystem inputs: " << publicDev2CSNames.size() << std::endl;
    std::cout << "  Device inputs: " << deviceWriteRegisterInfos.size() << std::endl;
    std::cout << "}" << std::endl;
  }
}
