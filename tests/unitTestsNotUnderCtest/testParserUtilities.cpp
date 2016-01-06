#include <boost/test/included/unit_test.hpp>
#include "parserUtilities.h"

using namespace boost::unit_test_framework;
namespace utils = mtca4u::parserUtilities;

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
  void testGetAbsolutePathToDirectory();
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
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testGetAbsolutePathToDirectory,
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
  BOOST_CHECK(utils::getCurrentWorkingDirectory() == expectedReturnValue);
}

void ParserUtilsTestClass::testConvertToAbsPath() {
  BOOST_CHECK(utils::convertToAbsolutePath("./test") == _currentWorkingDir + "/" + "./test");
  BOOST_CHECK(utils::convertToAbsolutePath("./test/") == _currentWorkingDir + "/" + "./test/");
  BOOST_CHECK(utils::convertToAbsolutePath("/test") == "/test");
  BOOST_CHECK(utils::convertToAbsolutePath("/test/") == "/test/");
  BOOST_CHECK(utils::convertToAbsolutePath("/") == "/");
  BOOST_CHECK(utils::convertToAbsolutePath("test") == _currentWorkingDir + "/" + "test");
  BOOST_CHECK(utils::convertToAbsolutePath("test/") == _currentWorkingDir + "/" + "test/");
  BOOST_CHECK(utils::convertToAbsolutePath("") == _currentWorkingDir + "/");
}

void ParserUtilsTestClass::testGetAbsolutePathToDirectory(){
  BOOST_CHECK(utils::getAbsolutePathToDirectory("./test") == _currentWorkingDir + "/" + "./");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("./test/") == _currentWorkingDir + "/" + "./test/");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("/test") == "/");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("/") == "/");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("/test/") == "/test/");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("test") == _currentWorkingDir + "/");
  BOOST_CHECK(utils::getAbsolutePathToDirectory("test/") == _currentWorkingDir + "/" + "test/");
  BOOST_CHECK(utils::convertToAbsolutePath("") == _currentWorkingDir + "/");
}

void ParserUtilsTestClass::testExtractFileName() {
  BOOST_CHECK(utils::extractFileName("./test") == "test");
  BOOST_CHECK(utils::extractFileName("./test/") == "");
  BOOST_CHECK(utils::extractFileName("/test") == "test");
  BOOST_CHECK(utils::extractFileName("/test/") == "");
  BOOST_CHECK(utils::extractFileName("") == "");
}

void ParserUtilsTestClass::testConcatenatePaths() {
  BOOST_CHECK(utils::concatenatePaths("./a", "b") == "./a/b");
  BOOST_CHECK(utils::concatenatePaths("./a/", "b") == "./a/b");
  BOOST_CHECK(utils::concatenatePaths("./a/", "/b") == "/b");
  BOOST_CHECK(utils::concatenatePaths("a", "b") == "a/b");
  BOOST_CHECK(utils::concatenatePaths("a/", "b") == "a/b");
  BOOST_CHECK(utils::concatenatePaths("a/", "/b") == "/b");
  BOOST_CHECK(utils::concatenatePaths("a/", "") == "a/");
  BOOST_CHECK(utils::concatenatePaths("", "") == "/");
}
