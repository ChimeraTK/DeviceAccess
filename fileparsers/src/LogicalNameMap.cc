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
#include "DeviceBackend.h"

namespace mtca4u {

  template<>
  LogicalNameMap::Value<std::string> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
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

    LogicalNameMap::Value<std::string> value;
    value = textNode->get_content();
    return value;
  }

  /********************************************************************************************************************/

  template<>
  LogicalNameMap::Value<int> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text or only one <ref></ref> sub-element.");
    }

    LogicalNameMap::Value<int> value;

    const xmlpp::TextNode *textNode = dynamic_cast<xmlpp::TextNode*>(childList.front());
    if(textNode) {
      value = std::stoi(textNode->get_content());
    }
    else {
      value.hasActualValue = false;
      value.registerName = getValueFromXmlSubnode<std::string>(list[0], "ref");
    }

    return value;
  }

  /********************************************************************************************************************/

  template<>
  LogicalNameMap::Value<unsigned int> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text.");
    }

    LogicalNameMap::Value<unsigned int> value;

    const xmlpp::TextNode *textNode = dynamic_cast<xmlpp::TextNode*>(childList.front());
    if(textNode) {
      value = std::stoi(textNode->get_content());
    }
    else {
      value.hasActualValue = false;
      value.registerName = getValueFromXmlSubnode<std::string>(list[0], "ref");
    }

    return value;
  }

  /********************************************************************************************************************/

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

      // create new RegisterInfo object
      _map[entryName].reset( new LogicalNameMap::RegisterInfo() );

      // obtain the type
      std::string type = getValueFromXmlSubnode<std::string>(child, "type");
      if(type == "register") {
        _map[entryName]->targetType = TargetType::REGISTER;
        _map[entryName]->deviceName = getValueFromXmlSubnode<std::string>(child, "device");
        _map[entryName]->registerName = getValueFromXmlSubnode<std::string>(child, "register");
      }
      else if(type == "range") {
        _map[entryName]->targetType = TargetType::RANGE;
        _map[entryName]->deviceName = getValueFromXmlSubnode<std::string>(child, "device");
        _map[entryName]->registerName = getValueFromXmlSubnode<std::string>(child, "register");
        _map[entryName]->firstIndex = getValueFromXmlSubnode<unsigned int>(child, "index");
        _map[entryName]->length = getValueFromXmlSubnode<unsigned int>(child, "length");
      }
      else if(type == "channel") {
        _map[entryName]->targetType = TargetType::CHANNEL;
        _map[entryName]->deviceName = getValueFromXmlSubnode<std::string>(child, "device");
        _map[entryName]->registerName = getValueFromXmlSubnode<std::string>(child, "register");
        _map[entryName]->channel = getValueFromXmlSubnode<unsigned int>(child, "channel");
      }
      else if(type == "int_constant") {
        _map[entryName]->targetType = TargetType::INT_CONSTANT;
        _map[entryName]->value = getValueFromXmlSubnode<int>(child, "value");
      }
      else if(type == "int_variable") {
        _map[entryName]->targetType = TargetType::INT_VARIABLE;
        _map[entryName]->value = getValueFromXmlSubnode<int>(child, "value");
      }
      else {
        parsingError("Wrong target type: "+type);
      }
    }

  }

  /********************************************************************************************************************/

  const LogicalNameMap::RegisterInfo& LogicalNameMap::getRegisterInfo(const std::string &name) const {
    try {
      return *(_map.at(name));
    }
    catch(std::out_of_range &e) {
      throw DeviceException("Register '"+name+"' was not found in logical name map ("+e.what()+").",
          DeviceException::REGISTER_DOES_NOT_EXIST);
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<LogicalNameMap::RegisterInfo> LogicalNameMap::getRegisterInfoShared(const std::string &name) {
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
      if(it->second->hasDeviceName()) {
        ret.insert(it->second->deviceName);
      }
    }
    return ret;
  }

  /********************************************************************************************************************/

  void LogicalNameMap::parsingError(const std::string &message) {
    throw DeviceException("Error parsing the xlmap file '"+_fileName+"': "+message, DeviceException::CANNOT_OPEN_MAP_FILE);
  }

  /********************************************************************************************************************/

  void LogicalNameMap::RegisterInfo::initAccessors(boost::shared_ptr<DeviceBackend> &backend) {
    if(!deviceName.hasActualValue && !deviceName.accessor) {
      deviceName.accessor = backend->getBufferingRegisterAccessor<std::string>("",deviceName.registerName);
    }
    if(!registerName.hasActualValue && !registerName.accessor) {
      registerName.accessor = backend->getBufferingRegisterAccessor<std::string>("",registerName.registerName);
    }
    if(!firstIndex.hasActualValue && !firstIndex.accessor) {
      firstIndex.accessor = backend->getBufferingRegisterAccessor<unsigned int>("",firstIndex.registerName);
    }
    if(!length.hasActualValue && !length.accessor) {
      length.accessor = backend->getBufferingRegisterAccessor<unsigned int>("",length.registerName);
    }
    if(!channel.hasActualValue && !channel.accessor) {
      channel.accessor = backend->getBufferingRegisterAccessor<unsigned int>("",channel.registerName);
    }
    if(!value.hasActualValue && !value.accessor) {
      value.accessor = backend->getBufferingRegisterAccessor<int>("",value.registerName);
    }
  }

} // namespace mtca4u
