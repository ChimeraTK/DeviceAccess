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
#include "RegisterPluginFactory.h"

namespace mtca4u {

  template<>
  Value<std::string> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
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

    Value<std::string> value;
    value = textNode->get_content();
    return value;
  }

  /********************************************************************************************************************/

  template<>
  Value<int> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text or only one <ref></ref> sub-element.");
    }

    Value<int> value;

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
  Value<unsigned int> LogicalNameMap::getValueFromXmlSubnode(const xmlpp::Node *node, const std::string &subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text.");
    }

    Value<unsigned int> value;

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

  void LogicalNameMap::parseFile(const std::string &fileName) {
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
      // cast into element, ignore if not an element (e.g. comment)
      const xmlpp::Element *element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) continue;

      // parse the element
      parseElement(RegisterPath(),element);
    }
  }

  void LogicalNameMap::parseElement(RegisterPath currentPath, const xmlpp::Element *element) {
    // module tag found: look for registers and sub-modules in module
    if(element->get_name() == "module") {

      // obtain name of the module
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError("Missing name attribute of 'module' tag.");
      }
      std::string moduleName = nameAttr->get_value();

      // iterate over childs in module
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const xmlpp::Element *childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;

        // parse the element
        parseElement(currentPath/moduleName, childElement);
      }
    }

    // register tag found: create new entry in map
    else if(element->get_name() == "register" || element->get_name() == "entry") {

      // obtain name of register
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError("Missing name attribute of 'register' tag.");
      }
      RegisterPath registerName = currentPath/std::string(nameAttr->get_value());

      // create new RegisterInfo object
      auto info = boost::shared_ptr<RegisterInfo>( new LogicalNameMap::RegisterInfo() );
      info->name = registerName;

      // obtain the type
      std::string type = getValueFromXmlSubnode<std::string>(element, "type");
      if(type == "register") {
        info->targetType = TargetType::REGISTER;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "device");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "register");
      }
      else if(type == "range") {
        info->targetType = TargetType::RANGE;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "device");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "register");
        info->firstIndex = getValueFromXmlSubnode<unsigned int>(element, "index");
        info->length = getValueFromXmlSubnode<unsigned int>(element, "length");
      }
      else if(type == "channel") {
        info->targetType = TargetType::CHANNEL;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "device");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "register");
        info->channel = getValueFromXmlSubnode<unsigned int>(element, "channel");
      }
      else if(type == "int_constant") {
        info->targetType = TargetType::INT_CONSTANT;
        info->value = getValueFromXmlSubnode<int>(element, "value");
      }
      else if(type == "int_variable") {
        info->targetType = TargetType::INT_VARIABLE;
        info->value = getValueFromXmlSubnode<int>(element, "value");
      }
      else {
        parsingError("Wrong target type: "+type);
      }
      
      // add register to catalogue
      _catalogue.addRegister(info);

      // iterate over childs of the register to find plugins
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const xmlpp::Element *childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;
        if(childElement->get_name() != "plugin") continue;  // look only for plugins

        // get name of plugin
        auto pluginNameAttr = childElement->get_attribute("name");
        if(!pluginNameAttr) {
          parsingError("Missing name attribute of 'plugin' tag.");
        }
        std::string pluginName = pluginNameAttr->get_value();
        
        // collect parameters
        std::map<std::string, Value<std::string> > parameters;
        for(const auto& paramchild : childElement->get_children()) {
          // cast into element, ignore if not an element (e.g. comment)
          const xmlpp::Element *paramElement = dynamic_cast<const xmlpp::Element*>(paramchild);
          if(!paramElement) continue;

          // get name of parameter
          std::string parameterName = paramElement->get_name();
          
          // get value of parameter and store in map
          parameters[parameterName] = getValueFromXmlSubnode<std::string>(childElement,parameterName);
          
        }
        
        // create instance of plugin and add to the list in the register info
        boost::shared_ptr<RegisterPlugin> plugin = RegisterPluginFactory::getInstance().createPlugin(pluginName, parameters);
        info->pluginList.push_back(plugin);

      }

    }
    else {
      parsingError("Unknown tag found: "+element->get_name());
    }
  }

  /********************************************************************************************************************/

  const LogicalNameMap::RegisterInfo& LogicalNameMap::getRegisterInfo(const std::string &name) const {
    try {
      // static cast the mtca4u::RegisterInfo into our LogicalNameMap::RegisterInfo is ok, since we control the
      // catalogue and put only actual LogicalNameMap::RegisterInfo in there.
      return *static_cast<LogicalNameMap::RegisterInfo*>(_catalogue.getRegister(currentModule/name).get());
    }
    catch(std::out_of_range &e) {
      throw DeviceException("Register '"+(currentModule/name)+"' was not found in logical name map ("+e.what()+").",
          DeviceException::REGISTER_DOES_NOT_EXIST);
    }
  }

  /********************************************************************************************************************/

  boost::shared_ptr<LogicalNameMap::RegisterInfo> LogicalNameMap::getRegisterInfoShared(const std::string &name) {
    try {
      // static cast the mtca4u::RegisterInfo into our LogicalNameMap::RegisterInfo is ok, since we control the
      // catalogue and put only actual LogicalNameMap::RegisterInfo in there.
      return boost::static_pointer_cast<LogicalNameMap::RegisterInfo>(_catalogue.getRegister(currentModule/name));
    }
    catch(std::out_of_range &e) {
      throw DeviceException("Register '"+(currentModule/name)+"' was not found in logical name map ("+e.what()+").",
          DeviceException::REGISTER_DOES_NOT_EXIST);
    }
  }

  /********************************************************************************************************************/

  std::unordered_set<std::string> LogicalNameMap::getTargetDevices() const {
    std::unordered_set<std::string> ret;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      auto info = boost::static_pointer_cast<const LogicalNameMap::RegisterInfo>(it.get());
      if(info->hasDeviceName()) {
        ret.insert(info->deviceName);
      }
    }
    return ret;
  }

  /********************************************************************************************************************/

  void LogicalNameMap::parsingError(const std::string &message) {
    throw DeviceException("Error parsing the xlmap file '"+_fileName+"': "+message, DeviceException::CANNOT_OPEN_MAP_FILE);
  }

} // namespace mtca4u
