// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AccessMode.h"
#include "BackendFactory.h"
#include "BackendRegisterInfoBase.h"
#include "DataDescriptor.h"
#include "Device.h"
#include "RegisterCatalogue.h"

#include <nlohmann/json.hpp>

#include <boost/program_options.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = boost::program_options;
using nlohmann::json;

// Return the number of bytes per single element of the given data descriptor, using the configured fixed string length.
size_t byteSizePerElement(const ChimeraTK::DataDescriptor& descriptor, size_t stringLength);

// Build the jmap "representation" object from the generic data descriptor.
json makeRepresentation(const ChimeraTK::DataDescriptor& descriptor);

// Return the jmap "access" string for a register from its read/write capability.
std::string accessString(const ChimeraTK::BackendRegisterInfoBase& info);

// Insert a register entry into the addressSpace hierarchy, mirroring the module path via "children" objects.
void insertRegister(json& children, const std::vector<std::string>& components, const json& entry);

// Round a byte size up to a multiple of 4 (the dummy minimum transfer alignment).
size_t padToAlignment(size_t size);

/**********************************************************************************************************************/

int main(int argc, char* argv[]) {
  po::options_description options("Usage: chimeratk-device-to-jmap [options]");
  options.add_options()("help", "print this help text")("device", po::value<std::string>()->required(),
      "device to read the register catalogue from, given as a ChimeraTK device descriptor (CDD) or an alias name")(
      "dmap", po::value<std::string>(), "dmap file path used to resolve alias names; does not affect --device")(
      "output", po::value<std::string>()->required(), "path of the jmap file to write (never written to stdout)")(
      "string-length", po::value<size_t>(),
      "fixed byte size per element assumed for ASCII string registers (default 80)");

  po::variables_map variableMap;
  try {
    po::store(po::parse_command_line(argc, argv, options), variableMap);
  }
  catch(const po::error& error) {
    std::cerr << "Error: " << error.what() << "\n\n";
    std::cerr << options << "\n";
    return 1;
  }

  if(variableMap.count("help")) {
    std::cout << options << "\n";
    std::cout << "Limitations:\n";
    std::cout << "  - Two-dimensional registers are skipped (a single warning lists all affected registers).\n";
    std::cout << "  - Writable registers with AccessMode::wait_for_new_data become read-only interrupts (a single "
                 "warning lists all affected registers).\n";
    std::cout << "  - Addresses in the output are synthetic.\n";
    std::cout << "  - ASCII strings are written with a fixed length of --string-length bytes per element "
                 "(default 80).\n";
    return 0;
  }

  try {
    po::notify(variableMap);
  }
  catch(const po::error& error) {
    std::cerr << "Error: " << error.what() << "\n\n";
    std::cerr << options << "\n";
    return 1;
  }

  if(variableMap.count("dmap")) {
    ChimeraTK::BackendFactory::getInstance().setDMapFilePath(variableMap["dmap"].as<std::string>());
  }

  std::string deviceString = variableMap["device"].as<std::string>();
  std::string outputPath = variableMap["output"].as<std::string>();
  size_t stringLength = 80;
  if(variableMap.count("string-length")) {
    stringLength = variableMap["string-length"].as<size_t>();
  }

  ChimeraTK::Device device;
  // Opening the device can throw a ChimeraTK::logic_error (e.g. for an unknown backend or an invalid descriptor).
  // Report it instead of terminating via an unhandled exception in main.
  try {
    device.open(deviceString);
  }
  catch(const ChimeraTK::logic_error& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
  auto catalogue = device.getRegisterCatalogue();

  // Collect the registers and sort them alphabetically by full path for deterministic output.
  std::vector<const ChimeraTK::BackendRegisterInfoBase*> registers;
  registers.reserve(catalogue.getNumberOfRegisters());
  for(const auto& registerInfo : catalogue) {
    registers.push_back(&registerInfo);
  }
  std::ranges::sort(
      registers, [](const ChimeraTK::BackendRegisterInfoBase* left, const ChimeraTK::BackendRegisterInfoBase* right) {
        return left->getRegisterName() < right->getRegisterName();
      });

  json addressSpace = json::object();
  std::vector<std::string> twoDimensionalRegisters;
  std::vector<std::string> writableInterruptRegisters;
  size_t interruptId = 0;
  size_t addressOffset = 0;

  try {
    for(const auto* registerInfo : registers) {
      auto name = registerInfo->getRegisterName();
      if(registerInfo->getNumberOfDimensions() > 1) {
        twoDimensionalRegisters.push_back(name);
        continue;
      }

      auto descriptor = registerInfo->getDataDescriptor();
      bool isVoid = (descriptor.fundamentalType() == ChimeraTK::DataDescriptor::FundamentalType::nodata);
      bool waitForNewData = registerInfo->getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data);

      if(waitForNewData && registerInfo->isWriteable() && !isVoid) {
        writableInterruptRegisters.push_back(name);
      }

      json entry = json::object();
      if(isVoid) {
        // Pure interrupt source: void representation, no address, no access field.
        entry["triggeredByInterrupt"] = json::array({interruptId++});
        entry["representation"] = makeRepresentation(descriptor);
      }
      else {
        auto elementBytes = byteSizePerElement(descriptor, stringLength);
        if(waitForNewData) {
          entry["triggeredByInterrupt"] = json::array({interruptId++});
        }
        else {
          entry["access"] = accessString(*registerInfo);
        }
        entry["representation"] = makeRepresentation(descriptor);
        entry["numberOfElements"] = registerInfo->getNumberOfElements();
        entry["bytesPerElement"] = elementBytes;
        entry["address"] = json::object({{"type", "IO"}, {"channel", 0}, {"offset", addressOffset}});
        addressOffset += padToAlignment(elementBytes * registerInfo->getNumberOfElements());
      }

      insertRegister(addressSpace, name.getComponents(), entry);
    }
  }
  // A std::logic_error can only be thrown by insertRegister on an internal inconsistency (e.g. a duplicate register
  // path or a conflicting key). Report it instead of terminating via an unhandled exception in main.
  catch(const std::logic_error& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }

  json output;
  output["mapFormatVersion"] = "0.0.1";
  output["metadata"] = json::object();
  output["interruptHandler"] = json::object();
  output["addressSpace"] = addressSpace;

  if(!twoDimensionalRegisters.empty()) {
    std::cerr << "Warning: the following two-dimensional registers are unsupported and were skipped:\n";
    for(const auto& reg : twoDimensionalRegisters) {
      std::cerr << "  " << reg << "\n";
    }
  }
  if(!writableInterruptRegisters.empty()) {
    std::cerr << "Warning: the following writable registers with AccessMode::wait_for_new_data were written as "
                 "read-only interrupts:\n";
    for(const auto& reg : writableInterruptRegisters) {
      std::cerr << "  " << reg << "\n";
    }
  }

  std::ofstream outputFile(outputPath);
  if(!outputFile.is_open()) {
    std::cerr << "Error: could not open output file '" << outputPath << "' for writing.\n";
    return 1;
  }
  // Pretty-print with two-space indentation for human readability. Keys are already sorted because nlohmann::json
  // stores objects in a std::map, and floats are serialised with round-trip precision by default, so output is
  // deterministic.
  outputFile << output.dump(2) << "\n";
  return 0;
}

