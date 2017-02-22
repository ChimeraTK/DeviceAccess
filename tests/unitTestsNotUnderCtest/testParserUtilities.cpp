#include <boost/test/included/unit_test.hpp>
#include "parserUtilities.h"

using namespace boost::unit_test_framework;
namespace parsutils = mtca4u::parserUtilities;

// The test cases require the current working directory. This is provided
// to the test suite through the shell script:
// ./tests/scripts/testParserUtilities.sh

/******************************************************************************/
/*                  Code for setting up test suite                            */
/******************************************************************************/
class ParserUtilsTestClass {
public:
  ParserUtilsTestClass(std::string const& currentWorkingDirectory);
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
  ParserUtilitiesTestSuite(std::string const& currentWorkingDirectory)
      : test_suite("ParserUtilitiesTestSuite") {

    boost::shared_ptr<ParserUtilsTestClass> parserUtilTest( new ParserUtilsTestClass(currentWorkingDirectory));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testGetCurrentWorkingDir,
                              parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testConvertToAbsPath,
                              parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testExtractDirectory,
                              parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testExtractFileName,
                              parserUtilTest));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testConcatenatePaths,
                              parserUtilTest));
  }
};

boost::unit_test::test_suite* init_unit_test_suite(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << "currentWorkingDir" << std::endl;
    return NULL;
  }

  return new ParserUtilitiesTestSuite(argv[1]);
}

ParserUtilsTestClass::ParserUtilsTestClass(const std::string& currentWorkingDirectory)
    : _currentWorkingDir(currentWorkingDirectory) {}

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

void ParserUtilsTestClass::testExtractDirectory(){
  BOOST_CHECK(parsutils::extractDirectory("./test") ==  "./");
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
  BOOST_CHECK(parsutils::extractFileName("./test/") == "");
  BOOST_CHECK(parsutils::extractFileName("/test") == "test");
  BOOST_CHECK(parsutils::extractFileName("/test/") == "");
  BOOST_CHECK(parsutils::extractFileName("") == "");
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
