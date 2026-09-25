// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK

#define BOOST_TEST_MODULE testDeviceToJmap

#include "AccessMode.h"
#include "BackendFactory.h"
#include "DataDescriptor.h"
#include "Device.h"
#include "RegisterCatalogue.h"
#include "RegisterInfo.h"

#include <nlohmann/json.hpp>
#include <sys/wait.h>

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <format>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

// Path to the chimeratk-device-to-jmap tool, injected by CMake (top-level CMakeLists.txt).
#ifndef CHIMERATK_DEVICE_TO_JMAP
#  define CHIMERATK_DEVICE_TO_JMAP "chimeratk-device-to-jmap"
#endif

BOOST_AUTO_TEST_SUITE(testDeviceToJmap)

/**********************************************************************************************************************/

// Fixed paths used throughout this file (relative to the test working directory, which is the tests build directory).
static const std::string dummySourceCdd = "(dummy?map=simpleJsonFile.jmap)";
static const std::string lmapDmap = "logicalnamemap.dmap";

/**********************************************************************************************************************/
/* Helpers                                                                                                            */
/**********************************************************************************************************************/

/**
 *  Run the tool as a subprocess and return the exit status, stdout and stderr.
 *
 *  extraArgs are appended to the tool command line; stdout and stderr are redirected to the given files inside the
 *  working directory. Returns false if the process did not exit normally.
 */
static bool runTool(
    const std::string& extraArgs, const std::string& stdoutFile, const std::string& stderrFile, int& exitCode) {
  std::string command =
      std::format(R"("{}" {} >"{}" 2>"{}")", CHIMERATK_DEVICE_TO_JMAP, extraArgs, stdoutFile, stderrFile);
  int status = std::system(command.c_str());
  if(!WIFEXITED(status)) {
    return false;
  }
  exitCode = WEXITSTATUS(status);
  return true;
}

/**********************************************************************************************************************/

/** Read a file into a std::string (used to check stdout / stderr / output files). */
static std::string readFile(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

/**********************************************************************************************************************/

/**
 *  Compare two registers of two catalogues.
 *
 *  Holds both comparison rules: access rights modulo the documented lossy transformation of writable
 *  wait_for_new_data registers into read-only interrupts, and signedness / integrality / element size only for
 *  numeric registers (other fundamental types may legitimately infer a different minimum data type).
 */
static bool compareRegisters(const RegisterInfo& source, const RegisterInfo& destination) {
  if(source.getNumberOfElements() != destination.getNumberOfElements()) {
    return false;
  }
  const DataDescriptor& sourceDescriptor = source.getDataDescriptor();
  const DataDescriptor& destinationDescriptor = destination.getDataDescriptor();
  if(sourceDescriptor.fundamentalType() != destinationDescriptor.fundamentalType()) {
    return false;
  }
  // The read capability must match.
  if(source.isReadable() != destination.isReadable()) {
    return false;
  }
  // The only accepted difference in write capability is a writable source register becoming read-only (the documented
  // lossy transformation of writable wait_for_new_data registers into read-only interrupts).
  if(source.isWriteable() != destination.isWriteable() && (!source.isWriteable() || destination.isWriteable())) {
    return false;
  }
  // Signedness, integrality and element size come from DataDescriptor::minimumDataType() and are only defined for
  // numeric registers.
  if(sourceDescriptor.fundamentalType() == DataDescriptor::FundamentalType::numeric) {
    return sourceDescriptor.isSigned() == destinationDescriptor.isSigned() &&
        sourceDescriptor.isIntegral() == destinationDescriptor.isIntegral() &&
        sourceDescriptor.minimumDataType().getNumberOfBytes() ==
        destinationDescriptor.minimumDataType().getNumberOfBytes();
  }
  return true;
}

/**********************************************************************************************************************/

/**
 *  Compare the one-dimensional registers of two catalogues.
 *
 *  Registers are paired by path via hasRegister()/getRegister(). Returns the number of mismatches. Two-dimensional
 *  registers are lossy-skipped by the tool by design, so they are not compared.
 */
static size_t compareCatalogues(const RegisterCatalogue& source, const RegisterCatalogue& destination) {
  size_t mismatches = 0;
  for(const auto& registerInfo : source) {
    if(registerInfo.getNumberOfDimensions() > 1) {
      continue;
    }
    const std::string path = registerInfo.getRegisterName();
    if(!destination.hasRegister(path)) {
      ++mismatches;
      continue;
    }
    if(!compareRegisters(source.getRegister(path), destination.getRegister(path))) {
      ++mismatches;
    }
  }
  // Every one-dimensional destination register must also be present in the source.
  for(const auto& registerInfo : destination) {
    if(registerInfo.getNumberOfDimensions() > 1) {
      continue;
    }
    const std::string path = registerInfo.getRegisterName();
    if(!source.hasRegister(path)) {
      ++mismatches;
    }
  }
  return mismatches;
}

/**********************************************************************************************************************/

/** Open a device, write its catalogue to a jmap file with the tool, re-open the file on a DummyBackend, and compare. */
static size_t roundTrip(const std::string& sourceCdd, const std::string& toolArguments, const std::string& outputFile,
    const std::string& stdoutFile, const std::string& stderrFile) {
  Device source;
  source.open(sourceCdd);
  auto sourceCatalogue = source.getRegisterCatalogue();

  int exitCode = -1;
  BOOST_REQUIRE(runTool(toolArguments, stdoutFile, stderrFile, exitCode));
  BOOST_REQUIRE(exitCode == 0);

  Device destination;
  destination.open("(dummy?map=" + outputFile + ")");
  auto destinationCatalogue = destination.getRegisterCatalogue();

  return compareCatalogues(sourceCatalogue, destinationCatalogue);
}

/**********************************************************************************************************************/

/** Generate a jmap file from the DummyBackend on simpleJsonFile.jmap. Returns the output file path. */
static std::string generateFromDummy(const std::string& outputFile) {
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}" --output {})", dummySourceCdd, outputFile), outputFile + ".stdout",
      outputFile + ".stderr", exitCode));
  BOOST_REQUIRE(exitCode == 0);
  return outputFile;
}

