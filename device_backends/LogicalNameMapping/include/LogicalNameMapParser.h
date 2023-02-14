// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterCatalogue.h"
#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "LNMVariable.h"
#include "RegisterPath.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <utility>

// forward declaration
namespace xmlpp {
  class Node;
  class Element;
} // namespace xmlpp

namespace ChimeraTK {

  /** Logical name map: store information from xlmap file and provide it to the
   * LogicalNameMappingBackend and its register accessors. */
  class LogicalNameMapParser {
   public:
    /** Constructor: parse map from XML file */
    LogicalNameMapParser(std::map<std::string, std::string> parameters, std::map<std::string, LNMVariable>& variables)
    : _parameters(std::move(parameters)), _variables(variables) {}

    /** parse the given XML file */
    BackendRegisterCatalogue<LNMBackendRegisterInfo> parseFile(const std::string& fileName);
    // BackendRegisterCatalogue<LNMBackendRegisterInfo> _catalogue;

   protected:
    /** called inside parseFile() to parse an XML element and its sub-elements
     * recursively */
    void parseElement(RegisterPath currentPath, const xmlpp::Element* element,
        BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue);

    /** throw a parsing error with more information */
    [[noreturn]] void parsingError(const xmlpp::Node* node, const std::string& message);

    /** Build a Value object for a given subnode. */
    template<typename ValueType>
    ValueType getValueFromXmlSubnode(const xmlpp::Node* node, const std::string& subnodeName,
        BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue, bool hasDefault = false,
        ValueType defaultValue = ValueType());
    template<typename ValueType>
    std::vector<ValueType> getValueVectorFromXmlSubnode(const xmlpp::Node* node, const std::string& subnodeName,
        BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue);

    /** file name of the logical map */
    std::string _fileName;

    /** current register path in the map */
    RegisterPath currentModule;

    /** actual register info map (register name to target information) */
    // BackendRegisterCatalogue<LNMBackendRegisterInfo> _catalogue;
    // LNMRegisterCatalogue _catalogue;

    /** parameter map */
    std::map<std::string, std::string> _parameters;

    /** Reference to the variables map inside the LNM backend. Is filled with initial values in the parser **/
    std::map<std::string, LNMVariable>& _variables;
  };

  template<>
  std::string LogicalNameMapParser::getValueFromXmlSubnode<std::string>(const xmlpp::Node* node,
      const std::string& subnodeName, BackendRegisterCatalogue<LNMBackendRegisterInfo> const& catalogue,
      bool hasDefault, std::string defaultValue);

} // namespace ChimeraTK
