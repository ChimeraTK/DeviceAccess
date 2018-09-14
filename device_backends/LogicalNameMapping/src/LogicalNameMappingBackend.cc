/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include "LNMBackendChannelAccessor.h"
#include "LNMBackendBitAccessor.h"
#include "LNMBackendVariableAccessor.h"
#include "LogicalNameMappingBackend.h"
#include "LogicalNameMapParser.h"

namespace ChimeraTK {

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
    _catalogue_mutable = parser.getCatalogue();

    // create all devices referenced in the map
    for(auto &devName : parser.getTargetDevices()) {
      _devices[devName] = BackendFactory::getInstance().createBackend(devName);
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open()
  {
    parse();
    // open all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      if (!device->second->isOpen())
        device->second->open();
    }
    // flag as opened
    _opened = true;
    catalogueCompleted = false;
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::close()
  {
    // close all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      if (device->second->isOpen())
        device->second->close();
    }
    // flag as closed
    _opened = false;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> LogicalNameMappingBackend::createInstance(
      std::string /*address*/, std::map<std::string,std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("Map file name not speficied.");
    }
    return boost::shared_ptr<DeviceBackend>(new LogicalNameMappingBackend(parameters["map"]));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > LogicalNameMappingBackend::getRegisterAccessor_impl(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {

    // obtain register info
    auto info = boost::static_pointer_cast<LNMBackendRegisterInfo>(_catalogue_mutable.getRegister(registerPathName));

    // implementation for each type
    boost::shared_ptr< NDRegisterAccessor<UserType> > ptr;
    if( info->targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
      DeviceBackend *_targetDevice;
      std::string devName = info->deviceName;
      if(devName != "this") {
        _targetDevice = _devices[info->deviceName].get();
      }
      else {
        _targetDevice = this;
      }
      // make sure the target device exists
      if(_targetDevice == nullptr) {
        throw ChimeraTK::logic_error("Target device for this logical register is not opened. See exception thrown in open()!");
      }
      // determine the offset and length
      size_t actualOffset = size_t(info->firstIndex) + wordOffsetInRegister;
      size_t actualLength = ( numberOfWords > 0 ? numberOfWords : size_t(info->length) );
      // obtain underlying register accessor
      ptr = _targetDevice->getRegisterAccessor<UserType>(RegisterPath(info->registerName),actualLength,actualOffset,flags);
    }
    else if( info->targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
      ptr = boost::shared_ptr< NDRegisterAccessor<UserType> >(new LNMBackendChannelAccessor<UserType>(shared_from_this(),
          registerPathName, numberOfWords, wordOffsetInRegister, flags));
    }
    else if( info->targetType == LNMBackendRegisterInfo::TargetType::BIT) {
      ptr = boost::shared_ptr< NDRegisterAccessor<UserType> >(new LNMBackendBitAccessor<UserType>(shared_from_this(),
          registerPathName, numberOfWords, wordOffsetInRegister, flags));
    }
    else if( info->targetType == LNMBackendRegisterInfo::TargetType::INT_CONSTANT ||
             info->targetType == LNMBackendRegisterInfo::TargetType::INT_VARIABLE    ) {
      ptr = boost::shared_ptr< NDRegisterAccessor<UserType> >(new LNMBackendVariableAccessor<UserType>(shared_from_this(),
          registerPathName, numberOfWords, wordOffsetInRegister, flags));
    }
    else {
      throw ChimeraTK::logic_error("For this register type, a RegisterAccessor cannot be obtained (name of logical register: "+
          registerPathName+").");
    }

    return ptr;
  }

  /********************************************************************************************************************/

  const RegisterCatalogue& LogicalNameMappingBackend::getRegisterCatalogue() const {

    if(catalogueCompleted) return _catalogue_mutable;

    // fill in information to the catalogue from the target devices
    for(auto &info : _catalogue_mutable) {
      LNMBackendRegisterInfo &info_cast = static_cast<LNMBackendRegisterInfo&>(info);
      auto targetType = info_cast.targetType;
      if(targetType != LNMBackendRegisterInfo::TargetType::REGISTER &&
         targetType != LNMBackendRegisterInfo::TargetType::CHANNEL     ) continue;

      std::string devName = info_cast.deviceName;
      boost::shared_ptr<RegisterInfo> target_info;
      if(devName != "this") {
        target_info = _devices.at(devName)->getRegisterCatalogue().getRegister(std::string(info_cast.registerName));
      }
      else {
        target_info = getRegisterCatalogue().getRegister(std::string(info_cast.registerName));
      }

      info_cast._dataDescriptor = target_info->getDataDescriptor();

      if(targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
        info_cast.nDimensions = target_info->getNumberOfDimensions();
        info_cast.nChannels = target_info->getNumberOfChannels();
      }
      if((int)info_cast.length == 0) info_cast.length = target_info->getNumberOfElements();
    }

    catalogueCompleted = true;
    return _catalogue_mutable;
  }

} // namespace ChimeraTK
