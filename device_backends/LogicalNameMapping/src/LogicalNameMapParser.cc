// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LogicalNameMapParser.h"

#include "DeviceBackend.h"
#include "Exception.h"
#include "LNMBackendRegisterInfo.h"
#include <libxml++/libxml++.h>

#include <stdexcept>

namespace ChimeraTK {

  template<>
  std::string LogicalNameMapParser::getValueFromXmlSubnode<std::string>(const xmlpp::Node* node,
      const std::string& subnodeName, BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue,
      bool hasDefault, std::string defaultValue) {
    auto list = node->find(subnodeName);
    if(list.empty() && hasDefault) return defaultValue;
    if(list.size() != 1) {
      parsingError(node,
          "Expected exactly one subnode of the type '" + subnodeName + "' below node '" + node->get_name() + "'.");
    }
    auto childList = list[0]->get_children();

    std::string value;
    for(auto& child : childList) {
      if(!child) {
        parsingError(child, "Got nullptr from parser library.");
      }

      // Check for CDATA node
      const auto* cdataNode = dynamic_cast<const xmlpp::CdataNode*>(child);
      if(cdataNode) {
        value += cdataNode->get_content();
        continue;
      }

      // check for plain text
      const auto* textNode = dynamic_cast<const xmlpp::TextNode*>(child);
      if(textNode) {
        // put to stream buffer
        value += textNode->get_content();
        continue;
      }

      // check for reference
      if(child->get_name() == "ref") {
        auto childChildList = child->get_children();
        const auto* refNameNode = dynamic_cast<const xmlpp::TextNode*>(childChildList.front());
        if(refNameNode && childChildList.size() == 1) {
          std::string regName = refNameNode->get_content();
          if(!catalogue.hasRegister(regName)) {
            parsingError(child, "Reference to constant '" + regName + "' could not be resolved.");
          }
          auto reg = catalogue.getBackendRegister(regName);
          // auto reg_casted = boost::dynamic_pointer_cast<LNMBackendRegisterInfo>(reg);
          // assert(reg_casted != nullptr); // this is our own catalogue
          //  fetch the value of the target constant
          if(reg.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT) {
            if(!reg.plugins.empty()) {
              parsingError(childList.front(), "'" + regName + "' uses plugins which is not supported for <ref>");
            }
            // put to stream buffer
            auto& lnmVariable = _variables[reg.name];
            callForType(reg.valueType, [&](auto arg) {
              std::stringstream buf;
              buf << boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table).latestValue[0];
              value = buf.str();
            });
            continue;
          }
          parsingError(child, "Reference to '" + regName + "' does not refer to a constant.");
        }
        else {
          parsingError(child, "The <ref> node must contain only text.");
        }
      }

      // check for parameter
      if(child->get_name() == "par") {
        auto childChildList = child->get_children();
        const auto* parNameNode = dynamic_cast<const xmlpp::TextNode*>(childChildList.front());
        if(parNameNode && childChildList.size() == 1) {
          std::string parName = parNameNode->get_content();
          if(_parameters.find(parName) == _parameters.end()) {
            parsingError(child, "Parameter '" + parName + "' could not be resolved.");
          }
          // put to stream buffer
          value += _parameters[parName];
          continue;
        }
        parsingError(child, "The <par> node must contain only text.");
      }

      // neither found: throw error
      parsingError(node,
          "Node '" + subnodeName +
              "' should contain only text, CDATA sections, references or parameters. Instead child '" +
              child->get_name() + "' was found.");
    }
    return value;
  } // namespace ChimeraTK

  /********************************************************************************************************************/

  template<typename T>
  static T stringToValue(const std::string& valAsString) {
    if constexpr(std::is_integral_v<T>) {
      // special case of integers: handle prefix for hex or octal notation, like 0xff, 077
      T value;
      try {
        // base=0 means auto detect base, depending on prefix
        int64_t longVal = std::stoll(valAsString, nullptr, 0);
        if(std::is_unsigned_v<T>) {
          if(longVal < 0) {
            throw std::out_of_range(std::string("negative!"));
          }
        }
        else {
          if(longVal < (int64_t)std::numeric_limits<T>::min()) {
            throw std::out_of_range(std::string("too small!"));
          }
        }
        // we need to exclude upper range check for uint64 since max val does not fit into longVal
        if(!std::is_same_v<T, uint64_t> && longVal > (int64_t)std::numeric_limits<T>::max()) {
          throw std::out_of_range(std::string("too large!"));
        }
        value = longVal;
      }
      catch(std::exception& e) {
        std::string msg =
            "LogicalNameMapParser: string '" + valAsString + "' cannot be converted to integer (" + e.what() + ")";
        throw ChimeraTK::logic_error(msg);
      }
      return value;
    }
    else {
      // generic case: put into stream
      std::stringstream stream;
      stream << valAsString;
      // interpret stream as value of type T and return it
      T value;
      stream >> value;
      return value;
    }
  }

  template<typename T>
  T LogicalNameMapParser::getValueFromXmlSubnode(const xmlpp::Node* node, const std::string& subnodeName,
      BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue, bool hasDefault, T defaultValue) {
    // obtain result as string
    auto valAsString =
        getValueFromXmlSubnode<std::string>(node, subnodeName, catalogue, hasDefault, std::to_string(defaultValue));

    return stringToValue<T>(valAsString);
  }

  /********************************************************************************************************************/

  template<typename T>
  std::vector<T> LogicalNameMapParser::getValueVectorFromXmlSubnode(const xmlpp::Node* node,
      const std::string& subnodeName, BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue) {
    auto list = node->find(subnodeName);
    if(list.empty()) {
      parsingError(node,
          "Expected at least one subnode of the type '" + subnodeName + "' below node '" + node->get_name() + "'.");
    }

    std::vector<T> valueVector;

    for(auto& child : list) {
      const auto* childElement = dynamic_cast<const xmlpp::Element*>(child);
      if(!childElement) continue; // ignore comments etc.

      // obtain index and resize valueVector if necessary
      auto* indexAttr = childElement->get_attribute("index");
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
      const auto* textNode = dynamic_cast<const xmlpp::TextNode*>(childList.front());
      if(textNode) {
        std::string valAsString = textNode->get_content();
        valueVector[index] = stringToValue<T>(valAsString);
        continue;
      }

      // check for reference
      if(childList.front()->get_name() == "ref") {
        auto childChildList = childList.front()->get_children();
        const auto* refNameNode = dynamic_cast<const xmlpp::TextNode*>(childChildList.front());
        if(refNameNode && childChildList.size() == 1) {
          std::string regName = refNameNode->get_content();
          if(!catalogue.hasRegister(regName)) {
            parsingError(childList.front(), "Reference to constant '" + regName + "' could not be resolved.");
          }
          auto reg = catalogue.getBackendRegister(regName);
          // fetch the value of the target constant
          if(reg.targetType == LNMBackendRegisterInfo::TargetType::CONSTANT) {
            if(!reg.plugins.empty()) {
              parsingError(childList.front(), "'" + regName + "' uses plugins which is not supported for <ref>");
            }
            // convert via string
            std::stringstream buf;
            auto& lnmVariable = _variables[reg.name];
            callForType(reg.valueType, [&](auto arg) {
              buf << boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table).latestValue[0];
            });
            std::string strVal;
            buf >> strVal;
            valueVector[index] = stringToValue<T>(strVal);
            continue;
          }
          parsingError(childList.front(), "Reference to '" + regName + "' does not refer to a constant.");
        }
        else {
          parsingError(childList.front(), "The <ref> node must contain only text.");
        }
      }

      if(childList.front()->get_name() == "par") {
        auto childChildList = childList.front()->get_children();
        const auto* parNameNode = dynamic_cast<const xmlpp::TextNode*>(childChildList.front());
        if(parNameNode && childChildList.size() == 1) {
          std::string parName = parNameNode->get_content();
          if(_parameters.find(parName) == _parameters.end()) {
            parsingError(childList.front(), "Parameter '" + parName + "' could not be resolved.");
          }
          valueVector[index] = stringToValue<T>(_parameters[parName]);
          continue;
        }
        parsingError(childList.front(), "The <par> node must contain only text.");
      }

      // neither found: throw error
      parsingError(node,
          "Node '" + subnodeName + "' should contain only text or exactly one reference, instead child '" +
              childList.front()->get_name() + "' was found.");
    }

    return valueVector;
  }

  /********************************************************************************************************************/

  BackendRegisterCatalogue<LNMBackendRegisterInfo> LogicalNameMapParser::parseFile(const std::string& fileName) {
    _fileName = fileName;

    BackendRegisterCatalogue<LNMBackendRegisterInfo> catalogue;

    // parse the file into a DOM structure
    xmlpp::DomParser parser;
    try {
      parser.parse_file(fileName);
    }
    catch(xmlpp::exception& e) {
      throw ChimeraTK::logic_error("Error opening the xlmap file '" + fileName + "': " + e.what());
    }

    // get root element
    auto* const root = parser.get_document()->get_root_node();
    if(root->get_name() != "logicalNameMap") {
      parsingError(root, "Expected 'logicalNameMap' tag instead of: " + root->get_name());
    }

    // parsing loop
    for(const auto& child : root->get_children()) {
      // cast into element, ignore if not an element (e.g. comment)
      const auto* element = dynamic_cast<const xmlpp::Element*>(child);
      if(!element) continue;

      // parse the element
      parseElement(RegisterPath(), element, catalogue);
    }

    return catalogue;
  }

  void LogicalNameMapParser::parseElement(const RegisterPath& currentPath, const xmlpp::Element* element,
      BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue) {
    // module tag found: look for registers and sub-modules in module
    if(element->get_name() == "module") {
      // obtain name of the module
      auto* nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError(element, "Missing name attribute of 'module' tag.");
      }
      std::string moduleName = nameAttr->get_value();

      // iterate over children in module
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const auto* childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;

        // parse the element
        parseElement(currentPath / moduleName, childElement, catalogue);
      }
    }

    // register tag found: create new entry in map
    else {
      // obtain the type
      std::string type = element->get_name();

      // obtain name of logical register
      auto* nameAttr = element->get_attribute("name");
      if(!nameAttr) {
        parsingError(element, "Missing name attribute of '" + type + "' tag.");
      }
      RegisterPath registerName = currentPath / std::string(nameAttr->get_value());

      // create new RegisterInfo object
      LNMBackendRegisterInfo info;
      info.name = registerName;
      if(type == "redirectedRegister") {
        info.targetType = LNMBackendRegisterInfo::TargetType::REGISTER;
        info.deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice", catalogue);
        info.registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister", catalogue);
        info.firstIndex = getValueFromXmlSubnode<unsigned int>(element, "targetStartIndex", catalogue, true, 0);
        info.length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", catalogue, true, 0);
        info.nChannels = 0;
      }
      else if(type == "redirectedChannel") {
        info.targetType = LNMBackendRegisterInfo::TargetType::CHANNEL;
        info.deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice", catalogue);
        info.registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister", catalogue);
        info.channel = getValueFromXmlSubnode<unsigned int>(element, "targetChannel", catalogue);
        info.firstIndex = getValueFromXmlSubnode<unsigned int>(element, "targetStartIndex", catalogue, true, 0);
        info.length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", catalogue, true, 0);
        info.nChannels = 1;
      }
      else if(type == "redirectedBit") {
        info.targetType = LNMBackendRegisterInfo::TargetType::BIT;
        info.deviceName = getValueFromXmlSubnode<std::string>(element, "targetDevice", catalogue);
        info.registerName = getValueFromXmlSubnode<std::string>(element, "targetRegister", catalogue);
        info.bit = getValueFromXmlSubnode<unsigned int>(element, "targetBit", catalogue);
        info.firstIndex = 0;
        info.length = 0;
        info.nChannels = 1;
      }
      else if(type == "constant") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type", catalogue);
        if(constantType == "integer") constantType = "int32";
        info.targetType = LNMBackendRegisterInfo::TargetType::CONSTANT;
        info.valueType = DataType(constantType);
        auto& lnmVariable = _variables[info.name];
        callForType(info.valueType, [&](auto arg) {
          boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table).latestValue =
              this->getValueVectorFromXmlSubnode<decltype(arg)>(element, "value", catalogue);
        });
        info.firstIndex = 0;
        info.length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", catalogue, true, 1);
        info.nChannels = 1;
        info.writeable = false;
        info.readable = true;
        info._dataDescriptor = ChimeraTK::DataDescriptor(info.valueType);
      }
      else if(type == "variable") {
        std::string constantType = getValueFromXmlSubnode<std::string>(element, "type", catalogue);
        if(constantType == "integer") constantType = "int32";
        info.targetType = LNMBackendRegisterInfo::TargetType::VARIABLE;
        info.valueType = DataType(constantType);
        auto& lnmVariable = _variables[info.name];
        callForType(info.valueType, [&](auto arg) {
          boost::fusion::at_key<decltype(arg)>(lnmVariable.valueTable.table).latestValue =
              this->getValueVectorFromXmlSubnode<decltype(arg)>(element, "value", catalogue);
        });
        info.firstIndex = 0;
        info.length = getValueFromXmlSubnode<unsigned int>(element, "numberOfElements", catalogue, true, 1);
        info.nChannels = 1;
        info.writeable = true;
        info.readable = true;
        info._dataDescriptor = ChimeraTK::DataDescriptor(info.valueType);
        info.supportedFlags = {AccessMode::wait_for_new_data};
      }
      else {
        parsingError(element, "Wrong logical register type: " + type);
      }

      // iterate over children of the register to find plugins
      for(const auto& child : element->get_children()) {
        // cast into element, ignore if not an element (e.g. comment)
        const auto* childElement = dynamic_cast<const xmlpp::Element*>(child);
        if(!childElement) continue;
        if(childElement->get_name() != "plugin") continue; // look only for plugins

        // get name of plugin
        auto* pluginNameAttr = childElement->get_attribute("name");
        if(!pluginNameAttr) {
          parsingError(childElement, "Missing name attribute of 'plugin' tag.");
        }
        std::string pluginName = pluginNameAttr->get_value();

        // collect parameters
        std::map<std::string, std::string> parameters;
        for(const auto& paramchild : childElement->get_children()) {
          // cast into element, ignore if not an element (e.g. comment)
          const auto* paramElement = dynamic_cast<const xmlpp::Element*>(paramchild);
          if(!paramElement) continue;
          if(paramElement->get_name() != "parameter") {
            parsingError(paramElement, "Unexpected element '" + paramElement->get_name() + "' inside plugin tag.");
          }

          // get name of parameter
          auto* parameterNameAttr = paramElement->get_attribute("name");
          if(!pluginNameAttr) {
            parsingError(paramElement, "Missing name attribute of 'parameter' tag.");
          }
          std::string parameterName = parameterNameAttr->get_value();

          // get value of parameter and store in map
          parameters[parameterName] =
              getValueFromXmlSubnode<std::string>(childElement, "parameter[@name='" + parameterName + "']", catalogue);
        }

        // create instance of plugin and add to the list in the register info
        info.plugins.push_back(LNMBackend::makePlugin(info, info.plugins.size(), pluginName, parameters));
      }

      // add register to catalogue
      catalogue.addRegister(info);
    }
  }

  /********************************************************************************************************************/

  void LogicalNameMapParser::parsingError(const xmlpp::Node* node, const std::string& message) {
    throw ChimeraTK::logic_error(
        "Error parsing the xlmap file '" + _fileName + "'(" + std::to_string(node->get_line()) + ") : " + message);
  }

} // namespace ChimeraTK
