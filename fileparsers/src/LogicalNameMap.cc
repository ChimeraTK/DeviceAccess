/*
 * LogicalNameMap.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <stdexcept>
#include <libxml++/libxml++.h>

#include "LogicalNameMap.h"
#include "DeviceException.h"

namespace mtca4u {

  std::string LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text.");
    }

    const xmlpp::TextNode *textNode = dynamic_cast<xmlpp::TextNode*>(childList.front());
    if(!textNode) {
      parsingError("Node '"+subnodeName+"' does not contain text.");
    }

    return textNode->get_content();

  }

  LogicalNameMap::LogicalNameMap(const std::string &fileName)
  : _fileName(fileName)
  {

    // parse the file into a DOM structure
    xmlpp::DomParser parser;
    try {
      parser.parse_file(fileName);
    }
    catch(xmlpp::exception &e) {
      throw DeviceException("Error opening the xlmap file '"+fileName+"': "+e.what(), DeviceException::CANNOT_OPEN_MAP_FILE);
    }

    // get root element
    const auto root = parser.get_document()->get_root_node();
    if(root->get_name() != "logicalNameMap") {
      parsingError("Expected 'logicalNameMap' tag instead of: "+root->get_name());
    }

    // parsing loop
    for(const auto& child : root->get_children()) {
      const xmlpp::Element *element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) continue;    // comment etc.

      // check if entry tag found
      if(element->get_name() != "entry") {
        parsingError("Expected 'entry' tag instead of: "+root->get_name());
      }

      // obtain name of entry
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError("Missing name attribute of 'entry' tag.");
      }
      std::string entryName = nameAttr->get_value();

      // obtain the type
      std::string type = getValueFromXmlSubnode(child, "type");
      if(type == "register") {
        _map[entryName].targetType = TargetType::REGISTER;
        _map[entryName].deviceName = getValueFromXmlSubnode(child, "device");
        _map[entryName].registerName = getValueFromXmlSubnode(child, "register");
      }
      else if(type == "range") {
        _map[entryName].targetType = TargetType::RANGE;
        _map[entryName].deviceName = getValueFromXmlSubnode(child, "device");
        _map[entryName].registerName = getValueFromXmlSubnode(child, "register");
        _map[entryName].firstIndex = std::stoi(getValueFromXmlSubnode(child, "index"));
        _map[entryName].length = std::stoi(getValueFromXmlSubnode(child, "length"));
      }
      else if(type == "channel") {
        _map[entryName].targetType = TargetType::CHANNEL;
        _map[entryName].deviceName = getValueFromXmlSubnode(child, "device");
        _map[entryName].registerName = getValueFromXmlSubnode(child, "register");
        _map[entryName].channel = std::stoi(getValueFromXmlSubnode(child, "channel"));
      }
      else if(type == "int_constant") {
        _map[entryName].targetType = TargetType::INT_CONSTANT;
        _map[entryName].value = std::stoi(getValueFromXmlSubnode(child, "value"));
      }
      else {
        parsingError("Wrong target type: "+type);
      }
    }

  }

  /********************************************************************************************************************/

  const LogicalNameMap::RegisterInfo& LogicalNameMap::getRegisterInfo(const std::string &name) const {
    try {
      return _map.at(name);
    }
    catch(std::out_of_range &e) {
      throw DeviceException("Register '"+name+"' was not found in logical name map ("+e.what()+").",
          DeviceException::REGISTER_DOES_NOT_EXIST);
    }
  }

  /********************************************************************************************************************/

  std::unordered_set<std::string> LogicalNameMap::getTargetDevices() const {
    std::unordered_set<std::string> ret;
    for(auto it = _map.begin(); it != _map.end(); ++it) {
      if(it->second.hasDeviceName()) {
        ret.insert(it->second.deviceName);
      }
    }
    return ret;
  }

  /********************************************************************************************************************/

  void LogicalNameMap::parsingError(const std::string &message) {
    throw DeviceException("Error parsing the xlmap file '"+_fileName+"': "+message, DeviceException::CANNOT_OPEN_MAP_FILE);
  }

} // namespace mtca4u
