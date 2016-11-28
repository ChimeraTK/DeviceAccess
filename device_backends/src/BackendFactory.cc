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
#include "DMapFilesParser.h"
#include "DMapFileDefaults.h"
#include "Exception.h"

namespace mtca4u {

  void BackendFactory::registerBackendType(std::string interface, std::string protocol,
      boost::shared_ptr<mtca4u::DeviceBackend> (*creatorFunction)(std::string host, std::string instance,std::list<std::string>parameters, std::string mapFileName))
  {
#ifdef _DEBUG
    std::cout << "adding:" << interface << std::endl << std::flush;
#endif
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
    registerBackendType("pci","",&PcieBackend::createInstance);
    registerBackendType("pci","pcie",&PcieBackend::createInstance);
    registerBackendType("dummy","",&DummyBackend::createInstance);
    registerBackendType("rebot","",&RebotBackend::createInstance); // FIXME: Do we use protocol for tmcb?
    registerBackendType("logicalNameMap","",&LogicalNameMappingBackend::createInstance);
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
      //backend could be closed by the time it is recieved.  
      return iterator->second;
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
        _existingBackends.insert({deviceInfo.uri, backend});
        return backend;
      }
        //return (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters, deviceInfo.mapFileName);
    }

    throw BackendFactoryException("Unregistered device: Interface = "+sdm._Interface+" Protocol = "+sdm._Protocol,
                                  BackendFactoryException::UNREGISTERED_DEVICE);
      return boost::shared_ptr<DeviceBackend>(); //won't execute
    }

} // namespace mtca4u
