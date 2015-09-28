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
#include "Exception.h"
#include <boost/algorithm/string.hpp>

namespace mtca4u {

void BackendFactory::registerDeviceType(std::string interface, std::string protocol,
		boost::shared_ptr<mtca4u::DeviceBackend> (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters))
{
#ifdef _DEBUG
	std::cout << "adding:" << interface << std::endl << std::flush;
#endif
	creatorMap[make_pair(interface,protocol)] = creatorFunction;
}

boost::shared_ptr<DeviceBackend> BackendFactory::createBackend(std::string aliasName) {
	boost::shared_ptr<DeviceBackend> base;
	DMapFile::dRegisterInfo elem;
	boost::tie(base, elem) = parseDMap(aliasName);
	return base;
}

boost::tuple<boost::shared_ptr<DeviceBackend>, DMapFile::dRegisterInfo> BackendFactory::parseDMap(std::string devName)
{
	std::vector<std::string> device_info;
	std::string uri;	
	DMapFilesParser filesParser;
	DMapFile::dRegisterInfo dRegisterInfoent;
	std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
	//std::cout<<"testFilePath:"<<testFilePath<<std::endl;
	try {
		if ( boost::filesystem::exists(testFilePath ) )
			filesParser.parse_file(testFilePath);
		else
			filesParser.parse_file(DMAP_FILE_PATH);
	}
	catch (Exception& e) {
		std::cout << e.what() << std::endl;
		return boost::make_tuple(boost::shared_ptr<DeviceBackend>(), dRegisterInfoent);
	}

#ifdef _DEBUG
	for (DMapFilesParser::iterator deviceIter = filesParser.begin();
			deviceIter != filesParser.end(); ++deviceIter) {
		std::cout << (*deviceIter).first.dev_name << std::endl;
		std::cout << (*deviceIter).first.dev_file << std::endl;
		std::cout << (*deviceIter).first.map_file_name << std::endl;
		std::cout << (*deviceIter).first.dmap_file_name << std::endl;
		std::cout << (*deviceIter).first.dmap_file_line_nr << std::endl;
	}
#endif
	bool found = false;
	for (DMapFilesParser::iterator deviceIter = filesParser.begin();
			deviceIter != filesParser.end(); ++deviceIter) {
		if (boost::iequals((*deviceIter).first.dev_name, devName)) {
#ifdef _DEBUG
			std::cout << "found:" << (*deviceIter).first.dev_file << std::endl;
#endif
			uri = (*deviceIter).first.dev_file;
			dRegisterInfoent = (*deviceIter).first;
			found = true;
			break;
		}
	}
	if (!found)
	{
		// do not throw here because theoretically client could work with multiple unrelated devices
		throw BackendFactoryException("Unknown device alias.", BackendFactoryException::UNKNOWN_ALIAS);
		return boost::make_tuple(boost::shared_ptr<DeviceBackend>(), dRegisterInfoent);
	}

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
		try {
				sdm = Utilities::parseDeviceString(uri);
			}
			catch(UtilitiesException &ue)
			{
				std::cout<<ue.what()<<std::endl;
			}
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
			return boost::make_tuple( (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters), dRegisterInfoent);
		}

	throw BackendFactoryException("Unregistered device.", BackendFactoryException::UNREGISTERED_DEVICE);
	return boost::make_tuple(boost::shared_ptr<DeviceBackend>(), dRegisterInfoent); //won't execute
}

} // namespace mtca4u
