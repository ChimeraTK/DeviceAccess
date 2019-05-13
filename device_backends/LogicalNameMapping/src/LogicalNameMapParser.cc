/*
 * LogicalNameMap.cc
 *
 *  Created on: Feb 8, 2016
 *      Author: Martin Hierholzer
 */

#include <libxml++/libxml++.h>
#include <stdexcept>

#include "DeviceBackend.h"
#include "Exception.h"
#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMapParser.h"

namespace ChimeraTK {

  template<>
  std::string LogicalNameMapParser::getValueFromXmlSubnode<std::string>(
      const xmlpp::Node* node, const std::string& subnodeName, bool hasDefault, std::string defaultValue) {
    auto list = node->find(subnodeName);
    if(list.size() < 1 && hasDefault) return defaultValue;
    if(list.size() != 1) {
      parsingError(node,
          "Expected exactly one subnode of the type '" + subnodeName + "' below node '" + node->get_name() + "'.");
    }
    auto childList = list[0]->get_children();

    std::string value;
    for(auto& child : childList) {
      // check for plain text
      const xmlpp::TextNode* textNode = dynamic_cast<xmlpp::TextNode*>(child);
      if(textNode) {
        // put to stream buffer
        value += textNode->get_content();
        continue;
      }

      // check for reference
      if(child->get_name() == "ref") {
        auto childChildList = child->get_children();
        const xmlpp::TextNode* refNameNode = dynamic_cast<xmlpp::TextNode*>(childChildList.front());
        if(refNameNode && childChildList.size() == 1) {
          std::string regName = refNameNode->get_content();
          if(!_catalogue.hasRegister(regName)) {
            parsingError(child, "Reference to constant '" + regName + "' could not be resolved.");
          }
          auto reg = _catalogue.getRegister(regName);
          auto reg_casted = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(reg);
          assert(reg_casted != nullptr); // this is our own catalogue
          // fetch the value of the target constant
          if(reg_casted->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT) {
            // put to stream buffer
            callForType(reg_casted->valueType, [&](auto arg) {
              std::stringstream buf;
              buf << boost::fusion::at_key<decltype(arg)>(reg_casted->valueTable.table)[0];
              value = buf.str();
            });
            continue;
          }
          else {
            parsingError(child, "Reference to '" + regName + "' does not refer to a constant.");
          }
        }
        else {
          parsingError(child, "The <ref> node must contain only text.");
        }
      }

      // check for parameter
      if(child->get_name() == "par") {
        auto childChildList = child->get_children();
        const xmlpp::TextNode* parNameNode = dynamic_cast<xmlpp::TextNode*>(childChildList.front());
        if(parNameNode && childChildList.size() == 1) {
          std::string parName = parNameNode->get_content();
          if(_parameters.find(parName) == _parameters.end()) {
            parsingError(child, "Parameter '" + parName + "' could not be resolved.");
          }
          // put to stream buffer
          value += _parameters[parName];
          continue;
        }
        else {
          parsingError(child, "The <par> node must contain only text.");
        }
      }

      // neither found: throw error
      parsingError(node,
          "Node '" + subnodeName + "' should contain only text, references or parameters. Instead child '" +
              child->get_name() + "' was found.");
    }
    return value;
  }

  /********************************************************************************************************************/

  template<typename T>
  T LogicalNameMapParser::getValueFromXmlSubnode(
      const xmlpp::Node* node, const std::string& subnodeName, bool hasDefault, T defaultValue) {
    // obtain result as string an put into stream
    std::stringstream stream;
    stream << getValueFromXmlSubnode<std::string>(node, subnodeName, hasDefault, std::to_string(defaultValue));

    // interpret stream as value of type T and return it
    T value;
    stream >> value;
    return value;
  }

  /********************************************************************************************************************/

