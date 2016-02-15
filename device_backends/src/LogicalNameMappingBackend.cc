/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include "LogicalNameMappingBackend.h"
#include "LogicalNameMappingBackendRangeRegisterAccessor.h"
#include "LogicalNameMappingBackendBufferingRangeRegisterAccessor.h"
#include "Device.h"

namespace mtca4u {

  LogicalNameMappingBackend::LogicalNameMappingBackend(std::string lmapFileName)
  : _lmapFileName(lmapFileName), _map(lmapFileName)
  {
    // create all devices referenced in the map
    auto devNames = _map.getTargetDevices();
    for(auto devName = devNames.begin(); devName != devNames.end(); ++devName) {
      _devices[*devName] = boost::shared_ptr<Device>(new Device());
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open()
  {
    // open all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      device->second->open(device->first);
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::close()
  {
    // close all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      device->second->close();
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> LogicalNameMappingBackend::createInstance(std::string /*host*/,
      std::string /*instance*/, std::list<std::string> /*parameters*/, std::string mapFileName) {
    return boost::shared_ptr<DeviceBackend>(new LogicalNameMappingBackend(mapFileName));
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::read(const std::string &regModule, const std::string &regName,
      int32_t *data, size_t dataSize, uint32_t addRegOffset) {

    // obtain register info
    std::string name = ( regModule.length() > 0 ? regModule + "." + regName : regName );
    LogicalNameMap::RegisterInfo info;
    info = _map.getRegisterInfo(name);

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info.hasDeviceName()) targetDevice = _devices[info.deviceName];

    // implementation for each type
    if(info.targetType == LogicalNameMap::TargetType::REGISTER) {
      targetDevice->readReg(info.registerName,data,dataSize,addRegOffset);
    }
    else if(info.targetType == LogicalNameMap::TargetType::RANGE) {
      targetDevice->readReg(info.registerName,data,dataSize,addRegOffset + sizeof(int32_t)*info.firstIndex);
    }
    else if(info.targetType == LogicalNameMap::TargetType::CHANNEL) {
      throw DeviceException("Reading a channel of a multiplexed register is only supported using register accessors.",
          DeviceException::NOT_IMPLEMENTED);
    }
    else if(info.targetType == LogicalNameMap::TargetType::INT_CONSTANT) {
      if(dataSize >= 4 && addRegOffset == 0) {
        data[0] = info.value;
      }
    }

  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::write(const std::string &regModule, const std::string &regName,
      int32_t const *data, size_t dataSize, uint32_t addRegOffset) {

    // obtain register info
    std::string name = ( regModule.length() > 0 ? regModule + "." + regName : regName );
    LogicalNameMap::RegisterInfo info;
    info = _map.getRegisterInfo(name);

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info.hasDeviceName()) targetDevice = _devices[info.deviceName];

    // implementation for each type
    if(info.targetType == LogicalNameMap::TargetType::REGISTER) {
      targetDevice->writeReg(info.registerName,data,dataSize,addRegOffset);
    }
    else if(info.targetType == LogicalNameMap::TargetType::RANGE) {
      targetDevice->writeReg(info.registerName,data,dataSize,addRegOffset + sizeof(int32_t)*info.firstIndex);
    }
    else if(info.targetType == LogicalNameMap::TargetType::CHANNEL) {
      throw DeviceException("Writing a channel of a multiplexed register is only supported using register accessors.",
          DeviceException::NOT_IMPLEMENTED);
    }

  }

  /********************************************************************************************************************/

  boost::shared_ptr<mtca4u::RegisterAccessor> LogicalNameMappingBackend::getRegisterAccessor(
      const std::string &registerName, const std::string &module) {

    // obtain register info
    std::string name = ( module.length() > 0 ? module + "." + registerName : registerName );
    LogicalNameMap::RegisterInfo info;
    info = _map.getRegisterInfo(name);

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info.hasDeviceName()) targetDevice = _devices[info.deviceName];

    // implementation for each type
    if(info.targetType == LogicalNameMap::TargetType::REGISTER) {
      return targetDevice->getRegisterAccessor(info.registerName);
    }
    else if(info.targetType == LogicalNameMap::TargetType::RANGE) {
      return boost::shared_ptr<mtca4u::RegisterAccessor>(new LogicalNameMappingBackendRangeRegisterAccessor(
          targetDevice->getRegisterAccessor(info.registerName), info.firstIndex, info.length));
    }
    else {
      throw DeviceException("For this register type, a RegisterAccessor cannot be obtained (name of logical register: "+
          name+").", DeviceException::NOT_IMPLEMENTED);
    }

  }

  /********************************************************************************************************************/

  template<typename UserType>
  TwoDRegisterAccessorImpl<UserType>* LogicalNameMappingBackend::getTwoDRegisterAccessor(
      const std::string &module, const std::string &registerName) {
    (void)module; (void)registerName; // avoid warning
    throw DeviceException("2D register accessors not supported for LogicalNameMappingBackends.",
        DeviceException::NOT_IMPLEMENTED);
  }

  VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER(LogicalNameMappingBackend, getTwoDRegisterAccessorImpl,
      getTwoDRegisterAccessor, const std::string &, const std::string &)

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessorImpl<UserType>* LogicalNameMappingBackend::getBufferingRegisterAccessor(
      const std::string &module, const std::string &registerName) {

    // obtain register info
    std::string name = ( module.length() > 0 ? module + "." + registerName : registerName );
    LogicalNameMap::RegisterInfo info;
    info = _map.getRegisterInfo(name);

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info.hasDeviceName()) targetDevice = _devices[info.deviceName];

    // implementation for each type
    if(info.targetType == LogicalNameMap::TargetType::REGISTER) {
      return new LogicalNameMappingBackendBufferingRangeRegisterAccessor<UserType>(shared_from_this(), module, registerName);
    }
    else if(info.targetType == LogicalNameMap::TargetType::RANGE) {
      return new LogicalNameMappingBackendBufferingRangeRegisterAccessor<UserType>(shared_from_this(), module, registerName);
    }
    else {
      throw DeviceException("For this register type, a RegisterAccessor cannot be obtained (name of logical register: "+
          name+").", DeviceException::NOT_IMPLEMENTED);
    }
  }

  VIRTUAL_FUNCTION_TEMPLATE_IMPLEMENTER(LogicalNameMappingBackend, getBufferingRegisterAccessorImpl,
      getBufferingRegisterAccessor, const std::string &, const std::string &)


} // namespace mtca4u
