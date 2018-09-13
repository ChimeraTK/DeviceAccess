/*
 * BackendFactory.cpp
 *
 *  Created on: Jul 9, 2015
 *      Author: nshehzad
 */

#include <boost/algorithm/string.hpp>

#include "Utilities.h"
#include "BackendFactory.h"
#include "MapFileParser.h"
#include "RebotBackend.h"
#ifdef CHIMERATK_HAVE_PCIE_BACKEND
#include "PcieBackend.h"
#endif
#include "DummyBackend.h"
#include "LogicalNameMappingBackend.h"
#include "SubdeviceBackend.h"
#include "SharedDummyBackend.h"
#include "DMapFileParser.h"
#include "Exception.h"
#include "DeviceAccessVersion.h"
#include <dlfcn.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace ChimeraTK {

  /********************************************************************************************************************/

  void BackendFactory::registerBackendType(std::string backendType,
     boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance,
                                                         std::map<std::string, std::string> parameters),
     std::string version)
  {
#ifdef _DEBUG
    std::cout << "adding:" << backendType << std::endl << std::flush;
#endif
    if (version != CHIMERATK_DEVICEACCESS_VERSION){
      // Register a function that throws an exception with the message when trying to create a backend.
      // We do not throw here because this would throw an uncatchable exception inside the static instance
      // of a backend registerer, which would violate the policy that a dmap file with broken backends
      // should be usable if the particular backend is not used.
      std::stringstream errorMessage;
      errorMessage << "Backend plugin '"<< backendType << "' compiled with wrong DeviceAccess version "
                   << version <<". Please recompile with version " << CHIMERATK_DEVICEACCESS_VERSION;
      creatorMap_compat[make_pair(backendType,"")] =
        boost::bind(BackendFactory::failedRegistrationThrowerFunction,_1,_2,_3,_4,errorMessage.str());
      std::string errorMessageString = errorMessage.str();
      creatorMap[backendType] =
        [errorMessageString](std::string, std::string, std::map<std::string,std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
          throw ChimeraTK::logic_error(errorMessageString);
        };
      return;
    }
    creatorMap[backendType] = creatorFunction;
    creatorMap_compat[make_pair(backendType,"")] =
      [creatorFunction](std::string host, std::string instance, std::list<std::string> parameters, std::string) {
        std::map<std::string,std::string> pars;
        size_t i=0;
        for(auto &p : parameters) {
          pars[std::to_string(i)] = p;
          ++i;
        }
        return creatorFunction(host, instance, pars);
      };
  }

  /********************************************************************************************************************/

  void BackendFactory::registerBackendType(std::string interface, std::string protocol,
     boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance,
                                                         std::list<std::string> parameters, std::string mapFileName),
     std::string version)
  {
#ifdef _DEBUG
    std::cout << "adding:" << interface << std::endl << std::flush;
#endif
    if (version != CHIMERATK_DEVICEACCESS_VERSION){
      // Register a function that throws an exception with the message when trying to create a backend.
      // We do not throw here because this would throw an uncatchable exception inside the static instance
      // of a backend registerer, which would violate the policy that a dmap file with broken backends
      // should be usable if the particular backend is not used.
      std::stringstream errorMessage;
      errorMessage << "Backend plugin '"<< interface << "' compiled with wrong DeviceAccess version "
                   << version <<". Please recompile with version " << CHIMERATK_DEVICEACCESS_VERSION;
      creatorMap_compat[make_pair(interface,protocol)] =
        boost::bind(BackendFactory::failedRegistrationThrowerFunction,_1,_2,_3,_4,errorMessage.str());
      std::string errorMessageString = errorMessage.str();
      creatorMap[interface] =
        [errorMessageString](std::string, std::string, std::map<std::string,std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
          throw ChimeraTK::logic_error(errorMessageString);
        };
      return;
    }
    creatorMap_compat[make_pair(interface,protocol)] = creatorFunction;
    creatorMap[interface] =
      [interface](std::string, std::string, std::map<std::string,std::string>) -> boost::shared_ptr<ChimeraTK::DeviceBackend> {
        throw ChimeraTK::logic_error("The backend type '"+interface+"' does not yet support ChimeraTK device "
                                     "descriptors! Please update the backend!");
      };
  }

  /********************************************************************************************************************/

  void BackendFactory::setDMapFilePath(std::string dMapFilePath)
  {
    _dMapFile = dMapFilePath;
  }
  /********************************************************************************************************************/

  std::string BackendFactory::getDMapFilePath()
  {
    return _dMapFile;
  }
  /********************************************************************************************************************/

  BackendFactory::BackendFactory(){
#ifdef CHIMERATK_HAVE_PCIE_BACKEND
    registerBackendType("pci","",&PcieBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("pci","pcie",&PcieBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
#endif
    registerBackendType("dummy","",&DummyBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("rebot","",&RebotBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("logicalNameMap","",&LogicalNameMappingBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("subdevice","",&SubdeviceBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("sharedMemoryDummy", "", &SharedDummyBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
  }
  /********************************************************************************************************************/

  BackendFactory & BackendFactory::getInstance(){
#ifdef _DEBUG
    std::cout << "getInstance" << std::endl << std::flush;
#endif
    static BackendFactory factoryInstance; /** Thread safe in C++11*/
    return factoryInstance;
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackend(std::string aliasOrUri) {

    std::lock_guard<std::mutex> lock(_mutex);

    loadAllPluginsFromDMapFile();

    if(Utilities::isSdm(aliasOrUri)) {
      // it is a URI, directly create a deviceinfo and call the internal creator function
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

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackendInternal(const DeviceInfoMap::DeviceInfo &deviceInfo) {
#ifdef _DEBUG
    std::cout << "uri to parse" << deviceInfo.uri << std::endl;
    std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif
    auto iterator = _existingBackends.find(deviceInfo.uri);
    if (iterator != _existingBackends.end())
    {
      auto strongPtr = iterator->second.lock();
      if (strongPtr) {
        return strongPtr;
      }
    }
    Sdm sdm;
    try {
      sdm = Utilities::parseSdm(deviceInfo.uri);
    }
    catch(ChimeraTK::logic_error &e){
      //fixme: the programme flow should not use exceptions here. It is a supported
      // condition that the old device syntax is used.

      //parseDeviceString currently does not throw
      sdm = Utilities::parseDeviceString(deviceInfo.uri);
      // todo: enable the deprecated warning. As long as most servers are still using MtcaMappedDevice
      // DMap files have to stay with device node, but QtHardMon would print the message, which causes confusion.
      //std::cout<<"# Using the device node in a dmap file is deprecated. Please change to sdm if applicable."<<std::endl;
    }
#ifdef _DEBUG
    std::cout<< "sdm._Host:"<<sdm._Host<< std::endl;
    std::cout<< "sdm.Interface:"<<sdm._Interface<< std::endl;
    std::cout<< "sdm.Instance:"<<sdm._Instance<< std::endl;
    std::cout<< "sdm._Protocol:"<<sdm._Protocol<< std::endl;
    std::cout<< "sdm.parameter:"<<sdm._Parameters.size()<< std::endl;
    for (std::list<std::string>::iterator it=sdm._Parameters.begin(); it != sdm._Parameters.end(); ++it)
    {
      std::cout<<*it<<std::endl;
    }
#endif
    for(auto iter = creatorMap_compat.begin(); iter != creatorMap_compat.end(); ++iter) {
#ifdef _DEBUG
      std::cout<<"Pair:"<<iter->first.first<<"+"<<iter->first.second<<std::endl;
#endif
      if ( (iter->first.first == sdm._Interface) && (iter->first.second == sdm._Protocol) )
      {
        auto backend = (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters, deviceInfo.mapFileName);
        boost::weak_ptr<DeviceBackend>  weakBackend = backend;
        _existingBackends[deviceInfo.uri] = weakBackend;
        // return the shared pointer, not the weak pointer
        return backend;
      }
        //return (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters, deviceInfo.mapFileName);
    }

    throw ChimeraTK::logic_error("Unregistered device: Interface = "+sdm._Interface+" Protocol = "+sdm._Protocol);
      return boost::shared_ptr<DeviceBackend>(); //won't execute
  }

  /********************************************************************************************************************/

  void BackendFactory::loadPluginLibrary(std::string soFile){
    // Create a copy of the original creator map. If we unload the library because the signature is not
    // recognised we have to restore the original map because it could be an old backed that has a working
    // registerer (or the programmer forgot the signature function). In this case we would leave the
    // factory with a dangling function pointer which causes a segfault if the backend is opened.
    auto originalCreatorMap = creatorMap;

    // first do an open which does not load the symbols yet
    // FIXME: Only works for functions, not for variables = registerer :-(
    void *hndl = dlopen(soFile.c_str() , RTLD_LAZY );
    if(hndl == NULL){
      throw ChimeraTK::logic_error(dlerror());
    }

    // try to find the symbol for the version function.
    const char * (*versionFunction)();
    // We have to use an instance of the function pointer with the right signature
    // and reinterpred cast it because dlsym is giving out a void pointer which in
    // pedantic C++ cannot be casted directly to a function pointer.
    *reinterpret_cast<void**>(&versionFunction) = dlsym(hndl, "deviceAccessVersionUsedToCompile");

    if (versionFunction == NULL){
      creatorMap = originalCreatorMap;
      dlclose(hndl);
      std::stringstream errorMessage;
      errorMessage << "Symbol 'deviceAccessVersionUsedToCompile' not found in " <<soFile;
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    if (std::string(versionFunction()) != CHIMERATK_DEVICEACCESS_VERSION){
      std::stringstream errorMessage;
      errorMessage << soFile << " was compiled with the wrong DeviceAccess version " << versionFunction()
                   << ". Recompile with DeviceAccess version " << CHIMERATK_DEVICEACCESS_VERSION;
      // Do not restore the original creator map. If the registerer worked there is a function which throws an
      // exception that tells the user that the backend has been compiled against the wrong version.
      // This is better than just printing a warning because the exception which is thrown here is caught
      // if the library is loaded via dmap file (to keep dmap files working if broken backends are not used).
      dlclose(hndl);
      throw ChimeraTK::logic_error(errorMessage.str());
    }

    // it is a correct plugin, load all the symbols now
    dlopen(soFile.c_str() , RTLD_NOW);
  }

  /********************************************************************************************************************/

  void BackendFactory::loadAllPluginsFromDMapFile(){
    if (_dMapFile.empty()){
      return;
    }

    auto dmap = DMapFileParser().parse(_dMapFile);

    for ( auto lib : dmap->getPluginLibraries() ){
      try{
        loadPluginLibrary(lib);
      }catch(ChimeraTK::logic_error &e){
        //Ignore library loading errors when doing this automatically for all plugins in the dmap file
        //to keep dmap files usable if the broken backends are not used.
        //Print a warning that the loading failed.
        std::cerr << "Error: Caught exception loading plugin '"<< lib <<"' specified in dmap file: "
                  << e.what() << std::endl;
        std::cerr << "Some backends will not be available!" << std::endl;
      }
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> BackendFactory::failedRegistrationThrowerFunction(
    std::string /*host*/, std::string /*instance*/, std::list<std::string> /*parameters*/, std::string /*mapFileName*/, std::string exception_what){
    throw ChimeraTK::logic_error(exception_what);
  }
} // namespace ChimeraTK
