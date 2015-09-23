/*
 * Utilities.h
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_
#define SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_

#include <list>
#include "Exception.h"
namespace mtca4u {
/**This structure holds the information of an SDM.
 *
 */
struct Sdm
{
	double _SdmVersion;
	std::string _Host;
	std::string _Interface;
	std::string _Instance;
	std::string _Protocol;
	std::list<std::string> _Parameters;
	Sdm():_SdmVersion(0.1){}
};
/** A class to provide exceptions to the Utilities."
 *
 */
class UtilitiesException : public Exception {
public:
	enum {INVALID_SDM};
	UtilitiesException(const std::string &message, unsigned int exceptionID)
	: Exception( message, exceptionID ){}
};

/** A class to provide generic useful function accross the library."
 *
 */
class Utilities {

public:
	Utilities();
	Sdm static parseSdm(std::string sdmString);
	Sdm static parseDeviceString(std::string deviceEntry);
	bool static isSdm(std::string theString);
	static size_t countOccurence(std::string theString, char delimiter);
	virtual ~Utilities();
};

} /* namespace mtca4u */

#endif /* SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_ */
