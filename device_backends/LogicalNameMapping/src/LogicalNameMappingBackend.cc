/*
 * LogicalNameMappingBackend.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#  include "LogicalNameMappingBackend.h"
#  include "LNMBackendBitAccessor.h"
#  include "LNMBackendChannelAccessor.h"
#  include "LNMBackendVariableAccessor.h"
#  include "LogicalNameMapParser.h"

namespace ChimeraTK {

  LogicalNameMappingBackend::LogicalNameMappingBackend(std::string lmapFileName)
  : hasParsed(false), _lmapFileName(lmapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::parse() const {
    // don't run, if already parsed
    if(hasParsed) return;
    hasParsed = true;

    // parse the map fle
    LogicalNameMapParser parser = LogicalNameMapParser(_lmapFileName, _parameters);
    //parser.
    _catalogue_mutable = parser.getCatalogue();

    // create all devices referenced in the map
    for(auto& devName : parser.getTargetDevices()) {
      _devices[devName] = BackendFactory::getInstance().createBackend(devName);
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open() {
    if(isFunctional()) return;
    parse();

    // open all referenced devices (unconditionally, open() is also used for recovery)
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      device->second->open();
    }

    // flag as opened
    _opened = true;
    _hasException = false;
    _asyncReadActive = false;

    // make sure to update the catalogue from target devices in case they change their catalogue upon open
    catalogueCompleted = false;

    // call the openHook for all plugins
    for(auto& reg : _catalogue_mutable) {
      for(auto& plug : dynamic_cast<LNMBackendRegisterInfo&>(reg).plugins) {
        plug->openHook(boost::dynamic_pointer_cast<LogicalNameMappingBackend>(shared_from_this()));
      }
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::close() {
    if(!_opened) return;

    // call the closeHook for all plugins
    for(auto& reg : _catalogue_mutable) {
      for(auto& plug : dynamic_cast<LNMBackendRegisterInfo&>(reg).plugins) {
        plug->closeHook();
      }
    }

    // close all referenced devices
    for(auto device = _devices.begin(); device != _devices.end(); ++device) {
      if(device->second->isOpen()) device->second->close();
    }
    // flag as closed
    _opened = false;
    _asyncReadActive = false;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> LogicalNameMappingBackend::createInstance(
      std::string /*address*/, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("Map file name not speficied.");
    }
    auto ptr = boost::make_shared<LogicalNameMappingBackend>(parameters["map"]);
    parameters.erase(parameters.find("map"));
    ptr->_parameters = parameters;
    return boost::static_pointer_cast<DeviceBackend>(ptr);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> LogicalNameMappingBackend::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags,
      size_t omitPlugins) {
    parse();
    // check if accessor plugin present
    boost::shared_ptr<NDRegisterAccessor<UserType>> returnValue;
    auto info = _catalogue_mutable.getBackendRegister(registerPathName);
    if(info.plugins.size() <= omitPlugins) {
      // no plugin: directly return the accessor
      returnValue =
          getRegisterAccessor_internal<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
    else {
      returnValue = info.plugins[omitPlugins]->getAccessor<UserType>(
          boost::static_pointer_cast<LogicalNameMappingBackend>(shared_from_this()), numberOfWords,
          wordOffsetInRegister, flags, omitPlugins);
    }

    returnValue->setExceptionBackend(shared_from_this());

    return returnValue;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> LogicalNameMappingBackend::getRegisterAccessor_internal(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    // obtain register info
    auto info = _catalogue_mutable.getBackendRegister(registerPathName);

    // Check that the requested requested accessor fits into the register as described by the info. It is not enough to let
    // the target do the check. It might be a sub-register of a much larger one and for the target it is fine.
    if(info.length != 0) {
      // If info->length is 0 we let the target device do the checking. Nothing we can decide here.
      if(numberOfWords == 0) numberOfWords = info.length;
      if((numberOfWords + wordOffsetInRegister) > info.length) {
        throw ChimeraTK::logic_error(
            std::string(
                "LogicalNameMappingBackend: Error creating accessor. Number of words plus offset too large in ") +
            registerPathName);
      }
    }

    // determine the offset and length
    size_t actualOffset = size_t(info.firstIndex) + wordOffsetInRegister;
    size_t actualLength = (numberOfWords > 0 ? numberOfWords : size_t(info.length));

    // implementation for each type
    boost::shared_ptr<NDRegisterAccessor<UserType>> ptr;
    if(info.targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
      DeviceBackend* _targetDevice;
      std::string devName = info.deviceName;
      if(devName != "this") {
        _targetDevice = _devices[info.deviceName].get();
      }
      else {
        _targetDevice = this;
      }
      // make sure the target device exists
      if(_targetDevice == nullptr) {
        throw ChimeraTK::logic_error("Target device for this logical register is not opened. See "
                                     "exception thrown in open()!");
      }
      // obtain underlying register accessor
      ptr = _targetDevice->getRegisterAccessor<UserType>(
          RegisterPath(info.registerName), actualLength, actualOffset, flags);
    }
    else if(info.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
      ptr = boost::shared_ptr<NDRegisterAccessor<UserType>>(new LNMBackendChannelAccessor<UserType>(
          shared_from_this(), registerPathName, actualLength, actualOffset, flags));
    }
    else if(info.targetType == LNMBackendRegisterInfo::TargetType::BIT) {
      ptr = boost::shared_ptr<NDRegisterAccessor<UserType>>(
          new LNMBackendBitAccessor<UserType>(shared_from_this(), registerPathName, actualLength, actualOffset, flags));
    }
    else if(info.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT ||
        info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
      ptr = boost::shared_ptr<NDRegisterAccessor<UserType>>(new LNMBackendVariableAccessor<UserType>(
          shared_from_this(), registerPathName, actualLength, actualOffset, flags));
    }
    else {
      throw ChimeraTK::logic_error("For this register type, a RegisterAccessor cannot be obtained (name "
                                   "of logical register: " +
          registerPathName + ").");
    }

    return ptr;
  }

  /********************************************************************************************************************/

  RegisterCatalogue LogicalNameMappingBackend::getRegisterCatalogue() const {
    if(catalogueCompleted) return RegisterCatalogue(_catalogue_mutable.clone());
    parse();

    // fill in information to the catalogue from the target devices
    for(auto& info : _catalogue_mutable) {
      LNMBackendRegisterInfo& info_cast = static_cast<LNMBackendRegisterInfo&>(info);
      auto targetType = info_cast.targetType;
      if(targetType != LNMBackendRegisterInfo::TargetType::REGISTER &&
          targetType != LNMBackendRegisterInfo::TargetType::CHANNEL &&
          targetType != LNMBackendRegisterInfo::TargetType::BIT)
        continue;

      std::string devName = info_cast.deviceName;
      //boost::shared_ptr<RegisterInfoImpl target_info;
      LNMBackendRegisterInfo target_info;
      if(devName != "this") {
        auto cat = _devices.at(devName)->getRegisterCatalogue();
        if(!cat.hasRegister(info_cast.registerName)) continue;
        target_info = cat.getRegister(info_cast.registerName);
      }
      else {
        target_info = _catalogue_mutable.getBackendRegister(std::string(info_cast.registerName));
      }

      info_cast.supportedFlags = target_info.getSupportedAccessModes();
      if(targetType != LNMBackendRegisterInfo::TargetType::BIT) {
        info_cast._dataDescriptor = target_info.getDataDescriptor();
      }
      else {
        info_cast._dataDescriptor =
            DataDescriptor(DataDescriptor::FundamentalType::boolean, true, false, 1, 0);
        info_cast.supportedFlags.remove(AccessMode::raw);
      }
      info_cast.readable = target_info.isReadable();
      info_cast.writeable = target_info.isWriteable();

      if(targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
        info_cast.writeable = false;
      }

      if(targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
        info_cast.nDimensions = target_info.getNumberOfDimensions();
        info_cast.nChannels = target_info.getNumberOfChannels();
      }
      if((int)info_cast.length == 0) info_cast.length = target_info.getNumberOfElements();
    }

    // update catalogue info by plugins
    for(auto& info : _catalogue_mutable) {
      LNMBackendRegisterInfo& info_cast = static_cast<LNMBackendRegisterInfo&>(info);
      for(auto& plugin : info_cast.plugins) {
        plugin->updateRegisterInfo();
      }
    }

    catalogueCompleted = true;
    return RegisterCatalogue(_catalogue_mutable.clone());
  }

  /********************************************************************************************************************/
  // Instantiate templated members - this is required for some gcc versions like the one on Ubuntu 18.04

  template<typename UserType>
  class InstantiateLogicalNameMappingBackendFunctions {
    LogicalNameMappingBackend* p{nullptr};

    void getRegisterAccessor_impl(const RegisterPath& registerPathName, size_t numberOfWords,
        size_t wordOffsetInRegister, AccessModeFlags flags, size_t omitPlugins) {
      p->getRegisterAccessor_impl<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags, omitPlugins);
    }

    void getRegisterAccessor_internal(const RegisterPath& registerPathName, size_t numberOfWords,
        size_t wordOffsetInRegister, AccessModeFlags flags) {
      p->getRegisterAccessor_internal<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    }
  };
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(InstantiateLogicalNameMappingBackendFunctions);

  /********************************************************************************************************************/
  bool LogicalNameMappingBackend::isFunctional() const {
    if(not _opened) {
      return false;
    }
    if(_hasException) {
      return false;
    }
    for(auto& e : _devices) {
      if(not e.second->isFunctional()) {
        return false;
      }
    }
    return true;
  }

  /********************************************************************************************************************/
  void LogicalNameMappingBackend::setException() {
    _hasException = true;
    for(auto& d : _devices) {
      d.second->setException();
    }

    // iterate all push subscriptions of variables and place exception into queue
    if(_asyncReadActive) {
      VersionNumber v{};
      for(auto& r : _catalogue_mutable) {
        auto& info = dynamic_cast<LNMBackendRegisterInfo&>(r);
        if(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
          callForType(info.valueType, [&](auto arg) {
            auto& vtEntry = boost::fusion::at_key<decltype(arg)>(info.valueTable.table);
            for(auto& sub : vtEntry.subscriptions) {
              try {
                throw ChimeraTK::runtime_error("previous, unrecovered fault");
              }
              catch(...) {
                sub.second.push_overwrite_exception(std::current_exception());
              }
            }
          });
        }
      }
    }

    // deactivate async read
    _asyncReadActive = false;

    // call the exceptionHook for all plugins
    for(auto& reg : _catalogue_mutable) {
      for(auto& plug : dynamic_cast<LNMBackendRegisterInfo&>(reg).plugins) {
        plug->exceptionHook();
      }
    }
  }

  /********************************************************************************************************************/
  void LogicalNameMappingBackend::activateAsyncRead() noexcept {
    if(!_opened || _hasException) return;

    // store information locally, as variable accessors have async read
    _asyncReadActive = true;

    // delegate to target devices
    for(auto& d : _devices) {
      d.second->activateAsyncRead();
    }

    // iterate all push subscriptions of variables and place initial value into queue
    VersionNumber v{};
    for(auto& r : _catalogue_mutable) {
      auto& info = dynamic_cast<LNMBackendRegisterInfo&>(r);
      if(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
        callForType(info.valueType, [&](auto arg) {
          auto& vtEntry = boost::fusion::at_key<decltype(arg)>(info.valueTable.table);
          for(auto& sub : vtEntry.subscriptions) {
            sub.second.push_overwrite({vtEntry.latestValue, vtEntry.latestValidity, v});
          }
        });
      }
    }
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK

