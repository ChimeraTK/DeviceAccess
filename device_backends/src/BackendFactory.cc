// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BackendFactory.h"

#include "RebotBackend.h"
#include "Utilities.h"

#include <boost/algorithm/string.hpp>
#ifdef CHIMERATK_HAVE_PCIE_BACKEND
#  include "PcieBackend.h"
#endif
#ifdef CHIMERATK_HAVE_XDMA_BACKEND
#  include "XdmaBackend.h"
#endif
#ifdef CHIMERATK_HAVE_UIO_BACKEND
#  include "UioBackend.h"
#endif
// Clang tidy reports a false positive. It seems to be case-sensitive although it should not be.
// clang-format is fixing this correctly.
// NOLINTNEXTLINE(llvm-include-order)
#include "DeviceAccessVersion.h"
#include "DMapFileParser.h"
#include "DummyBackend.h"
#include "Exception.h"
#include "LogicalNameMappingBackend.h"
#include "SharedDummyBackend.h"
#include "SubdeviceBackend.h"

#include <boost/bind/bind.hpp>
#include <boost/function.hpp>

#include <dlfcn.h>
#include <utility>

#ifdef _DEBUG
#  include <iostream>
#endif

namespace ChimeraTK {

  /********************************************************************************************************************/

  void BackendFactory::registerBackendType(const std::string& backendType,
      boost::shared_ptr<DeviceBackend> (*creatorFunction)(
          std::string address, std::map<std::string, std::string> parameters),
      const std::vector<std::string>& sdmParameterNames, const std::string& version) {
#ifdef _DEBUG
    std::cout << "adding:" << backendType << std::endl << std::flush;
#endif
    called_registerBackendType = true;

    if(creatorMap.find(backendType) != creatorMap.end()) {
      throw ChimeraTK::logic_error("A backend with the type name '" + backendType + "' has already been registered.");
    }

    if(version != CHIMERATK_DEVICEACCESS_VERSION) {
      // Register a function that throws an exception with the message when trying
      // to create a backend. We do not throw here because this would throw an
      // uncatchable exception inside the static instance of a backend registerer,
      // which would violate the policy that a dmap file with broken backends
      // should be usable if the particular backend is not used.
      std::stringstream errorMessage;
      errorMessage << "Backend plugin '" << backendType << "' compiled with wrong DeviceAccess version " << version
                   << ". Please recompile with version " << CHIMERATK_DEVICEACCESS_VERSION;
      std::string errorMessageString = errorMessage.str();
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTBEGIN(performance-unnecessary-value-param)
      creatorMap_compat[make_pair(backendType, "")] = [errorMessageString](std::string host, std::string instance,
                                                          std::list<std::string> parameters, std::string mapFileName) {
        return BackendFactory::failedRegistrationThrowerFunction(
            host, instance, parameters, mapFileName, errorMessageString);
      };
      creatorMap[backendType] = [errorMessageString](std::string,
                                    std::map<std::string, std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
        // NOLINTEND(performance-unnecessary-value-param)
        throw ChimeraTK::logic_error(errorMessageString);
      };
      return;
    }
    creatorMap[backendType] = creatorFunction;
    // FIXME #11279 Implement API breaking changes from linter warnings
    // NOLINTBEGIN(performance-unnecessary-value-param)
    creatorMap_compat[make_pair(backendType, "")] = [creatorFunction, sdmParameterNames](std::string,
                                                        std::string instance, std::list<std::string> parameters,
                                                        std::string mapFileName) {
      // NOLINTEND(performance-unnecessary-value-param)
      std::map<std::string, std::string> pars;
      size_t i = 0;
      for(auto& p : parameters) {
        if(i >= sdmParameterNames.size()) break;
        pars[sdmParameterNames[i]] = p;
        ++i;
      }
      if(!mapFileName.empty()) {
        if(pars["map"].empty()) {
          pars["map"] = mapFileName;
        }
        else {
          std::cout << "WARNING: You have specified the map file name twice, in "
                       "the parameter list and in the 3rd "
                       "column of the DMAP file."
                    << std::endl;
          std::cout << "Please only specify the map file name in the parameter list!" << std::endl;
        }
      }
      return creatorFunction(std::move(instance), pars);
    };
  }

  /********************************************************************************************************************/

