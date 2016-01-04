/*
 * Utilities.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#include "Utilities.h"
#include "DMapFilesParser.h"
#include "BackendFactory.h"
#include "DMapFileDefaults.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace mtca4u {

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
	catch(SdmUriParseException &ue)
	{
		return false;
	}
	return true;
}

Sdm Utilities::parseSdm(std::string sdmString) {
	Sdm sdmInfo;
	size_t signatureLen = 6;
	if (sdmString.empty())
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	if (sdmString.length() < signatureLen)
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	if (sdmString.substr(0,6) !="sdm://")
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	int pos = 6;

	std::size_t found = sdmString.find_first_of("/", pos);
	std::string subUri;
	if (found != std::string::npos)
	{
		sdmInfo._Host =  sdmString.substr(pos, found - pos); // Get the Host
	}
	else
	{
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	}
	if (sdmString.length() < found+1)
		return sdmInfo;
	subUri = sdmString.substr(found+1);
	//let's do a sanity check, only one delimiter occurrence is allowed.
	if (countOccurence(subUri,':') > 1) /* check ':' */
	{
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	}
	if (countOccurence(subUri,';') > 1) /* check ';' */
	{
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
	}
	if (countOccurence(subUri,'=') > 1) /* check '=' */
	{
		throw SdmUriParseException("Invalid sdm.", SdmUriParseException::INVALID_SDM);
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
  DeviceInfoMap::DeviceInfo deviceInfo;
  DMapFilesParser filesParser;
  try {
    filesParser.parse_file(dmapFilePath);
  }
  catch (Exception& e) {
      std::cout << e.what() << std::endl;
      return deviceInfo;
  }
#ifdef _DEBUG
  for (DMapFilesParser::iterator deviceIter = filesParser.begin();
      deviceIter != filesParser.end(); ++deviceIter) {
    std::cout << (*deviceIter).first.dev_name << std::endl;
    std::cout << (*deviceIter).first.uri << std::endl;
    std::cout << (*deviceIter).first.map_file_name << std::endl;
    std::cout << (*deviceIter).first.dmap_file_name << std::endl;
    std::cout << (*deviceIter).first.dmap_file_line_nr << std::endl;
  }
#endif
  for (DMapFilesParser::iterator deviceIter = filesParser.begin();
      deviceIter != filesParser.end(); ++deviceIter) {
    if (boost::iequals((*deviceIter).first.deviceName, aliasName)) {
#ifdef _DEBUG
      std::cout << "found:" << (*deviceIter).first.uri << std::endl;
#endif
      deviceInfo = deviceIter->first;
      break;
    }
  }
  return deviceInfo;
}

std::string Utilities::findFirstOfAlias(std::string aliasName)
{
  std::string uri;
  char const* dmapFileFromEnvironment = std::getenv( DMAP_FILE_ENVIROMENT_VARIABLE.c_str());
  if ( dmapFileFromEnvironment != NULL ) {
    uri = Utilities::aliasLookUp(aliasName, dmapFileFromEnvironment).uri;
  }
  if (!uri.empty())
    return dmapFileFromEnvironment;

  std::string dMapFilePath = BackendFactory::getInstance().getDMapFilePath();
  uri = aliasLookUp(aliasName, dMapFilePath).uri;
  if (!uri.empty())
    return dMapFilePath;

  uri = aliasLookUp(aliasName, DMAP_FILE_DEFAULT_DIRECTORY + DMAP_FILE_DEFAULT_NAME).uri;
  if (!uri.empty())
    return DMAP_FILE_DEFAULT_DIRECTORY + DMAP_FILE_DEFAULT_NAME;

  return std::string(); // no alias found, return an empty string
}

std::string Utilities::getCurrentWorkingDirectory() {
  char *currentWorkingDir = get_current_dir_name();
  if (!currentWorkingDir) {
    throw std::runtime_error("Could not get the current working directory");
  }
  std::string dir(currentWorkingDir);
  free(currentWorkingDir);
  return dir + "/";
}
} /* namespace mtca4u */
