/*
 * devFactory.cpp

 *
 *  Created on: Jul 9, 2015
 *      Author: nshehzad
 */

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>

#include "DeviceFactory.h"

#include "MapFileParser.h"
#include "ExcBase.h"

namespace mtca4u {

MappedDevice<BaseDevice>* DeviceFactory::createMappedDevice(std::string devname) {
  BaseDevice* base;
  DMapFile::dmapElem elem;
  boost::tie(base, elem) = parseDMap(devname);
  base->openDev();
  return (new mtca4u::MappedDevice<mtca4u::BaseDevice>(base, elem.map_file_name));
}

BaseDevice* DeviceFactory::createDevice(std::string devname) {
  BaseDevice* base;
  DMapFile::dmapElem elem;
  boost::tie(base, elem) = parseDMap(devname);
  return base;
}

boost::tuple<BaseDevice*, DMapFile::dmapElem> DeviceFactory::parseDMap(
    std::string devname) {

  /**Functor would be another option*/
  /** Todo
   * open file here check if file exist, pass both params
   * read file e.g, specialDev sdm://./foo:S3
   * check if you have this uri device
   * return device and add special device to list of connected devices.
   * Device->open will be called without name of device.
   * check the handle against connected devices and try opening it.
   * rest is same.
   */
  std::vector<std::string> device_info;
  std::string uri;
  DMapFilesParser filesParser;
  DMapFile::dmapElem dmapElement;

  try {
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
    if (boost::iequals((*deviceIter).first.dev_name, devname)) {
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
    return boost::make_tuple((BaseDevice*)NULL, dmapElement);

#ifdef _DEBUG
  std::cout << "uri to look for" << uri << std::endl;
  std::cout << "Entries" << creatorMap.size() << std::endl << std::flush;
#endif
  for (std::map<std::string, BaseDevice* (*)(std::string)>::iterator iter =
           creatorMap.begin();
       iter != creatorMap.end(); ++iter) {
#ifdef _DEBUG
    std::cout << iter->first << " => " << iter->second << '\n';
#endif
    if (iter->first == uri) {
      std::string device_name = uriToNode(uri);
#ifdef _DEBUG
      std::cout << "device_name:" << device_name << std::endl;
#endif
      /*temporary hand converstion to file name*/
      if (device_name == "/../tests/mtcadummy_withoutModules.map")
        return boost::make_tuple(
            (iter->second)("../tests/mtcadummy_withoutModules.map"),
            dmapElement);
      else if (device_name == "/DummyDevice")
        return boost::make_tuple((iter->second)("DummyDevice"), dmapElement);
      else if (device_name == "/Reference_Device")
        return boost::make_tuple((iter->second)("Reference_Device"),
                                 dmapElement);
      else if (device_name == "/fakeDevice")
        return boost::make_tuple((iter->second)("fakeDevice"), dmapElement);
      else if (device_name == "/sequences")
        return boost::make_tuple((iter->second)("sequences.map"), dmapElement);
      if (device_name.length() > 0)
        return boost::make_tuple((iter->second)(device_name), dmapElement);
    }
  }
  return boost::make_tuple((BaseDevice*)NULL, dmapElement);
}

std::string DeviceFactory::uriToNode(std::string uri) {

  if (uri.length() < 6)
    return std::string();
  std::string Host;
  std::string Node;
  std::string instance;
  std::string port;

  int pos = 6;
  std::size_t found = uri.find_first_of("/", pos);
  if (found != std::string::npos) {
    Host = uri.substr(pos, found - pos);
#ifdef _DEBUG
    std::cout << "found" << found << std::endl;
    std::cout << "Host" << Host << std::endl;
#endif
    if (uri.length() > found + pos) {
      std::size_t comma = uri.find_first_of(",", found);
      if (comma != std::string::npos) {
        Node = uri.substr(found, comma - found);
#ifdef _DEBUG
        std::cout << "comma:" << comma << std::endl;
        std::cout << "Node:" << Node << std::endl;
#endif
        std::size_t colon = uri.find_first_of(":", comma);
        {
          if (colon != std::string::npos) {
            instance = uri.substr(comma + 1, colon - comma - 1);
            port = uri.substr(colon + 1, uri.length() - colon - 1);
#ifdef _DEBUG
            std::cout << "colon:" << colon << st d::endl;
            std::cout << "instance:" << instance << std::endl;
            std::cout << "port:" << port << std::endl;
#endif
          }
        }
      }
    }
  }
  Node = Node + port;
  if ((Host.length() == 0) || (Host == "."))
    return Node;
  return Host + Node;
}

} // namespace mtca4u