  template<typename T>
  std::vector<T> LogicalNameMapParser::getValueVectorFromXmlSubnode(
      const xmlpp::Node* node, const std::string& subnodeName) {
    auto list = node->find(subnodeName);
    if(list.size() < 1) {
      parsingError(node,
          "Expected at least one subnode of the type '" + subnodeName + "' below node '" + node->get_name() + "'.");
    }

    std::vector<T> valueVector;

    for(auto& child : list) {
      const xmlpp::Element* childElement = dynamic_cast<const xmlpp::Element*>(child);
      if(!childElement) continue; // ignore comments etc.

      // obtain index and resize valueVector if necessary
      auto indexAttr = childElement->get_attribute("index");
      size_t index = 0;
      if(indexAttr) {
        index = std::stoi(indexAttr->get_value());
      }
      if(index >= valueVector.size()) valueVector.resize(index + 1);

      // get content
      auto childList = childElement->get_children();
      if(childList.size() != 1) {
        parsingError(childElement,
            "Node '" + subnodeName +
                "' should contain only text or exactly one reference, "
                "instead multiple childs "
                "were found.");
      }

      // check for plain text
      const xmlpp::TextNode* textNode = dynamic_cast<xmlpp::TextNode*>(childList.front());
      if(textNode) {
        std::stringstream buf(textNode->get_content());
        buf >> valueVector[index];
        continue;
      }

      // check for reference
      if(childList.front()->get_name() == "ref") {
        auto childChildList = childList.front()->get_children();
        const xmlpp::TextNode* refNameNode = dynamic_cast<xmlpp::TextNode*>(childChildList.front());
        if(refNameNode && childChildList.size() == 1) {
          std::string regName = refNameNode->get_content();
          if(!_catalogue.hasRegister(regName)) {
            parsingError(childList.front(), "Reference to constant '" + regName + "' could not be resolved.");
          }
          auto reg = _catalogue.getRegister(regName);
          auto reg_casted = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(reg);
          assert(reg_casted != nullptr); // this is our own catalogue
          // fetch the value of the target constant
          if(reg_casted->targetType == LNMBackendRegisterInfo::TargetType::CONSTANT) {
            // convert via string
            std::stringstream buf;
            callForType(reg_casted->valueType,
                [&](auto arg) { buf << boost::fusion::at_key<decltype(arg)>(reg_casted->valueTable.table)[0]; });
            buf >> valueVector[index];
            continue;
          }
          else {
            parsingError(childList.front(), "Reference to '" + regName + "' does not refer to a constant.");
          }
        }
        else {
          parsingError(childList.front(), "The <ref> node must contain only text.");
        }
      }

      if(childList.front()->get_name() == "par") {
        auto childChildList = childList.front()->get_children();
        const xmlpp::TextNode* parNameNode = dynamic_cast<xmlpp::TextNode*>(childChildList.front());
        if(parNameNode && childChildList.size() == 1) {
          std::string parName = parNameNode->get_content();
          if(_parameters.find(parName) == _parameters.end()) {
            parsingError(childList.front(), "Parameter '" + parName + "' could not be resolved.");
          }
          // put to stream buffer
          std::stringstream buf;
          buf << _parameters[parName];
          buf >> valueVector[index];
          continue;
        }
        else {
          parsingError(childList.front(), "The <par> node must contain only text.");
        }
      }

      // neither found: throw error
      parsingError(node,
          "Node '" + subnodeName + "' should contain only text or exactly one reference, instead child '" +
              childList.front()->get_name() + "' was found.");
    }

    return valueVector;
  }

  /********************************************************************************************************************/

