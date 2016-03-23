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
      _devices[*devName] = BackendFactory::getInstance().createBackend(*devName);
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open()
  {
    parse();
    // open all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      device->second->open();
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

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > LogicalNameMappingBackend::getBufferingRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) {

    // obtain register info
    auto info = boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_catalogue.getRegister(registerPathName));

    // implementation for each type
    NDRegisterAccessor<UserType> *ptr;
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
    auto accessor = boost::shared_ptr< NDRegisterAccessor<UserType> >(ptr);
    return decorateBufferingRegisterAccessor<UserType>(registerPathName,accessor);
  }


} // namespace mtca4u
