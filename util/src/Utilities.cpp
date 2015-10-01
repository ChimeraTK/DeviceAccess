/*
 * Utilities.cpp
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#include "Utilities.h"
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
	catch(UtilitiesException &ue)
	{
		return false;
	}
	return true;
}

Sdm Utilities::parseSdm(std::string sdmString) {
	Sdm sdmInfo;
	size_t signatureLen = 6;
	if (sdmString.empty())
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	if (sdmString.length() < signatureLen)
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	if (sdmString.substr(0,6) !="sdm://")
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	int pos = 6;

	std::size_t found = sdmString.find_first_of("/", pos);
	std::string subUri;
	if (found != std::string::npos)
	{
		sdmInfo._Host =  sdmString.substr(pos, found - pos); // Get the Host
	}
	else
	{
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	}
	if (sdmString.length() < found+1)
		return sdmInfo;
	subUri = sdmString.substr(found+1);
	//let's do a sanity check, only one delimiter occurrence is allowed.
	if (countOccurence(subUri,':') > 1) /* check ':' */
	{
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	}
	if (countOccurence(subUri,';') > 1) /* check ';' */
	{
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
	}
	if (countOccurence(subUri,'=') > 1) /* check '=' */
	{
		throw UtilitiesException("Invalid sdm.", UtilitiesException::INVALID_SDM);
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

} /* namespace mtca4u */