/**********************************************************************************************************************/

/**
 *  Return the number of bytes per single element of the given data descriptor.
 *
 *  The generic Device interface does not expose the byte size of ASCII strings, so the configured fixed
 *  --string-length is used for them, only relevant for the synthetic address assignment.
 */
size_t byteSizePerElement(const ChimeraTK::DataDescriptor& descriptor, size_t stringLength) {
  switch(descriptor.fundamentalType()) {
    case ChimeraTK::DataDescriptor::FundamentalType::numeric:
      return descriptor.minimumDataType().getNumberOfBytes();
    case ChimeraTK::DataDescriptor::FundamentalType::boolean:
      // Boolean registers are written as 4-byte fixed-point values with a width of 1 (see makeRepresentation), i.e.
      // the element size is fixed at 4 bytes rather than derived from sizeof(ChimeraTK::Boolean).
      return 4;
    case ChimeraTK::DataDescriptor::FundamentalType::string:
      return stringLength;
    case ChimeraTK::DataDescriptor::FundamentalType::nodata:
    case ChimeraTK::DataDescriptor::FundamentalType::undefined:
      return 0;
  }
  return 0;
}

/**********************************************************************************************************************/

/** Build the jmap "representation" object from the generic data descriptor. */
json makeRepresentation(const ChimeraTK::DataDescriptor& descriptor) {
  json representation = json::object();
  switch(descriptor.fundamentalType()) {
    case ChimeraTK::DataDescriptor::FundamentalType::string:
      representation["type"] = "string";
      break;
    case ChimeraTK::DataDescriptor::FundamentalType::boolean:
      // Boolean registers are written as 4-byte fixed-point values with a width of 1.
      representation["type"] = "fixedPoint";
      representation["width"] = 1;
      representation["fractionalBits"] = 0;
      representation["isSigned"] = false;
      break;
    case ChimeraTK::DataDescriptor::FundamentalType::numeric: {
      auto minimumType = descriptor.minimumDataType();
      if(minimumType.isIntegral()) {
        representation["type"] = "fixedPoint";
        representation["width"] = static_cast<unsigned>(minimumType.getNumberOfBytes()) * 8;
        representation["fractionalBits"] = 0;
        representation["isSigned"] = descriptor.isSigned();
      }
      else {
        representation["type"] = "IEEE754";
        representation["width"] = static_cast<unsigned>(minimumType.getNumberOfBytes()) * 8;
        representation["isSigned"] = true;
      }
      break;
    }
    case ChimeraTK::DataDescriptor::FundamentalType::nodata:
      representation["type"] = "void";
      break;
    case ChimeraTK::DataDescriptor::FundamentalType::undefined:
      representation["type"] = "fixedPoint";
      representation["width"] = 32;
      representation["fractionalBits"] = 0;
      representation["isSigned"] = descriptor.isSigned();
      break;
  }
  return representation;
}

