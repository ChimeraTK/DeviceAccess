/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include "LNMBackendChannelAccessor.h"
#include "LNMBackendVariableAccessor.h"
#include "LogicalNameMappingBackend.h"
#include "LogicalNameMapParser.h"

namespace mtca4u {

  LogicalNameMappingBackend::LogicalNameMappingBackend(std::string lmapFileName)
  : hasParsed(false), _lmapFileName(lmapFileName)
  {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::parse() {

    // don't run, if already parsed
    if(hasParsed) return;

    // parse the map fle
    LogicalNameMapParser parser = LogicalNameMapParser(_lmapFileName);
    _catalogue = parser.getCatalogue();

    // create all devices referenced in the map
    auto devNames = parser.getTargetDevices();
    for(auto devName = devNames.begin(); devName != devNames.end(); ++devName) {
      _devices[*devName] = BackendFactory::getInstance().createBackend(*devName);
    }

    // fill in information to the catalogue from the target devices
    for(auto &info : _catalogue) {
      LNMBackendRegisterInfo &info_cast = static_cast<LNMBackendRegisterInfo&>(info);
      if(info_cast.targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
        auto target_info = _devices[info_cast.deviceName]->getRegisterCatalogue().getRegister(std::string(info_cast.registerName));
        info_cast.nDimensions = target_info->getNumberOfDimensions();
        info_cast.nChannels = target_info->getNumberOfChannels();
        if((int)info_cast.length == 0) info_cast.length = target_info->getNumberOfElements();
      }
      else if(info_cast.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
        auto target_info = _devices[info_cast.deviceName]->getRegisterCatalogue().getRegister(std::string(info_cast.registerName));
        if((int)info_cast.length == 0) info_cast.length = target_info->getNumberOfElements();
      }
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
  boost::shared_ptr< NDRegisterAccessor<UserType> > LogicalNameMappingBackend::getRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) {

    // obtain register info
    auto info = boost::static_pointer_cast<LNMBackendRegisterInfo>(_catalogue.getRegister(registerPathName));

    // implementation for each type
    boost::shared_ptr< NDRegisterAccessor<UserType> > ptr;
    if( info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
      auto _targetDevice = _devices[info->deviceName];
      // determine the offset and length
      size_t actualOffset = size_t(info->firstIndex) + wordOffsetInRegister;
      size_t actualLength = ( numberOfWords > 0 ? numberOfWords : size_t(info->length) );
      // obtain underlying register accessor
      ptr = _targetDevice->getRegisterAccessor<UserType>(RegisterPath(info->registerName),actualLength,actualOffset,
          enforceRawAccess);
    }
    else if( info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
      ptr = boost::shared_ptr< NDRegisterAccessor<UserType> >(new LNMBackendChannelAccessor<UserType>(shared_from_this(),
          registerPathName, numberOfWords, wordOffsetInRegister, enforceRawAccess));
    }
    else if( info->targetType == LNMBackendRegisterInfo::TargetType::INT_CONSTANT ||
             info->targetType == LNMBackendRegisterInfo::TargetType::INT_VARIABLE    ) {
      ptr = boost::shared_ptr< NDRegisterAccessor<UserType> >(new LNMBackendVariableAccessor<UserType>(shared_from_this(),
          registerPathName, numberOfWords, wordOffsetInRegister, enforceRawAccess));
    }
    else {
      throw DeviceException("For this register type, a RegisterAccessor cannot be obtained (name of logical register: "+
          registerPathName+").", DeviceException::NOT_IMPLEMENTED);
    }

    // allow plugins to decorate the accessor and return it
    return decorateRegisterAccessor<UserType>(registerPathName,ptr);
  }


} // namespace mtca4u
