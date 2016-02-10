/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include "LogicalNameMappingBackend.h"
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

} // namespace mtca4u


