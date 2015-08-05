/*
 * Utilities.h
 *
 *  Created on: Aug 3, 2015
 *      Author: nshehzad
 */

#ifndef SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_
#define SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_
#include <string>
#include <list>
#include "ExcBase.h"
namespace mtca4u {
struct Sdm
{
	int _SdmVersion;
	std::string _Host;
	std::string _Interface;
	std::string _Instance;
	std::string _Protocol;
	std::list<std::string> _Parameters;
};

class UtilitiesException : public ExcBase {
public:
	enum {INVALID_SDM};
	UtilitiesException(const std::string &message, unsigned int exceptionID)
	: ExcBase( message, exceptionID ){}
};

class Utilities {

public:
	Utilities();
  Sdm static parseSdm(std::string sdmString);
	static size_t countOccurence(std::string theString, char delimiter);
  virtual ~Utilities();
};

} /* namespace mtca4u */

#endif /* SOURCE_DIRECTORY__DEVICES_INCLUDE_UTILITES_H_ */