  void BackendFactory::registerBackendType(const std::string& interface, const std::string& protocol,
      boost::shared_ptr<DeviceBackend> (*creatorFunction)(
          std::string host, std::string instance, std::list<std::string> parameters, std::string mapFileName),
      const std::string& version) {
#ifdef _DEBUG
    std::cout << "adding:" << interface << std::endl << std::flush;
#endif
    called_registerBackendType = true;
    if(version != CHIMERATK_DEVICEACCESS_VERSION) {
      // Register a function that throws an exception with the message when trying
      // to create a backend. We do not throw here because this would throw an
      // uncatchable exception inside the static instance of a backend registerer,
      // which would violate the policy that a dmap file with broken backends
      // should be usable if the particular backend is not used.
      std::stringstream errorMessage;
      errorMessage << "Backend plugin '" << interface << "' compiled with wrong DeviceAccess version " << version
                   << ". Please recompile with version " << CHIMERATK_DEVICEACCESS_VERSION;
      std::string errorMessageString = errorMessage.str();
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTBEGIN(performance-unnecessary-value-param)
      creatorMap_compat[make_pair(interface, protocol)] = [errorMessageString](std::string host, std::string instance,
                                                              std::list<std::string> parameters,
                                                              std::string mapFileName) {
        return BackendFactory::failedRegistrationThrowerFunction(
            host, instance, parameters, mapFileName, errorMessageString);
      };
      creatorMap[interface] = [errorMessageString](std::string,
                                  std::map<std::string, std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
        // NOLINTEND(performance-unnecessary-value-param)
        throw ChimeraTK::logic_error(errorMessageString);
      };
      return;
    }
    creatorMap_compat[make_pair(interface, protocol)] = creatorFunction;
    // FIXME #11279 Implement API breaking changes from linter warnings
    // NOLINTBEGIN(performance-unnecessary-value-param)
    creatorMap[interface] = [interface](std::string,
                                std::map<std::string, std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
      // NOLINTEND(performance-unnecessary-value-param)
      throw ChimeraTK::logic_error("The backend type '" + interface +
          "' does not yet support ChimeraTK device "
          "descriptors! Please update the backend!");
    };
  }

  /********************************************************************************************************************/

  void BackendFactory::setDMapFilePath(std::string dMapFilePath) {
    _dMapFile = std::move(dMapFilePath);
    loadAllPluginsFromDMapFile();
  }
  /********************************************************************************************************************/

  std::string BackendFactory::getDMapFilePath() {
    return _dMapFile;
  }
  /********************************************************************************************************************/

  BackendFactory::BackendFactory() {
#ifdef CHIMERATK_HAVE_PCIE_BACKEND
    registerBackendType("pci", &PcieBackend::createInstance, {"map"});
#endif
#ifdef CHIMERATK_HAVE_XDMA_BACKEND
    registerBackendType("xdma", &XdmaBackend::createInstance, {"map"});
#endif
#ifdef CHIMERATK_HAVE_UIO_BACKEND
    registerBackendType("uio", &UioBackend::createInstance, {"map"});
#endif
    registerBackendType("dummy", &DummyBackend::createInstance, {"map"});
    registerBackendType("rebot", &RebotBackend::createInstance, {"ip", "port", "map", "timeout"});
    registerBackendType("logicalNameMap", &LogicalNameMappingBackend::createInstance, {"map"});
    registerBackendType("subdevice", &SubdeviceBackend::createInstance, {"map"});
    registerBackendType("sharedMemoryDummy", &SharedDummyBackend::createInstance, {"map"});
  }
  /********************************************************************************************************************/

