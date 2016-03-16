/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include "LNMBackendBufferingRegisterAccessor.h"
#include "LNMBackendBufferingChannelAccessor.h"
#include "LNMBackendBufferingVariableAccessor.h"
#include "LogicalNameMappingBackend.h"
#include "Device.h"

namespace mtca4u {

  LogicalNameMappingBackend::LogicalNameMappingBackend(std::string lmapFileName)
  : hasParsed(false), _lmapFileName(lmapFileName)
  {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getBufferingRegisterAccessor_impl);
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getTwoDRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::parse() {

    // don't run, if already parsed
    if(hasParsed) return;

    // parse the map fle
    LogicalNameMap map = LogicalNameMap(_lmapFileName);
    _catalogue = map._catalogue;

    // create all devices referenced in the map
    auto devNames = map.getTargetDevices();
    for(auto devName = devNames.begin(); devName != devNames.end(); ++devName) {
      _devices[*devName] = boost::shared_ptr<Device>(new Device());
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open()
  {
    parse();
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
    RegisterPath name = RegisterPath(regModule)/regName;
    auto info = boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_catalogue.getRegister(name));

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info->hasDeviceName()) targetDevice = _devices[info->deviceName];

    // implementation for each type
    if(info->targetType == LogicalNameMap::TargetType::REGISTER) {
      targetDevice->readReg(info->registerName,data,dataSize,addRegOffset);
    }
    else if(info->targetType == LogicalNameMap::TargetType::RANGE) {
      targetDevice->readReg(info->registerName,data,dataSize,addRegOffset + sizeof(int32_t)*info->firstIndex);
    }
    else if(info->targetType == LogicalNameMap::TargetType::CHANNEL) {
      throw DeviceException("Reading a channel of a multiplexed register is only supported using register accessors.",
          DeviceException::NOT_IMPLEMENTED);
    }
    else if(info->targetType == LogicalNameMap::TargetType::INT_CONSTANT) {
      if(dataSize >= 4 && addRegOffset == 0) {
        data[0] = info->value;
      }
    }

  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::write(const std::string &regModule, const std::string &regName,
      int32_t const *data, size_t dataSize, uint32_t addRegOffset) {

    // obtain register info
    RegisterPath name = RegisterPath(regModule)/regName;
    auto info = boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_catalogue.getRegister(name));

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info->hasDeviceName()) targetDevice = _devices[info->deviceName];

    // implementation for each type
    if(info->targetType == LogicalNameMap::TargetType::REGISTER) {
      targetDevice->writeReg(info->registerName,data,dataSize,addRegOffset);
    }
    else if(info->targetType == LogicalNameMap::TargetType::RANGE) {
      targetDevice->writeReg(info->registerName,data,dataSize,addRegOffset + sizeof(int32_t)*info->firstIndex);
    }
    else if(info->targetType == LogicalNameMap::TargetType::CHANNEL) {
      throw DeviceException("Writing a channel of a multiplexed register is only supported using register accessors.",
          DeviceException::NOT_IMPLEMENTED);
    }

  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > LogicalNameMappingBackend::getTwoDRegisterAccessor_impl(
      const std::string &module, const std::string &registerName) {
    (void)module; (void)registerName; // avoid warning
    throw DeviceException("2D register accessors not yet supported for LogicalNameMappingBackends.",
        DeviceException::NOT_IMPLEMENTED);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > LogicalNameMappingBackend::getBufferingRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) {

    // obtain register info
    auto info = boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // if applicable, obtain target device
    boost::shared_ptr<Device> targetDevice;
    if(info->hasDeviceName()) targetDevice = _devices[info->deviceName];

    // implementation for each type
    BufferingRegisterAccessorImpl<UserType> *ptr;
    if( info->targetType == LogicalNameMap::TargetType::REGISTER ||
        info->targetType == LogicalNameMap::TargetType::RANGE       ) {
      ptr = new LNMBackendBufferingRegisterAccessor<UserType>(shared_from_this(), registerPathName,
          numberOfWords, wordOffsetInRegister, enforceRawAccess);
    }
    else if( info->targetType == LogicalNameMap::TargetType::CHANNEL) {
      ptr = new LNMBackendBufferingChannelAccessor<UserType>(shared_from_this(), registerPathName,
          numberOfWords, wordOffsetInRegister, enforceRawAccess);
    }
    else if( info->targetType == LogicalNameMap::TargetType::INT_CONSTANT ||
             info->targetType == LogicalNameMap::TargetType::INT_VARIABLE    ) {
      ptr = new LNMBackendBufferingVariableAccessor<UserType>(shared_from_this(), registerPathName,
          numberOfWords, wordOffsetInRegister, enforceRawAccess);
    }
    else {
      throw DeviceException("For this register type, a RegisterAccessor cannot be obtained (name of logical register: "+
          registerPathName+").", DeviceException::NOT_IMPLEMENTED);
    }

    // allow plugins to decorate the accessor and return it
    auto accessor = boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> >(ptr);
    return decorateBufferingRegisterAccessor<UserType>(registerPathName,accessor);
  }


} // namespace mtca4u