/**********************************************************************************************************************/

/** Generate a jmap file from the LogicalNameMapping backend on valid.xlmap. Returns the output file path. */
static std::string generateFromLMap() {
  std::string outputFile = "devToJmap_lmap.jmap";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device LMAP0 --dmap {} --output {})", lmapDmap, outputFile),
      outputFile + ".stdout", outputFile + ".stderr", exitCode));
  BOOST_REQUIRE(exitCode == 0);
  return outputFile;
}

/**********************************************************************************************************************/

/** Open a generated jmap file on a DummyBackend and return its (one- and two-dimensional) register catalogue. */
static RegisterCatalogue openGenerated(const std::string& jmapFile) {
  Device device;
  device.open("(dummy?map=" + jmapFile + ")");
  return device.getRegisterCatalogue();
}

/**********************************************************************************************************************/

/** Clean up the auxiliary files produced by a test case. */
static void cleanup(const std::vector<std::string>& files) {
  for(const auto& file : files) {
    std::remove(file.c_str());
  }
}

/**********************************************************************************************************************/

/**
 *  Extract the register paths listed under a single warning block in a stderr text.
 *
 *  The tool emits each warning as exactly one "Warning: ..." header line followed by one indented register path per
 *  line. Returns false if the header identified by the given keyword does not appear exactly once.
 */
static bool extractWarningRegisters(
    const std::string& stderrText, const std::string& headerKeyword, std::vector<std::string>& registers) {
  std::istringstream stream(stderrText);
  std::string line;
  bool inMatchingBlock = false;
  bool foundHeader = false;
  while(std::getline(stream, line)) {
    if(line.compare(0, 8, "Warning:") == 0) {
      // A second warning block anywhere means the warning was not emitted exactly once.
      if(foundHeader) {
        return false;
      }
      if(line.find(headerKeyword) != std::string::npos) {
        foundHeader = true;
        inMatchingBlock = true;
      }
      continue;
    }
    if(inMatchingBlock) {
      if(line.compare(0, 2, "  ") == 0) {
        registers.push_back(line.substr(2));
      }
      else {
        inMatchingBlock = false;
      }
    }
  }
  return foundHeader;
}

/**********************************************************************************************************************/

/** Collect the paths of all writeable wait_for_new_data (non-void) registers of a catalogue. */
static std::set<std::string> collectWritableInterrupts(const RegisterCatalogue& catalogue) {
  std::set<std::string> result;
  for(const auto& registerInfo : catalogue) {
    if(registerInfo.isWriteable() &&
        registerInfo.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data) &&
        registerInfo.getDataDescriptor().fundamentalType() != DataDescriptor::FundamentalType::nodata) {
      result.insert(registerInfo.getRegisterName());
    }
  }
  return result;
}