  void LogicalNameMapParser::parseFile(const std::string& fileName) {
    _fileName = fileName;

    // parse the file into a DOM structure
    xmlpp::DomParser parser;
    try {
      parser.parse_file(fileName);
    }
    catch(xmlpp::exception& e) {
      throw ChimeraTK::logic_error("Error opening the xlmap file '" + fileName + "': " + e.what());
    }

    // get root element
    const auto root = parser.get_document()->get_root_node();
    if(root->get_name() != "logicalNameMap") {
      parsingError(root, "Expected 'logicalNameMap' tag instead of: " + root->get_name());
    }

    // parsing loop
    for(const auto& child : root->get_children()) {
      // cast into element, ignore if not an element (e.g. comment)
      const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) continue;

      // parse the element
      parseElement(RegisterPath(), element);
    }
  }

  void LogicalNameMapParser::parseElement(RegisterPath currentPath, const xmlpp::Element* element) {
    // module tag found: look for registers and sub-modules in module
    if(element->get_name() == "module") {
      // obtain name of the module
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError(element, "Missing name attribute of 'module' tag.");
      }
      std::string moduleName = nameAttr->get_value();

      // iterate over childs in module
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const xmlpp::Element* childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;

        // parse the element
        parseElement(currentPath / moduleName, childElement);
      }
    }

    // register tag found: create new entry in map
    else {
      // obtain the type
      std::string type = element->get_name();

      // create new RegisterInfo object
      auto info = boost::shared_ptr<LNMBackendRegisterInfo>(new LNMBackendRegisterInfo());

      if(type == "redirectedRegister") {
        info->targetType = LNMBackendRegisterInfo::TargetType::REGISTER;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister");
        info->firstIndex = getValueFromXmlSubnode<unsigned int>(element, "targetStartIndex", true, 0);
        info->length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", true, 0);
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
      else if(type == "redirectedBit") {
        info->targetType = LNMBackendRegisterInfo::TargetType::BIT;
        info->deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice");
        info->registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister");
        info->bit = getValueFromXmlSubnode<unsigned int>(element, "targetBit");
        info->firstIndex = 0;
        info->length = 0;
        info->nDimensions = 1;
        info->nChannels = 1;
      }
      else if(type == "constant") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type");
        if(constantType == "integer") constantType = "int32";
        info->targetType = LNMBackendRegisterInfo::TargetType::CONSTANT;
        info->valueType = DataType(constantType);
        callForType(info->valueType, [&](auto arg) {
          boost::fusion::at_key<decltype(arg)>(info->valueTable.table) =
              this->getValueVectorFromXmlSubnode<decltype(arg)>(element, "value");
        });
        info->firstIndex = 0;
        info->length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", true, 1);
        info->nDimensions = info->length > 1 ? 1 : 0;
        info->nChannels = 1;
        info->writeable = false;
        info->readable = true;
        info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(info->valueType);
      }
      else if(type == "variable") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type");
        if(constantType == "integer") constantType = "int32";
        info->targetType = LNMBackendRegisterInfo::TargetType::VARIABLE;
        info->valueType = DataType(constantType);
        callForType(info->valueType, [&](auto arg) {
          boost::fusion::at_key<decltype(arg)>(info->valueTable.table) =
              this->getValueVectorFromXmlSubnode<decltype(arg)>(element, "value");
        });
        info->firstIndex = 0;
        info->length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", true, 1);
        info->nDimensions = info->length > 1 ? 1 : 0;
        info->nChannels = 1;
        info->writeable = true;
        info->readable = true;
        info->_dataDescriptor = ChimeraTK::RegisterInfo::DataDescriptor(info->valueType);
      }
      else {
        parsingError(element, "Wrong logical register type: " + type);
      }

      // obtain name of logical register
      auto nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError(element, "Missing name attribute of '" + type + "' tag.");
      }
      RegisterPath registerName = currentPath / std::string(nameAttr->get_value());
      info->name = registerName;

      // iterate over childs of the register to find plugins
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const xmlpp::Element* childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;
        if(childElement->get_name() != "plugin") continue; // look only for plugins

        // get name of plugin
        auto pluginNameAttr = childElement->get_attribute("name");
        if(!pluginNameAttr) {
          parsingError(childElement, "Missing name attribute of 'plugin' tag.");
        }
        std::string pluginName = pluginNameAttr->get_value();

        // collect parameters
        std::map<std::string, std::string> parameters;
        for(const auto& paramchild : childElement->get_children()) {
          // cast into element, ignore if not an element (e.g. comment)
          const xmlpp::Element* paramElement = dynamic_cast<const xmlpp::Element*>(paramchild);
          if(!paramElement) continue;
          if(paramElement->get_name() != "parameter") {
            parsingError(paramElement, "Unexpected element '" + paramElement->get_name() + "' inside plugin tag.");
          }

          // get name of parameter
          auto parameterNameAttr = paramElement->get_attribute("name");
          if(!pluginNameAttr) {
            parsingError(paramElement, "Missing name attribute of 'parameter' tag.");
          }
          std::string parameterName = parameterNameAttr->get_value();

          // get value of parameter and store in map
          parameters[parameterName] =
              getValueFromXmlSubnode<std::string>(childElement, "parameter[@name='" + parameterName + "']");
        }

        // create instance of plugin and add to the list in the register info
        info->plugins.push_back(LNMBackend::makePlugin(info, pluginName, parameters));
      }

      // add register to catalogue
      _catalogue.addRegister(info);
    }
  }

  /********************************************************************************************************************/

  std::unordered_set<std::string> LogicalNameMapParser::getTargetDevices() const {
    std::unordered_set<std::string> ret;
    for(auto it = _catalogue.begin(); it != _catalogue.end(); ++it) {
      auto info = boost::static_pointer_cast<const LNMBackendRegisterInfo>(it.get());
      std::string dev = info->deviceName;
      if(dev != "this" && dev != "") ret.insert(dev);
    }
    return ret;
  }

  /********************************************************************************************************************/

  void LogicalNameMapParser::parsingError(const xmlpp::Node* node, const std::string& message) {
    throw ChimeraTK::logic_error(
        "Error parsing the xlmap file '" + _fileName + "'(" + std::to_string(node->get_line()) + ") : " + message);
  }

} // namespace ChimeraTK
