// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Utilities.h"

#include "BackendFactory.h"
#include "DMapFileParser.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <cxxabi.h>
#include <execinfo.h>
#include <utility>
#include <vector>

namespace ChimeraTK {

  /********************************************************************************************************************/

  size_t Utilities::countOccurence(std::string theString, char delimiter) {
    size_t count = std::count(theString.begin(), theString.end(), delimiter);
    return count;
  }

  /********************************************************************************************************************/

  bool Utilities::isSdm(const std::string& theString) {
    size_t signatureLen = 6;
    if(theString.length() < signatureLen) return false;
    if(theString.substr(0, 6) != "sdm://") return false;
    return true;
  }

  /********************************************************************************************************************/

  bool Utilities::isDeviceDescriptor(std::string theString) {
    boost::trim(theString);
    if(theString.length() < 3) return false;
    if(theString.substr(0, 1) != "(") return false;
    if(theString.substr(theString.length() - 1, 1) != ")") return false;
    return true;
  }

  /********************************************************************************************************************/

  DeviceDescriptor Utilities::parseDeviceDesciptor(std::string cddString) {
    DeviceDescriptor result;

    // first trim the string to remove whitespaces before and after the outer
    // parentheses
    boost::trim(cddString);

    // simple initial checks
    if(cddString.length() < 3) {
      throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (too short): " + cddString);
    }
    if(cddString.substr(0, 1) != "(") {
      throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (missing opening parenthesis): " + cddString);
    }
    if(cddString.substr(cddString.length() - 1, 1) != ")") {
      throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (missing opening parenthesis): " + cddString);
    }

    // walk through string
    size_t parenthesesLevel = 0;
    enum class tokenType { backendType, address, parameters };
    tokenType currentTokenType = tokenType::backendType;
    bool escapeNext = false;
    std::string token;
    size_t positionPlusOne = 0;
    for(auto& c : cddString) {
      ++positionPlusOne;
      // are we indide the outer main parenthesis but not within a deeper level?
      if(parenthesesLevel == 1) {
        // should the current character be escaped?
        if(escapeNext) {
          if(c == ' ' || c == '?' || c == '&' || c == '(' || c == ')' || c == '\\') {
            token += c;
          }
          else {
            throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (bad escape character): " + cddString);
          }
          escapeNext = false;
          continue; // no further parsing of this character!
        }
        // current character should be parsed normally
        if(c == '\\') {
          // escape next character
          escapeNext = true;
        }
        else if(currentTokenType == tokenType::backendType && (c == ':' || c == '?' || c == ')')) {
          // backendType token complete
          boost::trim(token);
          if(token.length() == 0) {
            throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor "
                                         "(backend type must be non-empty): " +
                cddString);
          }
          if(!boost::all(token, boost::is_alnum())) {
            throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor "
                                         "(backend type must be alphanumeric): " +
                cddString);
          }
          result.backendType = token;
          token = "";
          // determine next token type by checking the delimiter (')' will be
          // processed below)
          if(c == ':') {
            currentTokenType = tokenType::address;
          }
          else if(c == '?') {
            currentTokenType = tokenType::parameters;
          }
        }
        else if(currentTokenType == tokenType::address && (c == '?' || c == ')')) {
          // address token complete
          boost::trim(token);
          result.address = token;
          token = "";
          currentTokenType = tokenType::parameters; //  ')' will be processed below
        }
        else if(currentTokenType == tokenType::parameters && (c == '&' || c == ')')) {
          // parameter token complete (for one key-value pair)
          boost::trim(token);
          if(token.length() > 0) { // ignore empty parameter
            auto equalSign = token.find_first_of('=');
            if(equalSign == std::string::npos) {
              throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (parameters must be "
                                           "specified as key=value pairs): " +
                  cddString);
            }
            auto key = token.substr(0, equalSign);
            auto value = token.substr(equalSign + 1);
            boost::trim(key);
            if(key.length() == 0) {
              throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (parameter key names must "
                                           "not be empty): " +
                  cddString);
            }
            if(!boost::all(key, boost::is_alnum())) {
              throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (parameter key names must "
                                           "contain only "
                                           "alphanumeric characters): " +
                  cddString);
            }
            boost::trim(value);
            if(result.parameters.find(key) != result.parameters.end()) {
              throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (parameter '" + key +
                  "' specified multiple times): " + cddString);
            }
            result.parameters[key] = value;
            token = "";
          }
        }
        else {
          token += c;
        }
      }
      else if(parenthesesLevel > 1) {
        // deeper level: add all characters to current token
        token += c;
      }
      if(c == '(') {
        ++parenthesesLevel;
      }
      else if(c == ')') {
        --parenthesesLevel;
        if(parenthesesLevel == 0 && positionPlusOne != cddString.length()) {
          // main parenthesis closed but not yet end of the string
          throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (additional characters after "
                                       "last closing parenthesis): " +
              cddString);
        }
      }
    }

    // check if final parenthesis found
    if(parenthesesLevel != 0) {
      throw ChimeraTK::logic_error("Invalid ChimeraTK device descriptor (unmatched parenthesis): " + cddString);
    }

    // return the result
    return result;
  }

  /********************************************************************************************************************/

  Sdm Utilities::parseSdm(const std::string& sdmString) {
    Sdm sdmInfo;
    size_t signatureLen = 6;
    if(sdmString.length() < signatureLen) throw ChimeraTK::logic_error("Invalid sdm.");
    if(sdmString.substr(0, 6) != "sdm://") throw ChimeraTK::logic_error("Invalid sdm.");
    int pos = 6;

    std::size_t found = sdmString.find_first_of('/', pos);
    std::string subUri;
    if(found != std::string::npos) {
      sdmInfo.host = sdmString.substr(pos, found - pos); // Get the Host
    }
    else {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if(sdmString.length() < found + 1) return sdmInfo;
    subUri = sdmString.substr(found + 1);
    // let's do a sanity check, only one delimiter occurrence is allowed.
    if(countOccurence(subUri, ':') > 1) /* check ':' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if(countOccurence(subUri, ';') > 1) /* check ';' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    if(countOccurence(subUri, '=') > 1) /* check '=' */
    {
      throw ChimeraTK::logic_error("Invalid sdm.");
    }
    std::vector<std::string> tokens;
    boost::split(tokens, subUri, boost::is_any_of(":;="));
    size_t numOfTokens = tokens.size();
    if(numOfTokens < 1) return sdmInfo;
    size_t counter = 0;
    sdmInfo.interface = tokens[counter]; // Get the Interface
    counter++;
    if(counter < numOfTokens) {
      // Get the Instance
      found = sdmString.find_first_of(':', pos);
      if(found != std::string::npos) {
        sdmInfo.instance = tokens[counter];
        counter++;
      }
    }
    if(counter < numOfTokens) {
      // Get the Protocol
      found = sdmString.find_first_of(';', pos);
      if(found != std::string::npos) {
        sdmInfo.protocol = tokens[counter];
        counter++;
      }
    }
    if(counter < numOfTokens) {
      // Get the Parameters
      found = sdmString.find_first_of('=', pos);
      if(found != std::string::npos) {
        std::string parameters = tokens[counter];
        std::vector<std::string> paramterTokens;
        boost::split(paramterTokens, parameters, boost::is_any_of(","));
        for(auto& paramterToken : paramterTokens) {
          sdmInfo.parameters.push_back(paramterToken);
        }
      }
    }

    return sdmInfo;
  }

  /********************************************************************************************************************/

  Sdm Utilities::parseDeviceString(const std::string& deviceString) {
    Sdm sdmInfo;
    if(deviceString.substr(0, 5) == "/dev/") {
      sdmInfo.interface = "pci";
      if(deviceString.length() > 5) {
        sdmInfo.instance = deviceString.substr(5);
      }
    }
    else if((boost::ends_with(deviceString, ".map")) || (boost::ends_with(deviceString, ".mapp"))) {
      sdmInfo.interface = "dummy";
      sdmInfo.instance = deviceString;
      /*another change in interface requires now instance
                to be ignored and old expect old Instance parameter
                as firt item of the Parameters list*/
      sdmInfo.parameters.push_back(sdmInfo.instance);
    }
    else {
      return sdmInfo;
    }
    sdmInfo.host = ".";
    sdmInfo.protocol = "";
    return sdmInfo;
  }

  /********************************************************************************************************************/

  DeviceInfoMap::DeviceInfo Utilities::aliasLookUp(const std::string& aliasName, const std::string& dmapFilePath) {
    DeviceInfoMap::DeviceInfo deviceInfo;
    auto deviceInfoPointer = ChimeraTK::DMapFileParser::parse(dmapFilePath);
    deviceInfoPointer->getDeviceInfo(aliasName, deviceInfo);
    return deviceInfo;
  }

  /********************************************************************************************************************/

  std::vector<std::string> Utilities::getAliasList() {
    std::string dmapFileName = getDMapFilePath();
    if(dmapFileName.empty()) {
      throw ChimeraTK::logic_error("Dmap file not set");
    }

    try {
      auto deviceInfoMap = ChimeraTK::DMapFileParser::parse(dmapFileName);

      std::vector<std::string> listOfDeviceAliases;
      listOfDeviceAliases.reserve(deviceInfoMap->getSize());

      for(auto&& deviceInfo : *deviceInfoMap) {
        listOfDeviceAliases.push_back(deviceInfo.deviceName);
      }

      return listOfDeviceAliases;
    }
    catch(ChimeraTK::runtime_error& e) {
      std::cout << e.what() << std::endl;
      return {}; // empty list in case of failure
    }
  }

  /********************************************************************************************************************/

  std::string getDMapFilePath() {
    return (BackendFactory::getInstance().getDMapFilePath());
  }

  /********************************************************************************************************************/

  void setDMapFilePath(std::string dmapFilePath) {
    BackendFactory::getInstance().setDMapFilePath(std::move(dmapFilePath));
  }

  /********************************************************************************************************************/

  void Utilities::printStackTrace() {
    void* trace[16];
    char** messages;
    int i, trace_size = 0;

    trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    printf("[bt] Execution path:\n");
    for(i = 0; i < trace_size; ++i) {
      std::string msg(messages[i]);
      size_t a = msg.find_first_of('(');
      size_t b = msg.find_first_of('+');
      std::string functionName = boost::core::demangle(msg.substr(a + 1, b - a - 1).c_str());
      std::cout << "[bt] #" << i << " " << functionName << std::endl;
    }
  }

  /********************************************************************************************************************/

} /* namespace ChimeraTK */
