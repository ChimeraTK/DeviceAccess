/*
 * BackendFactory.cpp

 *
 *  Created on: Jul 9, 2015
 *      Author: nshehzad
 */

#include "Utilities.h"
#include "BackendFactory.h"
#include "MapFileParser.h"
#include "DMapFilesParser.h"
#include "DMapFileDefaults.h"
#include "Exception.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace mtca4u {

void BackendFactory::registerBackendType(std::string interface, std::string protocol,
		boost::shared_ptr<mtca4u::DeviceBackend> (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters))
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
  _dMapFile = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
  registerBackendType("pci","",&PcieBackend::createInstance);
  registerBackendType("pci","pcie",&PcieBackend::createInstance);
  registerBackendType("dummy","",&DummyBackend::createInstance);
}


boost::shared_ptr<DeviceBackend> BackendFactory::createBackend(std::string aliasName) {
  std::string uri;
  
  // check if an alias was found, if not try the environment variable
  char const* dmapFileFromEnvironment = std::getenv( DMAP_FILE_ENVIROMENT_VARIABLE.c_str());
  if ( dmapFileFromEnvironment != NULL ) {
    std::cout << "creating backend from " << dmapFileFromEnvironment << " for alias " << aliasName << std::endl;
    uri = Utilities::aliasLookUp(aliasName, dmapFileFromEnvironment)._deviceFile;
  }

  // try to get an alias of the DMap file set at run time by setDMapFilePath()
  if (uri.empty()){
    std::cout << "creating backend from " << _dMapFile << " for alias " << aliasName << std::endl;
    uri = Utilities::aliasLookUp(aliasName, _dMapFile)._deviceFile;
  }

  // finally try the system/compile time default
  if (uri.empty()){
    std::cout << "creating backend from " << DMAP_FILE_DEFAULT_DIRECTORY+  DMAP_FILE_DEFAULT_NAME 
	      << " for alias " << aliasName << std::endl;
    uri = Utilities::aliasLookUp(aliasName, DMAP_FILE_DEFAULT_DIRECTORY+  DMAP_FILE_DEFAULT_NAME)._deviceFile;
  }
  
  // if there still is no alias we are out of options and have to give up.
  if (uri.empty()){
    throw BackendFactoryException("Unknown device alias.", BackendFactoryException::UNKNOWN_ALIAS);
  }

  return createBackendInternal(uri);
}

boost::shared_ptr<DeviceBackend> BackendFactory::createBackendInternal(std::string uri) {
#ifdef _DEBUG
  std::cout << "uri to parse" << uri << std::endl;
  std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif
  Sdm sdm;
  try {
    sdm = Utilities::parseSdm(uri);
  }
  catch(UtilitiesException &e)
  {
    std::cout<<e.what()<<std::endl;
    //parseDeviceString currently does not throw
    sdm = Utilities::parseDeviceString(uri);
    std::cout<<"This format is obsolete, please change to sdm."<<std::endl;
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
  for (std::map< std::pair<std::string, std::string>, boost::shared_ptr<mtca4u::DeviceBackend> (*)(std::string host, std::string instance, std::list<std::string>parameters)>::iterator iter =
      creatorMap.begin();
      iter != creatorMap.end(); ++iter) {
#ifdef _DEBUG
    std::cout<<"Pair:"<<iter->first.first<<"+"<<iter->first.second<<std::endl;
#endif
    if ( (iter->first.first == sdm._Interface) && (iter->first.second == sdm._Protocol) )
      return (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters);
    }

  throw BackendFactoryException("Unregistered device.", BackendFactoryException::UNREGISTERED_DEVICE);
  return boost::shared_ptr<DeviceBackend>(); //won't execute
}

} // namespace mtca4u