/**********************************************************************************************************************/

/** Return the jmap "access" string for a register from its read/write capability. */
std::string accessString(const ChimeraTK::BackendRegisterInfoBase& info) {
  if(info.isReadable() && info.isWriteable()) {
    return "RW";
  }
  if(info.isWriteable()) {
    return "WO";
  }
  return "RO";
}

/**********************************************************************************************************************/

/**
 *  Insert a register entry into the addressSpace hierarchy, mirroring the module path via "children" objects.
 *
 *  A path component may be a register (with its own address and data descriptor) and, at the same time, a module
 *  carrying further registers below it (cf. the "BSP" and "DAQ" entries in the simpleJsonFile.jmap fixture). In that
 *  case the register data and the "children" object coexist in a single entry, so leaf and module nodes are handled
 *  uniformly: the destination object is created if absent and extended in place (never rebuilt or copied), and at the
 *  leaf the register fields are merged in asserting that no existing key is overwritten.
 */
void insertRegister(json& children, const std::vector<std::string>& components, const json& entry) {
  auto& destination = children[components[0]];
  if(components.size() > 1) {
    // Module node: promote the destination to an object and ensure a "children" map for the remaining components. A
    // pre-existing object (e.g. a register that also carries sub-registers) is extended in place by adding the
    // "children" key; its old contents are never copied into a freshly built object.
    if(!destination.is_object()) {
      destination = json::object();
    }
    if(!destination.contains("children")) {
      destination["children"] = json::object();
    }
    std::vector<std::string> rest(components.begin() + 1, components.end());
    insertRegister(destination["children"], rest, entry);
    return;
  }
  // Leaf node: merge the register fields. An overwrite of an existing key would indicate an internal inconsistency
  // (e.g. a duplicate register path), so an existing object is never silently replaced.
  for(const auto& [key, value] : entry.items()) {
    if(destination.contains(key)) {
      throw std::logic_error("Internal inconsistency: register path '" + entry.dump() +
          "' conflicts with existing "
          "key '" +
          key + "'.");
    }
    destination[key] = value;
  }
}

/**********************************************************************************************************************/

/** Round a byte size up to a multiple of 4 (the dummy minimum transfer alignment). */
size_t padToAlignment(size_t size) {
  return ((size + 3) / 4) * 4;
}

/**********************************************************************************************************************/