/**********************************************************************************************************************/

/** Collect the paths of all two-dimensional registers of a catalogue. */
static std::set<std::string> collectTwoDimensional(const RegisterCatalogue& catalogue) {
  std::set<std::string> result;
  for(const auto& registerInfo : catalogue) {
    if(registerInfo.getNumberOfDimensions() > 1) {
      result.insert(registerInfo.getRegisterName());
    }
  }
  return result;
}

/**********************************************************************************************************************/

/** Assert that the tool warning listed exactly the given affected register paths. */
static void checkWarningLists(
    const std::string& stderrText, const std::string& headerKeyword, const std::set<std::string>& affected) {
  std::vector<std::string> listed;
  BOOST_REQUIRE(extractWarningRegisters(stderrText, headerKeyword, listed));
  std::set<std::string> listedSet(listed.begin(), listed.end());
  // Exactly one (single) warning block must have been emitted, so no register may be listed twice.
  BOOST_TEST(listedSet.size() == listed.size());
  BOOST_TEST(listedSet == affected);
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/* Test cases                                                                                                        */
/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestRoundTripViaNumericAddressedDevice) {
  // Round trip via the numeric-addressed DummyBackend on simpleJsonFile.jmap. The generated jmap re-opened on a
  // DummyBackend must expose a matching one-dimensional register catalogue.
  size_t mismatches =
      roundTrip(dummySourceCdd, std::format(R"(--device "{}" --output {})", dummySourceCdd, "devToJmap_dummy.jmap"),
          "devToJmap_dummy.jmap", "devToJmap_dummy.jmap.stdout", "devToJmap_dummy.jmap.stderr");
  BOOST_TEST(mismatches == 0U);
  cleanup({"devToJmap_dummy.jmap", "devToJmap_dummy.jmap.stdout", "devToJmap_dummy.jmap.stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestRoundTripViaNonNumericDevice) {
  // Round trip via the non-numeric LogicalNameMapping backend on valid.xlmap. Demonstrates that the tool is
  // independent of numeric addressing. Writable wait_for_new_data variables become read-only interrupts.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  size_t mismatches =
      roundTrip("LMAP0", std::format(R"(--device LMAP0 --dmap {} --output {})", lmapDmap, "devToJmap_lmaprt.jmap"),
          "devToJmap_lmaprt.jmap", "devToJmap_lmaprt.jmap.stdout", "devToJmap_lmaprt.jmap.stderr");
  BOOST_TEST(mismatches == 0U);
  cleanup({"devToJmap_lmaprt.jmap", "devToJmap_lmaprt.jmap.stdout", "devToJmap_lmaprt.jmap.stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestNumericIntegralTypes) {
  // The plain xlmap variables cover each integral numeric type; they must be preserved faithfully (signedness,
  // integrality, element size) in the generated jmap.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  auto catalogue = openGenerated(outputFile);

  BOOST_TEST(catalogue.hasRegister("/var_int8"));
  BOOST_TEST(catalogue.getRegister("/var_int8").getDataDescriptor().minimumDataType().getNumberOfBytes() == 1U);
  BOOST_TEST(catalogue.getRegister("/var_int8").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_int8").getDataDescriptor().isIntegral());
  BOOST_TEST(catalogue.getRegister("/var_uint8").getDataDescriptor().minimumDataType().getNumberOfBytes() == 1U);
  BOOST_TEST(!catalogue.getRegister("/var_uint8").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_int16").getDataDescriptor().minimumDataType().getNumberOfBytes() == 2U);
  BOOST_TEST(catalogue.getRegister("/var_int16").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_uint16").getDataDescriptor().minimumDataType().getNumberOfBytes() == 2U);
  BOOST_TEST(!catalogue.getRegister("/var_uint16").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_int32").getDataDescriptor().minimumDataType().getNumberOfBytes() == 4U);
  BOOST_TEST(catalogue.getRegister("/var_int32").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_uint32").getDataDescriptor().minimumDataType().getNumberOfBytes() == 4U);
  BOOST_TEST(!catalogue.getRegister("/var_uint32").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_int64").getDataDescriptor().minimumDataType().getNumberOfBytes() == 8U);
  BOOST_TEST(catalogue.getRegister("/var_int64").getDataDescriptor().isSigned());
  BOOST_TEST(catalogue.getRegister("/var_uint64").getDataDescriptor().minimumDataType().getNumberOfBytes() == 8U);
  BOOST_TEST(!catalogue.getRegister("/var_uint64").getDataDescriptor().isSigned());

  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestFloatingPointTypes) {
  // IEEE754 payloads (var_float32, var_float64) must be preserved as non-integral numeric types.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  auto catalogue = openGenerated(outputFile);

  BOOST_TEST(catalogue.hasRegister("/var_float32"));
  BOOST_TEST(catalogue.getRegister("/var_float32").getDataDescriptor().minimumDataType().getNumberOfBytes() == 4U);
  BOOST_TEST(!catalogue.getRegister("/var_float32").getDataDescriptor().isIntegral());
  BOOST_TEST(catalogue.getRegister("/var_float64").getDataDescriptor().minimumDataType().getNumberOfBytes() == 8U);
  BOOST_TEST(!catalogue.getRegister("/var_float64").getDataDescriptor().isIntegral());

  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestStringRegisters) {
  // Strings are written with the ASCII string representation; var_string covers the fixed-bytesPerElement path and
  // BSP/SOME_INFO the numeric-addressed path.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto lmapFile = generateFromLMap();
  auto lmapCatalogue = openGenerated(lmapFile);
  BOOST_TEST(lmapCatalogue.hasRegister("/var_string"));
  BOOST_TEST(lmapCatalogue.getRegister("/var_string").getDataDescriptor().fundamentalType() ==
      DataDescriptor::FundamentalType::string);
  cleanup({lmapFile, lmapFile + ".stdout", lmapFile + ".stderr"});

  auto dummyFile = generateFromDummy("devToJmap_dummy_string.jmap");
  auto dummyCatalogue = openGenerated(dummyFile);
  BOOST_TEST(dummyCatalogue.hasRegister("/BSP/SOME_INFO"));
  BOOST_TEST(dummyCatalogue.getRegister("/BSP/SOME_INFO").getDataDescriptor().fundamentalType() ==
      DataDescriptor::FundamentalType::string);
  cleanup({dummyFile, dummyFile + ".stdout", dummyFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestStringLengthOption) {
  // --string-length sets the fixed byte size per element assumed for ASCII string registers (default 80). A custom
  // value writes the string registers with that bytesPerElement, while the default of 80 is implied when the option
  // is omitted.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  std::string customFile = "devToJmap_strlen16.jmap";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device LMAP0 --dmap {} --string-length 16 --output {})", lmapDmap, customFile),
      customFile + ".stdout", customFile + ".stderr", exitCode));
  BOOST_REQUIRE(exitCode == 0);
  nlohmann::json customJmap = nlohmann::json::parse(readFile(customFile));
  BOOST_TEST(customJmap["addressSpace"]["var_string"]["bytesPerElement"] == 16U);
  cleanup({customFile, customFile + ".stdout", customFile + ".stderr"});

  auto defaultFile = generateFromLMap();
  nlohmann::json defaultJmap = nlohmann::json::parse(readFile(defaultFile));
  BOOST_TEST(defaultJmap["addressSpace"]["var_string"]["bytesPerElement"] == 80U);
  cleanup({defaultFile, defaultFile + ".stdout", defaultFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestVoidInterruptRegisters) {
  // Pure interrupt sources are written as void registers (BSP/VOID_INTERRUPT_0 and BSP/VOID_INTERRUPT_3_0_1).
  auto dummyFile = generateFromDummy("devToJmap_dummy_void.jmap");
  auto catalogue = openGenerated(dummyFile);
  BOOST_TEST(catalogue.hasRegister("/BSP/VOID_INTERRUPT_0"));
  BOOST_TEST(catalogue.getRegister("/BSP/VOID_INTERRUPT_0").getDataDescriptor().fundamentalType() ==
      DataDescriptor::FundamentalType::nodata);
  BOOST_TEST(catalogue.hasRegister("/BSP/VOID_INTERRUPT_3_0_1"));
  BOOST_TEST(catalogue.getRegister("/BSP/VOID_INTERRUPT_3_0_1").getDataDescriptor().fundamentalType() ==
      DataDescriptor::FundamentalType::nodata);
  cleanup({dummyFile, dummyFile + ".stdout", dummyFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestBooleanRegisters) {
  // Boolean registers (IEEE-bit width 1, e.g. ENA and INACTIVE_BUF_ID in DOUBLE_BUF) are represented as values, not
  // as strings or void.
  auto dummyFile = generateFromDummy("devToJmap_dummy_bool.jmap");
  auto catalogue = openGenerated(dummyFile);
  for(const std::string& path : {std::string("/DAQ/DOUBLE_BUF/ENA"), std::string("/DAQ/DOUBLE_BUF/INACTIVE_BUF_ID")}) {
    BOOST_TEST(catalogue.hasRegister(path));
    const auto fundamentalType = catalogue.getRegister(path).getDataDescriptor().fundamentalType();
    const bool isNumericOrBool = fundamentalType == DataDescriptor::FundamentalType::numeric ||
        fundamentalType == DataDescriptor::FundamentalType::boolean;
    BOOST_TEST(isNumericOrBool);
    BOOST_TEST(catalogue.getRegister(path).isWriteable());
  }
  cleanup({dummyFile, dummyFile + ".stdout", dummyFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestConstants) {
  // Constants are read-only registers (Constant, Constant2, ArrayConstant); ArrayConstant has five elements.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  auto catalogue = openGenerated(outputFile);

  BOOST_TEST(catalogue.hasRegister("/Constant"));
  BOOST_TEST(!catalogue.getRegister("/Constant").isWriteable());
  BOOST_TEST(catalogue.getRegister("/Constant").isReadable());
  BOOST_TEST(catalogue.getRegister("/Constant").getNumberOfElements() == 1U);
  BOOST_TEST(catalogue.hasRegister("/Constant2"));
  BOOST_TEST(!catalogue.getRegister("/Constant2").isWriteable());
  BOOST_TEST(catalogue.hasRegister("/ArrayConstant"));
  BOOST_TEST(!catalogue.getRegister("/ArrayConstant").isWriteable());
  BOOST_TEST(catalogue.getRegister("/ArrayConstant").getNumberOfElements() == 5U);

  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestArrayRegisters) {
  // Array registers preserve their number of elements (ArrayVariable has six, ArrayConstant five).
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  auto catalogue = openGenerated(outputFile);
  BOOST_TEST(catalogue.hasRegister("/ArrayVariable"));
  BOOST_TEST(catalogue.getRegister("/ArrayVariable").getNumberOfElements() == 6U);
  BOOST_TEST(catalogue.getRegister("/ArrayConstant").getNumberOfElements() == 5U);
  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestModuleHierarchy) {
  // Module hierarchy is reflected in the register paths (MyModule.SomeSubmodule.Variable).
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  auto catalogue = openGenerated(outputFile);
  BOOST_TEST(catalogue.hasRegister("/MyModule/SomeSubmodule/Variable"));
  BOOST_TEST(catalogue.getRegister("/MyModule/SomeSubmodule/Variable").getNumberOfElements() == 1U);
  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestAccessRights) {
  // Access rights RW/RO/WO survive the round trip: SomeTopLevelRegister is RW, BSP/VERSION is RO, APP/SomeTable is WO.
  auto dummyFile = generateFromDummy("devToJmap_dummy_access.jmap");
  auto catalogue = openGenerated(dummyFile);

  BOOST_TEST(catalogue.hasRegister("/SomeTopLevelRegister"));
  BOOST_TEST(catalogue.getRegister("/SomeTopLevelRegister").isWriteable());
  BOOST_TEST(catalogue.getRegister("/SomeTopLevelRegister").isReadable());
  BOOST_TEST(catalogue.hasRegister("/BSP/VERSION"));
  BOOST_TEST(!catalogue.getRegister("/BSP/VERSION").isWriteable());
  BOOST_TEST(catalogue.getRegister("/BSP/VERSION").isReadable());
  BOOST_TEST(catalogue.hasRegister("/APP/SomeTable"));
  BOOST_TEST(catalogue.getRegister("/APP/SomeTable").isWriteable());
  BOOST_TEST(!catalogue.getRegister("/APP/SomeTable").isReadable());

  cleanup({dummyFile, dummyFile + ".stdout", dummyFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestWritableInterruptsBecomeReadOnly) {
  // Writable registers with AccessMode::wait_for_new_data (the xlmap variables) are written as read-only interrupts,
  // so they must no longer be writable in the generated catalogue.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();
  Device device;
  device.open("(dummy?map=" + outputFile + ")");
  auto fullCatalogue = device.getRegisterCatalogue();
  for(const std::string& path :
      {std::string("/var_int32"), std::string("/var_float64"), std::string("/ArrayVariable")}) {
    auto registerInfo = fullCatalogue.getRegister(path);
    BOOST_TEST(!registerInfo.isWriteable());
    BOOST_TEST(registerInfo.isReadable());
    BOOST_TEST(registerInfo.getSupportedAccessModes().has(ChimeraTK::AccessMode::wait_for_new_data));
  }

  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestEmptyCatalogue) {
  // An empty device catalogue yields a valid, empty jmap file and no warning.
  std::string emptySource = "devToJmap_empty_source.jmap";
  {
    std::ofstream stream(emptySource);
    stream << R"({
  "mapFormatVersion": "0.0.1",
  "metadata": {},
  "interruptHandler": {},
  "addressSpace": {}
})";
  }
  std::string outputFile = "devToJmap_empty_out.jmap";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}" --output {})", "(dummy?map=" + emptySource + ")", outputFile),
      outputFile + ".stdout", outputFile + ".stderr", exitCode));
  BOOST_TEST(exitCode == 0);
  BOOST_TEST(readFile(outputFile + ".stderr").empty());

  auto catalogue = openGenerated(outputFile);
  BOOST_TEST(catalogue.getNumberOfRegisters() == 0U);

  cleanup({emptySource, outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestTwoDimensionalSkippedWithWarning) {
  // Two-dimensional registers are unsupported and skipped with a single warning listing every affected register,
  // but a valid partial jmap is produced and the remaining (one-dimensional) registers are kept.
  Device source;
  source.open(dummySourceCdd);
  auto affected = collectTwoDimensional(source.getRegisterCatalogue());

  auto dummyFile = generateFromDummy("devToJmap_dummy_2d.jmap");
  BOOST_TEST(!affected.empty());
  checkWarningLists(readFile(dummyFile + ".stderr"), "two-dimensional registers", affected);

  auto catalogue = openGenerated(dummyFile);
  // The 2D parents must not be present, while a one-dimensional register remains.
  for(const auto& path : affected) {
    BOOST_TEST(!catalogue.hasRegister(path));
  }
  BOOST_TEST(catalogue.hasRegister("/DAQ/DOUBLE_BUF/ENA"));

  cleanup({dummyFile, dummyFile + ".stdout", dummyFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestAllTwoDimensionalSkipped) {
  // Edge case: a device whose catalogue consists solely of two-dimensional registers. The 2D parent is skipped with
  // a warning, while its one-dimensional channel children are still written; the tool exits successfully.
  std::string sourceFile = "devToJmap_2donly_source.jmap";
  {
    // Build the 2D-only source fixture by loading the existing test fixture tests/muxedDataAccessor.jmap and keeping
    // only the single two-dimensional register TEST/DMA.
    using nlohmann::json;
    json source = json::parse(readFile("muxedDataAccessor.jmap"));
    std::ofstream stream(sourceFile);
    stream << json{{"mapFormatVersion", source["mapFormatVersion"]}, {"metadata", json::object()},
        {"interruptHandler", json::object()},
        {"addressSpace",
            json{{"TEST", json{{"children", json{{"DMA", source["addressSpace"]["TEST"]["children"]["DMA"]}}}}}}}};
  }
  std::string outputFile = "devToJmap_2donly_out.jmap";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device '(dummy?map={})' --output {})", sourceFile, outputFile),
      outputFile + ".stdout", outputFile + ".stderr", exitCode));
  BOOST_TEST(exitCode == 0);
  BOOST_TEST(readFile(outputFile + ".stderr").find("two-dimensional registers") != std::string::npos);

  auto catalogue = openGenerated(outputFile);
  // The two-dimensional register itself is skipped, yet its one-dimensional channel children are kept.
  BOOST_TEST(!catalogue.hasRegister("/TEST/DMA"));
  BOOST_TEST(catalogue.hasRegister("/TEST/DMA/0"));
  BOOST_TEST(catalogue.hasRegister("/TEST/DMA/15"));

  cleanup({sourceFile, outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestWritableInterruptWarning) {
  // Writable wait_for_new_data registers produce a single stderr warning listing every affected register.
  BackendFactory::getInstance().setDMapFilePath(lmapDmap);
  auto outputFile = generateFromLMap();

  Device source;
  source.open("LMAP0");
  auto affected = collectWritableInterrupts(source.getRegisterCatalogue());
  BOOST_TEST(!affected.empty());
  checkWarningLists(readFile(outputFile + ".stderr"), "writable registers", affected);

  cleanup({outputFile, outputFile + ".stdout", outputFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestMissingOutputOption) {
  // Running without --output must exit with an error and write nothing to stdout.
  std::string stdoutFile = "devToJmap_noout.stdout";
  std::string stderrFile = "devToJmap_noout.stderr";
  int exitCode = 0;
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}")", dummySourceCdd), stdoutFile, stderrFile, exitCode));
  BOOST_TEST(exitCode != 0);
  BOOST_TEST(readFile(stdoutFile).empty());
  cleanup({stdoutFile, stderrFile});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestHelpOption) {
  // --help prints the usage text to stdout and exits successfully without requiring a device.
  std::string stdoutFile = "devToJmap_help.stdout";
  std::string stderrFile = "devToJmap_help.stderr";
  int exitCode = -1;
  BOOST_REQUIRE(runTool("--help", stdoutFile, stderrFile, exitCode));
  BOOST_TEST(exitCode == 0);
  BOOST_TEST(readFile(stdoutFile).find("Usage:") != std::string::npos);
  cleanup({stdoutFile, stderrFile});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestDeterminism) {
  // Identical input must yield byte-identical jmap files on consecutive runs.
  auto firstFile = generateFromDummy("devToJmap_det_a.jmap");
  std::string first = readFile(firstFile);
  int exitCode = -1;
  std::string secondFile = "devToJmap_det_b.jmap";
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}" --output {})", dummySourceCdd, secondFile), secondFile + ".stdout",
      secondFile + ".stderr", exitCode));
  BOOST_TEST(exitCode == 0);
  BOOST_TEST(first == readFile(secondFile));
  cleanup({firstFile, firstFile + ".stdout", firstFile + ".stderr", secondFile, secondFile + ".stdout",
      secondFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestIdentificationModes) {
  // A CDD and an alias (resolved via --dmap) addressing equivalent devices must produce the same jmap, and --dmap
  // must never change how --device is interpreted.
  std::string aliasDmap = "devToJmap_id.dmap";
  {
    std::ofstream stream(aliasDmap);
    stream << "DUMMYJ   (dummy?map=simpleJsonFile.jmap)\n";
  }

  std::string cddFile = "devToJmap_id_cdd.jmap";
  std::string aliasFile = "devToJmap_id_alias.jmap";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}" --output {})", dummySourceCdd, cddFile), cddFile + ".stdout",
      cddFile + ".stderr", exitCode));
  BOOST_TEST(exitCode == 0);
  BOOST_REQUIRE(runTool(std::format(R"(--device DUMMYJ --dmap {} --output {})", aliasDmap, aliasFile),
      aliasFile + ".stdout", aliasFile + ".stderr", exitCode));
  BOOST_TEST(exitCode == 0);

  BOOST_TEST(readFile(cddFile) == readFile(aliasFile));

  cleanup({aliasDmap, cddFile, cddFile + ".stdout", cddFile + ".stderr", aliasFile, aliasFile + ".stdout",
      aliasFile + ".stderr"});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestDeviceOpenFailure) {
  // A device that cannot be opened (here an unknown backend type) makes the tool report the ChimeraTK logic_error on
  // stderr and exit with status 1 instead of terminating via an unhandled exception.
  std::string outputFile = "devToJmap_openfail.jmap";
  std::string stdoutFile = "devToJmap_openfail.stdout";
  std::string stderrFile = "devToJmap_openfail.stderr";
  int exitCode = -1;
  BOOST_REQUIRE(runTool(std::format(R"(--device "{}" --output {})", "(noSuchBackend?x=1)", outputFile), stdoutFile,
      stderrFile, exitCode));
  BOOST_TEST(exitCode == 1);
  BOOST_TEST(!readFile(stderrFile).empty());
  cleanup({outputFile, stdoutFile, stderrFile});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
