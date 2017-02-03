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
#include "LogicalNameMappingBackend.h"
#include "DMapFileParser.h"
#include "DMapFileDefaults.h"
#include "DeviceException.h"
#include "DeviceAccessVersion.h"
#include <dlfcn.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace ChimeraTK {

  void BackendFactory::registerBackendType(std::string interface, std::string protocol,
     boost::shared_ptr<DeviceBackend> (*creatorFunction)(std::string host, std::string instance,std::list<std::string>parameters, std::string mapFileName),
     std::string version)
  {
#ifdef _DEBUG
    std::cout << "adding:" << interface << std::endl << std::flush;
#endif
    if (version != CHIMERATK_DEVICEACCESS_VERSION){
      // register a function that throws an exception with the message when trying to create a backend
      std::stringstream errorMessage;
      errorMessage << "Backend plugin '"<< interface << "' compiled with wrong DeviceAccess version "
		   << version <<". Please recompile with version " << CHIMERATK_DEVICEACCESS_VERSION;
      creatorMap[make_pair(interface,protocol)] =
        boost::bind(BackendFactory::failedRegistrationThrowerFunction,_1,_2,_3,_4,errorMessage.str());
      return;
    }
    creatorMap[make_pair(interface,protocol)] = creatorFunction;
  }

  void BackendFactory::setDMapFilePath(std::string dMapFilePath)
  {
    _dMapFile = dMapFilePath;
  }

  std::string BackendFactory::getDMapFilePath()
  {
    return _dMapFile;
  }

  BackendFactory::BackendFactory(){
    registerBackendType("pci","",&PcieBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("pci","pcie",&PcieBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("dummy","",&DummyBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("rebot","",&RebotBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    registerBackendType("logicalNameMap","",&LogicalNameMappingBackend::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
  }

  BackendFactory & BackendFactory::getInstance(){
#ifdef _DEBUG
    std::cout << "getInstance" << std::endl << std::flush;
#endif
    static BackendFactory factoryInstance; /** Thread safe in C++11*/
    return factoryInstance;
  }

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackend(
      std::string aliasOrUri) {
    
    std::lock_guard<std::mutex> lock(_mutex);

    loadAllPluginsFromDMapFile();
    
    if(aliasOrUri.substr(0, 6) == "sdm://"){
      // it is a URI, directly create a deviceinfo and call the internal creator function
      DeviceInfoMap::DeviceInfo deviceInfo;
      deviceInfo.uri = aliasOrUri;
      return createBackendInternal(deviceInfo);
    }

    // it's not a URI. Try finding the alias in the dmap file.
    if (_dMapFile.empty()) {
      throw DeviceException("DMap file not set.",
                            DeviceException::NO_DMAP_FILE);
    }
    auto deviceInfo = Utilities::aliasLookUp(aliasOrUri, _dMapFile);
    return createBackendInternal(deviceInfo);
  }

  boost::shared_ptr<DeviceBackend> BackendFactory::createBackendInternal(const DeviceInfoMap::DeviceInfo &deviceInfo) {
#ifdef _DEBUG
    std::cout << "uri to parse" << deviceInfo.uri << std::endl;
    std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif
    auto iterator = _existingBackends.find(deviceInfo.uri);
    if (iterator != _existingBackends.end())
    {
      if(!iterator->second.expired())
      {
        return iterator->second.lock();
      }
    }
    Sdm sdm;
    try {
      sdm = Utilities::parseSdm(deviceInfo.uri);
    }
    catch(SdmUriParseException &e){
      //fixme: the programme flow should not use exceptions here. It is a supported
      // condition that the old device syntax is used.

      //parseDeviceString currently does not throw
      sdm = Utilities::parseDeviceString(deviceInfo.uri);
      // todo: enable the deprecated warning. As long as most servers are still using MtcaMappedDevice
      // DMap files have to stay with device node, but QtHardMon would print the message, which causes confusion.
      //std::cout<<"# Using the device node in a dmap file is deprecated. Please change to sdm if applicable."<<std::endl;
    }
#ifdef _DEBUG
    std::cout<< "sdm._SdmVersion:"<<sdm._SdmVersion<< std::endl;
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
    for(auto iter = creatorMap.begin(); iter != creatorMap.end(); ++iter) {
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

    throw BackendFactoryException("Unregistered device: Interface = "+sdm._Interface+" Protocol = "+sdm._Protocol,
                                  BackendFactoryException::UNREGISTERED_DEVICE);
      return boost::shared_ptr<DeviceBackend>(); //won't execute
    }

  void BackendFactory::loadPluginLibrary(std::string soFile){
    // first do an open which does not load the symbols yet
    // FIXME: Only works for functions, not for variables = registerer :-(
    void *hndl = dlopen(soFile.c_str() , RTLD_LAZY );
    if(hndl == NULL){
      throw DeviceException( dlerror(), DeviceException::WRONG_PARAMETER);
    }

    // try to find the symbol for the version function.
    std::string (*versionFunction)();
    // We have to use an instance of the function pointer with the right signature
    // and reinterpred cast it because dlsym is giving out a void pointer which in
    // pedantic C++ cannot be casted directly to a function pointer.
    *reinterpret_cast<void**>(&versionFunction) = dlsym(hndl, "versionUsedToCompile");

    if (versionFunction == NULL){
      dlclose(hndl);
      std::stringstream errorMessage;
      errorMessage << "Symbol 'versionUsedToCompile' not found in " <<soFile;
      throw DeviceException( errorMessage.str(), DeviceException::WRONG_PARAMETER);
    }

    if (versionFunction() != CHIMERATK_DEVICEACCESS_VERSION){
      std::stringstream errorMessage;
      errorMessage << soFile << " was compiled with the wrong DeviceAccess version " << versionFunction()
		   << ". Recompile with DeviceAccess version " << CHIMERATK_DEVICEACCESS_VERSION;
      dlclose(hndl);
      throw DeviceException( errorMessage.str(), DeviceException::WRONG_PARAMETER);
    }

    // it is a correct plugin, load all the symbols now
    hndl = dlopen(soFile.c_str() , RTLD_NOW);
  }
  
  void BackendFactory::loadAllPluginsFromDMapFile(){
    if (_dMapFile.empty()){
      return;
    }

    auto dmap = DMapFileParser().parse(_dMapFile);

    for ( auto lib : dmap->getPluginLibraries() ){
      loadPluginLibrary(lib);
    }
  }

   boost::shared_ptr<DeviceBackend> BackendFactory::failedRegistrationThrowerFunction(
     std::string /*host*/, std::string /*instance*/, std::list<std::string> /*parameters*/, std::string /*mapFileName*/, std::string exception_what){
     throw BackendFactoryException(exception_what, BackendFactoryException::UNREGISTERED_DEVICE);
   }
} // namespace ChimeraTK
