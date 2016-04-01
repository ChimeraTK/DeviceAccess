/*
 * LogicalNameMap.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <stdexcept>
#include <libxml++/libxml++.h>

#include "LogicalNameMapParser.h"
#include "LNMBackendRegisterInfo.h"
#include "DeviceException.h"
#include "DeviceBackend.h"
#include "RegisterPluginFactory.h"

namespace mtca4u {

  template<>
  DynamicValue<std::string> LogicalNameMapParser::getValueFromXmlSubnode(const xmlpp::Node *node,
      const std::string &subnodeName, bool hasDefault, std::string defaultValue) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      if(hasDefault) return defaultValue;
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

    DynamicValue<std::string> value;
    value = textNode->get_content();
    return value;
  }

  /********************************************************************************************************************/

  template<>
  DynamicValue<int> LogicalNameMapParser::getValueFromXmlSubnode(const xmlpp::Node *node,
      const std::string &subnodeName, bool hasDefault, int defaultValue) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      if(hasDefault) return defaultValue;
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text or only one <ref></ref> sub-element.");
    }

    DynamicValue<int> value;

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
  DynamicValue<unsigned int> LogicalNameMapParser::getValueFromXmlSubnode(const xmlpp::Node *node,
      const std::string &subnodeName, bool hasDefault, unsigned int defaultValue) {
    auto list = node->find(subnodeName);
    if(list.size() != 1) {
      if(hasDefault) return defaultValue;
      parsingError("Expected exactly one subnode of the type '"+subnodeName+"' below node '"+node->get_name()+"'.");
    }
    auto childList = list[0]->get_children();
    if(childList.size() != 1) {
      parsingError("Node '"+subnodeName+"' should contain only text.");
    }

    DynamicValue<unsigned int> value;

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

  void LogicalNameMapParser::parseFile(const std::string &fileName) {
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

  void LogicalNameMapParser::parseElement(RegisterPath currentPath, const xmlpp::Element *element) {
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
    else {

      // obtain the type
      std::string type = element->get_name();

      // create new RegisterInfo object
      auto info = boost::shared_ptr<LNMBackendRegisterInfo>( new LNMBackendRegisterInfo() );

      if(type == "redirectedRegister") {
        info->targetType = LNMBackendRegisterInfo::TargetType::REGISTER;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister");
        info->firstIndex = getValueFromXmlSubnode<unsigned int>(element, "targetStartIndex",true,0);
        info->length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements",true,0);
        info->nDimensions = 0;
        info->nChannels = 0;
      }
      else if(type == "redirectedChannel") {
        info->targetType = LNMBackendRegisterInfo::TargetType::CHANNEL;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister");
        info->channel = getValueFromXmlSubnode<unsigned int>(element, "targetChannel");
        info->firstIndex = 0;
        info->length = 0;
        info->nDimensions = 1;
        info->nChannels = 1;
      }
      else if(type == "constant") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type");
        if(constantType != "integer") {
          parsingError("Type '"+constantType+"' is not valid for a constant");
        }
        info->targetType = LNMBackendRegisterInfo::TargetType::INT_CONSTANT;
        info->value = getValueFromXmlSubnode<int>(element, "value");
        info->firstIndex = 0;
        info->length = 1;
        info->nDimensions = 0;
        info->nChannels = 1;
      }
      else if(type == "variable") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type");
        if(constantType != "integer") {
          parsingError("Type '"+constantType+"' is not valid for a variable");
        }
        info->targetType = LNMBackendRegisterInfo::TargetType::INT_VARIABLE;
        info->value = getValueFromXmlSubnode<int>(element, "value");
        info->firstIndex = 0;
        info->length = 1;
        info->nDimensions = 0;
        info->nChannels = 1;
      }
      else {
        parsingError("Wrong logical register type: "+type);
      }

      // obtain name of logical register
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError("Missing name attribute of '"+type+"' tag.");
      }
      RegisterPath registerName = currentPath/std::string(nameAttr->get_value());
      info->name = registerName;

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
        std::map<std::string, DynamicValue<std::string> > parameters;
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
  }

  /********************************************************************************************************************/

  std::unordered_set<std::string> LogicalNameMapParser::getTargetDevices() const {
    std::unordered_set<std::string> ret;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      auto info = boost::static_pointer_cast<const LNMBackendRegisterInfo>(it.get());
      if(info->hasDeviceName()) {
        ret.insert(info->deviceName);
      }
    }
    return ret;
  }

  /********************************************************************************************************************/

  void LogicalNameMapParser::parsingError(const std::string &message) {
    throw DeviceException("Error parsing the xlmap file '"+_fileName+"': "+message, DeviceException::CANNOT_OPEN_MAP_FILE);
  }

} // namespace mtca4u
