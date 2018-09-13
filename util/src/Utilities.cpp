/*
 * Utilities.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#include <execinfo.h>
#include <cxxabi.h>

#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "Utilities.h"
#include "DMapFilesParser.h"
#include "BackendFactory.h"

namespace ChimeraTK {

  size_t Utilities::countOccurence(std::string theString, char delimiter)
  {
    size_t  count = std::count(theString.begin(), theString.end(), delimiter);
    return count;
  }

  bool Utilities::isSdm(std::string sdmString) {
    try
    {
      parseSdm(sdmString);
    }
    catch(ChimeraTK::logic_error &ue)
    {
      return false;
    }
    return true;
  }

  Sdm Utilities::parseSdm(std::string sdmString) {
    Sdm sdmInfo;
    size_t signatureLen = 6;
    if(sdmString.empty()) throw ChimeraTK::logic_error("Invalid sdm.");
    if(sdmString.length() < signatureLen) throw ChimeraTK::logic_error("Invalid sdm.");
    if(sdmString.substr(0,6) !="sdm://") throw ChimeraTK::logic_error("Invalid sdm.");
    int pos = 6;

    std::size_t found = sdmString.find_first_of("/", pos);
    std::string subUri;
    if (found != std::string::npos)
    {
      sdmInfo._Host =  sdmString.substr(pos, found - pos); // Get the Host
    }
    else
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if (sdmString.length() < found+1)
      return sdmInfo;
    subUri = sdmString.substr(found+1);
    //let's do a sanity check, only one delimiter occurrence is allowed.
    if (countOccurence(subUri,':') > 1) /* check ':' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if (countOccurence(subUri,';') > 1) /* check ';' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if (countOccurence(subUri,'=') > 1) /* check '=' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    std::vector<std::string> tokens;
    boost::split(tokens, subUri, boost::is_any_of(":;="));
    int numOfTokens = tokens.size();
    if (numOfTokens < 1)
      return sdmInfo;
    int counter = 0;
    sdmInfo._Interface = tokens[counter]; // Get the Interface
    counter++;
    if (counter < numOfTokens)
    {
      // Get the Instance
      found = sdmString.find_first_of(":", pos);
      if (found != std::string::npos) {
        sdmInfo._Instance = tokens[counter];
        counter++;
      }
    }
    if (counter < numOfTokens)
    {
      // Get the Protocol
      found = sdmString.find_first_of(";", pos);
      if (found != std::string::npos) {
        sdmInfo._Protocol = tokens[counter];
        counter++;
      }
    }
    if (counter < numOfTokens)
    {
      // Get the Parameters
      found = sdmString.find_first_of("=", pos);
      if (found != std::string::npos) {
        std::string parameters = tokens[counter];
        std::vector<std::string> paramterTokens;
        boost::split(paramterTokens, parameters, boost::is_any_of(","));
        for (uint i=0; i < paramterTokens.size();i++ )
          sdmInfo._Parameters.push_back(paramterTokens[i]);
      }
    }

    return sdmInfo;

  }

  Sdm Utilities::parseDeviceString(std::string deviceString) {

    Sdm sdmInfo;
    if (deviceString.substr(0,5)=="/dev/")
    {
      sdmInfo._Interface = "pci";
      if (deviceString.length() > 5)
      {
        sdmInfo._Instance = deviceString.substr(5);
      }
    }
    else if ( (boost::ends_with(deviceString, ".map")) || (boost::ends_with(deviceString, ".mapp")) )
    {
      sdmInfo._Interface = "dummy";
      sdmInfo._Instance = deviceString;
      /*another change in interface requires now instance
                to be ignored and old expect old Instance parameter
                as firt item of the Parameters list*/
      sdmInfo._Parameters.push_back(sdmInfo._Instance);
    }
    else
      return sdmInfo;
    sdmInfo._Host = ".";
    sdmInfo._Protocol = "";
    return sdmInfo;
  }


  DeviceInfoMap::DeviceInfo Utilities::aliasLookUp(std::string aliasName, std::string dmapFilePath)
  {
    DMapFileParser fileParser;
    DeviceInfoMap::DeviceInfo deviceInfo;
    auto deviceInfoPointer = fileParser.parse(dmapFilePath);
    deviceInfoPointer->getDeviceInfo(aliasName, deviceInfo);
    return deviceInfo;
  }

  std::vector<std::string> Utilities::getAliasList() {

    std::string dmapFileName = getDMapFilePath();
    if(dmapFileName.empty()){
      throw ChimeraTK::logic_error("Dmap file not set");
    }

    DMapFileParser fileParser;

    try {
      auto deviceInfoMap = fileParser.parse(dmapFileName);

      std::vector<std::string> listOfDeviceAliases;
      listOfDeviceAliases.reserve(deviceInfoMap->getSize());

      for (auto&& deviceInfo : *deviceInfoMap) {
        listOfDeviceAliases.push_back(deviceInfo.deviceName);
      }

      return listOfDeviceAliases;
    }
    catch (Exception& e) {
      std::cout << e.what() << std::endl;
      return std::vector<std::string>(); // empty list in case of failure
    }
  }

  std::string getDMapFilePath() {
    return (BackendFactory::getInstance().getDMapFilePath());
  }

  void setDMapFilePath(std::string dmapFilePath) {
    BackendFactory::getInstance().setDMapFilePath(dmapFilePath);
  }

  void Utilities::printStackTrace() {

    void *trace[16];
    char **messages;
    int i, trace_size = 0;

    trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    printf("[bt] Execution path:\n");
    for (i=0; i<trace_size; ++i) {
      std::string msg(messages[i]);
      size_t a = msg.find_first_of("(");
      size_t b = msg.find_first_of("+");
      std::string functionName = msg.substr(a+1,b-a-1);
      int status;
      char *demangledName = abi::__cxa_demangle(functionName.c_str(), nullptr, 0, &status);
      if(status == 0) {
        std::cout << "[bt] #" << i << " " << demangledName << std::endl;
        free(demangledName);
      }
      else {
        std::cout << "[bt] #" << i << " (demangling failed) " << functionName << std::endl;
      }
    }
  }

} /* namespace ChimeraTK */
