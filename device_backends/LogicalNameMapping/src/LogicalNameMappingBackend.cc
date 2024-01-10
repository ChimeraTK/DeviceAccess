// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LogicalNameMappingBackend.h"

#include "internal/LNMMathPluginFormulaHelper.h"
#include "LNMBackendBitAccessor.h"
#include "LNMBackendChannelAccessor.h"
#include "LNMBackendVariableAccessor.h"
#include "LogicalNameMapParser.h"

namespace ChimeraTK {

  LogicalNameMappingBackend::LogicalNameMappingBackend(std::string lmapFileName)
  : hasParsed(false), _lmapFileName(std::move(lmapFileName)) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::parse() const {
    // don't run, if already parsed
    if(hasParsed) return;
    hasParsed = true;

    // parse the map file
    LogicalNameMapParser parser = LogicalNameMapParser(_parameters, _variables);
    _catalogue_mutable = parser.parseFile(_lmapFileName);

    // create all devices referenced in the map
    for(const auto& devName : getTargetDevices()) {
      _devices[devName] = BackendFactory::getInstance().createBackend(devName);
    }
    // iterate over plugins and call postParsingHook
    for(auto& reg : _catalogue_mutable) {
      for(auto& p : reg.plugins) {
        auto thisp = boost::static_pointer_cast<const LogicalNameMappingBackend>(shared_from_this());
        p->postParsingHook(thisp);
      }
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMappingBackend::open() {
    if(isFunctional()) return;
    parse();

    // open all referenced devices (unconditionally, open() is also used for recovery)
    for(const auto& device : _devices) {
      device.second->open();
    }

    // flag as opened
    _versionOnOpen = ChimeraTK::VersionNumber{};
    _asyncReadActive = false;
    setOpenedAndClearException();

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
    for(const auto& device : _devices) {
      if(device.second->isOpen()) device.second->close();
    }
    // flag as closed
    _opened = false;
    _asyncReadActive = false;
  }

  /********************************************************************************************************************/

  // FIXME #11279 Implement API breaking changes from linter warnings
  // NOLINTBEGIN(performance-unnecessary-value-param)
  boost::shared_ptr<DeviceBackend> LogicalNameMappingBackend::createInstance(
      std::string /*address*/, std::map<std::string, std::string> parameters) {
    // NOLINTEND(performance-unnecessary-value-param)
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("Map file name not specified.");
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

    // Check that the requested requested accessor fits into the register as described by the info. It is not enough to
    // let the target do the check. It might be a sub-register of a much larger one and for the target it is fine.
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
      throw ChimeraTK::logic_error(
          "For this register type, a RegisterAccessor cannot be obtained (name of logical register: " +
          registerPathName + ").");
    }

    return ptr;
  }

  /********************************************************************************************************************/

  RegisterCatalogue LogicalNameMappingBackend::getRegisterCatalogue() const {
    if(catalogueCompleted) return RegisterCatalogue(_catalogue_mutable.clone());
    parse();

    // fill in information to the catalogue from the target devices
    for(auto& lnmInfo : _catalogue_mutable) {
      auto targetType = lnmInfo.targetType;
      if(targetType != LNMBackendRegisterInfo::TargetType::REGISTER &&
          targetType != LNMBackendRegisterInfo::TargetType::CHANNEL &&
          targetType != LNMBackendRegisterInfo::TargetType::BIT) {
        continue;
      }

      std::string devName = lnmInfo.deviceName;

      RegisterInfo target_info(lnmInfo.clone()); // Start with a clone of this info as there is not default constructor
      // In case the device is not "this" replace it with the real target register info
      if(devName != "this") {
        auto cat = _devices.at(devName)->getRegisterCatalogue();
        if(!cat.hasRegister(lnmInfo.registerName)) continue;
        target_info = cat.getRegister(lnmInfo.registerName);
      }
      else {
        target_info = _catalogue_mutable.getRegister(lnmInfo.registerName);
        // target_info might also be affected by plugins. e.g. forceReadOnly plugin
        // we need to process plugin list of target register before taking over anything
        auto& i = dynamic_cast<LNMBackendRegisterInfo&>(target_info.getImpl());
        for(auto& plugin : i.plugins) {
          plugin->updateRegisterInfo(_catalogue_mutable);
        }
        target_info = _catalogue_mutable.getRegister(lnmInfo.registerName);
      }

      lnmInfo.supportedFlags = target_info.getSupportedAccessModes();
      if(targetType != LNMBackendRegisterInfo::TargetType::BIT) {
        lnmInfo._dataDescriptor = target_info.getDataDescriptor();
      }
      else {
        lnmInfo._dataDescriptor = DataDescriptor(DataDescriptor::FundamentalType::boolean, true, false, 1, 0);
        lnmInfo.supportedFlags.remove(AccessMode::raw);
      }
      lnmInfo.readable = target_info.isReadable();
      lnmInfo.writeable = target_info.isWriteable();

      if(targetType == LNMBackendRegisterInfo::TargetType::CHANNEL) {
        lnmInfo.writeable = false;
      }

      if(targetType == LNMBackendRegisterInfo::TargetType::REGISTER) {
        lnmInfo.nChannels = target_info.getNumberOfChannels();
      }
      if(lnmInfo.length == 0) lnmInfo.length = target_info.getNumberOfElements();

      _catalogue_mutable.modifyRegister(lnmInfo);
    }

    // update catalogue info by plugins
    for(auto& lnmInfo : _catalogue_mutable) {
      for(auto& plugin : lnmInfo.plugins) {
        plugin->updateRegisterInfo(_catalogue_mutable);
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

  void LogicalNameMappingBackend::setExceptionImpl() noexcept {
    auto message = getActiveExceptionMessage();
    for(auto& d : _devices) {
      d.second->setException(message);
    }

    // iterate all push subscriptions of variables and place exception into queue
    if(_asyncReadActive) {
      VersionNumber v{};
      for(auto& r : _catalogue_mutable) {
        auto& info = dynamic_cast<LNMBackendRegisterInfo&>(r);
        if(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
          callForType(info.valueType, [&](auto arg) {
            auto& lnmVariable = _variables[info.name];
            auto& vtEntry = boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table);
            for(auto& sub : vtEntry.subscriptions) {
              try {
                throw ChimeraTK::runtime_error(message);
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
    if(!isFunctional()) return;

    // store information locally, as variable accessors have async read
    _asyncReadActive = true;

    // delegate to target devices
    for(auto& d : _devices) {
      d.second->activateAsyncRead();
    }

    // iterate all push subscriptions of variables and place initial value into queue
    VersionNumber v = getVersionOnOpen();
    for(auto& r : _catalogue_mutable) {
      auto& info = dynamic_cast<LNMBackendRegisterInfo&>(r);
      if(info.targetType == LNMBackendRegisterInfo::TargetType::VARIABLE) {
        auto& lnmVariable = _variables[info.name];
        try {
          callForType(info.valueType, [&](auto arg) {
            auto& vtEntry = boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table);
            // override version number if last write to variable was before reopening the device
            if(vtEntry.latestVersion < v) {
              vtEntry.latestVersion = v; // store in case an accessor is created after calling activateAsyncRead
            }
            for(auto& sub : vtEntry.subscriptions) {
              try {
                sub.second.push_overwrite({vtEntry.latestValue, vtEntry.latestValidity, vtEntry.latestVersion});
              }
              catch(std::system_error& e) {
                std::cerr << "Caught system error in activateAsyncRead(): " << e.what() << std::endl;
                std::terminate();
              }
            }
          });
        }
        catch(std::bad_cast& e) {
          // bad_cast is thrown by callForType if the type is not known. This should not happen at this point any more
          // because we are iterating a list that has already been processed before.
          std::ignore = e;
          assert(false);
        }
      }
    }
  }

  /********************************************************************************************************************/

  std::unordered_set<std::string> LogicalNameMappingBackend::getTargetDevices() const {
    std::unordered_set<std::string> ret;
    for(const auto& info : _catalogue_mutable) {
      if(info.deviceName != "this" && !info.deviceName.empty()) ret.insert(info.deviceName);
    }
    return ret;
  }

  /********************************************************************************************************************/

  ChimeraTK::VersionNumber LogicalNameMappingBackend::getVersionOnOpen() const {
    return _versionOnOpen;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