  BackendFactory& BackendFactory::getInstance() {
#ifdef _DEBUG
    std::cout << "getInstance" << std::endl << std::flush;
#endif
    static BackendFactory factoryInstance; /** Thread safe in C++11*/
    return factoryInstance;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackend(const std::string& aliasOrUri) {
    std::lock_guard<std::mutex> lock(_mutex);

    if(Utilities::isDeviceDescriptor(aliasOrUri) || Utilities::isSdm(aliasOrUri)) {
      // it is a URI, directly create a deviceinfo and call the internal creator
      // function
      DeviceInfoMap::DeviceInfo deviceInfo;
      deviceInfo.uri = aliasOrUri;
      return createBackendInternal(deviceInfo);
    }

    // it's not a URI. Try finding the alias in the dmap file.
    if(_dMapFile.empty()) {
      throw ChimeraTK::logic_error("DMap file not set.");
    }
    auto deviceInfo = Utilities::aliasLookUp(aliasOrUri, _dMapFile);
    return createBackendInternal(deviceInfo);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackendInternal(const DeviceInfoMap::DeviceInfo& deviceInfo) {
#ifdef _DEBUG
    std::cout << "uri to parse" << deviceInfo.uri << std::endl;
    std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif

    // Check if backend already exists
    auto iterator = _existingBackends.find(deviceInfo.uri);
    if(iterator != _existingBackends.end()) {
      auto strongPtr = iterator->second.lock();
      if(strongPtr) {
        return strongPtr;
      }
    }

    // Check if descriptor string is a ChimeraTK device descriptor
    if(Utilities::isDeviceDescriptor(deviceInfo.uri)) {
      auto cdd = Utilities::parseDeviceDesciptor(deviceInfo.uri);
      try {
        auto backend = (creatorMap.at(cdd.backendType))(cdd.address, cdd.parameters);
        _existingBackends[deviceInfo.uri] = backend;
        return backend;
      }
      catch(std::out_of_range&) {
        throw ChimeraTK::logic_error("Unknown backend: \"" + cdd.backendType + "\" at " + deviceInfo.dmapFileName +
            ":" + std::to_string(deviceInfo.dmapFileLineNumber) + " for " + deviceInfo.uri);
      }
    }

    // Check for deprecated SDM or device node
    // Note: Deprecation message was added 2022-07-28. Remove functionality past end of 2023.
    Sdm sdm;
    if(Utilities::isSdm(deviceInfo.uri)) {
      sdm = Utilities::parseSdm(deviceInfo.uri);
      std::cout << "Using the SDM descriptor is deprecated. Please change to CDD (ChimeraTK device descriptor)."
                << std::endl;
    }
    else {
      sdm = Utilities::parseDeviceString(deviceInfo.uri);
      std::cout
          << "Using the device node in a dmap file is deprecated. Please change to CDD (ChimeraTK device descriptor)."
          << std::endl;
    }
#ifdef _DEBUG
    std::cout << "sdm._Host:" << sdm._Host << std::endl;
    std::cout << "sdm.Interface:" << sdm._Interface << std::endl;
    std::cout << "sdm.Instance:" << sdm._Instance << std::endl;
    std::cout << "sdm._Protocol:" << sdm._Protocol << std::endl;
    std::cout << "sdm.parameter:" << sdm._Parameters.size() << std::endl;
    for(std::list<std::string>::iterator it = sdm._Parameters.begin(); it != sdm._Parameters.end(); ++it) {
      std::cout << *it << std::endl;
    }
#endif
    for(auto& iter : creatorMap_compat) {
#ifdef _DEBUG
      std::cout << "Pair:" << iter->first.first << "+" << iter->first.second << std::endl;
#endif
      if((iter.first.first == sdm._Interface)) {
        auto backend = (iter.second)(sdm._Host, sdm._Instance, sdm._Parameters, deviceInfo.mapFileName);
        boost::weak_ptr<DeviceBackend> weakBackend = backend;
        _existingBackends[deviceInfo.uri] = weakBackend;
        // return the shared pointer, not the weak pointer
        return backend;
      }
    }

    throw ChimeraTK::logic_error("Unregistered device: Interface = " + sdm._Interface + " Protocol = " + sdm._Protocol);
    return {}; // won't execute
  }

  /********************************************************************************************************************/

  void BackendFactory::loadPluginLibrary(const std::string& soFile) {
    // reset flag to check if the registerBackedType() function was called
    called_registerBackendType = false;

    // open the plugin library. RTLD_LAZY is enough since all symbols are loaded
    // when needed
    void* hndl = dlopen(soFile.c_str(), RTLD_LAZY);
    if(hndl == nullptr) {
      throw ChimeraTK::logic_error(dlerror());
    }

    // if no backend was registered, close the library and throw an exception
    if(!called_registerBackendType) {
      dlclose(hndl);
      throw ChimeraTK::logic_error(
          "'" + soFile + "' is not a valid DeviceAccess plugin, it does not register any backends!");
    }
  }

  /********************************************************************************************************************/

  void BackendFactory::loadAllPluginsFromDMapFile() {
    if(_dMapFile.empty()) {
      return;
    }

    auto dmap = DMapFileParser().parse(_dMapFile);

    for(const auto& lib : dmap->getPluginLibraries()) {
      try {
        loadPluginLibrary(lib);
      }
      catch(ChimeraTK::logic_error& e) {
        // Ignore library loading errors when doing this automatically for all
        // plugins in the dmap file to keep dmap files usable if the broken
        // backends are not used. Print a warning that the loading failed.
        std::cerr << "Error: Caught exception loading plugin '" << lib << "' specified in dmap file: " << e.what()
                  << std::endl;
        std::cerr << "Some backends will not be available!" << std::endl;
      }
    }
  }

  /********************************************************************************************************************/

  // FIXME #11279 Implement API breaking changes from linter warnings
  // NOLINTBEGIN(performance-unnecessary-value-param)
  boost::shared_ptr<DeviceBackend> BackendFactory::failedRegistrationThrowerFunction(std::string /*host*/,
      std::string /*instance*/, std::list<std::string> /*parameters*/, std::string /*mapFileName*/,
      std::string exception_what) {
    // NOLINTEND(performance-unnecessary-value-param)
    throw ChimeraTK::logic_error(exception_what);
  }
} // namespace ChimeraTK
