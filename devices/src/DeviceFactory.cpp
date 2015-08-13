/*
 * devFactory.cpp

 *
 *  Created on: Jul 9, 2015
 *      Author: nshehzad
 */

#include <boost/algorithm/string.hpp>
#include "Utilities.h"
#include "DeviceFactory.h"
#include "MapFileParser.h"
#include "Exception.h"

namespace mtca4u {

void DeviceFactory::registerDevice(std::string interface, std::string protocol,
		BaseDevice* (*creatorFunction)(std::string host, std::string instance, std::list<std::string>parameters))
{
#ifdef _DEBUG
	std::cout << "adding:" << interface << std::endl << std::flush;
#endif
	creatorMap[make_pair(interface,protocol)] = creatorFunction;
}

MappedDevice<BaseDevice>* DeviceFactory::createMappedDevice(std::string devname) {
	BaseDevice* base;
	DMapFile::dmapElem elem;
	boost::tie(base, elem) = parseDMap(devname);
	if (base != 0)
		base->openDev();
	return (new mtca4u::MappedDevice<mtca4u::BaseDevice>(base, elem.map_file_name));
}

BaseDevice* DeviceFactory::createDevice(std::string devname) {
	BaseDevice* base;
	DMapFile::dmapElem elem;
	boost::tie(base, elem) = parseDMap(devname);
	return base;
}


boost::tuple<BaseDevice*, DMapFile::dmapElem> DeviceFactory::parseDMap(std::string devName)
{
	std::vector<std::string> device_info;
	std::string uri;
	DMapFilesParser filesParser;
	DMapFile::dmapElem dmapElement;
	std::string testFilePath = boost::filesystem::initial_path().string() + (std::string)TEST_DMAP_FILE_PATH;
	try {
		if ( boost::filesystem::exists(testFilePath ) )
			filesParser.parse_file(testFilePath);
		else
			filesParser.parse_file(DMAP_FILE_PATH);
	}
	catch (ExcBase& e) {
		std::cout << e.what() << std::endl;
		return boost::make_tuple((BaseDevice*)NULL, dmapElement);
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
			dmapElement = (*deviceIter).first;
			found = true;
			break;
		}
	}
	if (!found)
	{
		// do not throw here because theoretically client could work with multiple unrelated devices
		//throw DeviceFactoryException("Unknown device alias.", DeviceFactoryException::UNKNOWN_ALIAS);
		return boost::make_tuple((BaseDevice*)NULL, dmapElement);
	}

#ifdef _DEBUG
	std::cout << "uri to parse" << uri << std::endl;
	std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif

	Sdm sdm = Utilities::parseSdm(uri);
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


	for (std::map< std::pair<std::string, std::string>, BaseDevice* (*)(std::string host, std::string instance, std::list<std::string>parameters)>::iterator iter =
			creatorMap.begin();
			iter != creatorMap.end(); ++iter) {

#ifdef _DEBUG
		std::cout<<"Pair:"<<iter->first.first<<"+"<<iter->first.second<<std::endl;
#endif

		if ( (iter->first.first == sdm._Interface) && (iter->first.second == sdm._Protocol) )
			return boost::make_tuple( (iter->second)(sdm._Host, sdm._Instance, sdm._Parameters), dmapElement);

	}
	throw DeviceFactoryException("Unregistered device.", DeviceFactoryException::UNREGISTERED_DEVICE);
	return boost::make_tuple((BaseDevice*)NULL, dmapElement); //won't execute
}

} // namespace mtca4u
