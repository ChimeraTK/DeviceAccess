// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "boost_dynamic_init_test.h"
#include "parserUtilities.h"

#include <iostream>
#include <utility>

using namespace boost::unit_test_framework;
namespace parsutils = ChimeraTK::parserUtilities;

// The test cases require the current working directory. This is provided
// to the test suite through the shell script:
// ./tests/scripts/testParserUtilities.sh

/******************************************************************************/
/*                  Code for setting up test suite                            */
/******************************************************************************/
class ParserUtilsTestClass {
 public:
  explicit ParserUtilsTestClass(std::string currentWorkingDirectory);
  void testGetCurrentWorkingDir();
  void testConvertToAbsPath();
  void testExtractDirectory();
  void testExtractFileName();
  void testConcatenatePaths();

 private:
  std::string _currentWorkingDir;
};

class ParserUtilitiesTestSuite : public test_suite {
 public:
  explicit ParserUtilitiesTestSuite(std::string const& currentWorkingDirectory)
  : test_suite("ParserUtilitiesTestSuite") {
    boost::shared_ptr<ParserUtilsTestClass> parserUtilTest(new ParserUtilsTestClass(currentWorkingDirectory));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testGetCurrentWorkingDir, parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testConvertToAbsPath, parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testExtractDirectory, parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testExtractFileName, parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testConcatenatePaths, parserUtilTest));
  }
};

bool init_unit_test() {
  if(framework::master_test_suite().argc < 2) {
    std::cout << "Usage: " << framework::master_test_suite().argv[0] << " currentWorkingDir" << std::endl;
    return false;
  }
  auto* currentWorkingDir = framework::master_test_suite().argv[1];

  framework::master_test_suite().p_name.value = "Rebot backend test suite";
  framework::master_test_suite().add(new ParserUtilitiesTestSuite(currentWorkingDir));

  return true;
}

ParserUtilsTestClass::ParserUtilsTestClass(std::string currentWorkingDirectory)
: _currentWorkingDir(std::move(currentWorkingDirectory)) {}

/******************************************************************************/

/* Test cases... */
void ParserUtilsTestClass::testGetCurrentWorkingDir() {
  std::string expectedReturnValue = _currentWorkingDir + "/";
  BOOST_CHECK(parsutils::getCurrentWorkingDirectory() == expectedReturnValue);
}

void ParserUtilsTestClass::testConvertToAbsPath() {
  BOOST_CHECK(parsutils::convertToAbsolutePath("./test") == _currentWorkingDir + "/" + "./test");
  BOOST_CHECK(parsutils::convertToAbsolutePath("./test/") == _currentWorkingDir + "/" + "./test/");
  BOOST_CHECK(parsutils::convertToAbsolutePath("/test") == "/test");
  BOOST_CHECK(parsutils::convertToAbsolutePath("/test/") == "/test/");
  BOOST_CHECK(parsutils::convertToAbsolutePath("/") == "/");
  BOOST_CHECK(parsutils::convertToAbsolutePath("test") == _currentWorkingDir + "/" + "test");
  BOOST_CHECK(parsutils::convertToAbsolutePath("test/") == _currentWorkingDir + "/" + "test/");
  BOOST_CHECK(parsutils::convertToAbsolutePath("") == _currentWorkingDir + "/");
}

void ParserUtilsTestClass::testExtractDirectory() {
  BOOST_CHECK(parsutils::extractDirectory("./test") == "./");
  BOOST_CHECK(parsutils::extractDirectory("./test/") == "./test/");
  BOOST_CHECK(parsutils::extractDirectory("/test") == "/");
  BOOST_CHECK(parsutils::extractDirectory("/") == "/");
  BOOST_CHECK(parsutils::extractDirectory("/test/") == "/test/");
  BOOST_CHECK(parsutils::extractDirectory("test") == "./");
  BOOST_CHECK(parsutils::extractDirectory("test/") == "test/");
  BOOST_CHECK(parsutils::extractDirectory("") == "./");
}

void ParserUtilsTestClass::testExtractFileName() {
  BOOST_CHECK(parsutils::extractFileName("./test") == "test");
  BOOST_CHECK(parsutils::extractFileName("./test/").empty());
  BOOST_CHECK(parsutils::extractFileName("/test") == "test");
  BOOST_CHECK(parsutils::extractFileName("/test/").empty());
  BOOST_CHECK(parsutils::extractFileName("").empty());
}

void ParserUtilsTestClass::testConcatenatePaths() {
  BOOST_CHECK(parsutils::concatenatePaths("./a", "b") == "./a/b");
  BOOST_CHECK(parsutils::concatenatePaths("./a/", "b") == "./a/b");
  BOOST_CHECK(parsutils::concatenatePaths("./a/", "/b") == "/b");
  BOOST_CHECK(parsutils::concatenatePaths("a", "b") == "a/b");
  BOOST_CHECK(parsutils::concatenatePaths("a/", "b") == "a/b");
  BOOST_CHECK(parsutils::concatenatePaths("a/", "/b") == "/b");
  BOOST_CHECK(parsutils::concatenatePaths("a/", "") == "a/");
  BOOST_CHECK(parsutils::concatenatePaths("", "") == "/");
}
