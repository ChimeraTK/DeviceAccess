#include <boost/test/included/unit_test.hpp>
#include "parserUtilities.h"
using namespace boost::unit_test_framework;


class ParserUtilsTestClass {
public:
  ParserUtilsTestClass(std::string const& currentWorkingDirectory);
  void testGetCurrentWorkingDir();
private:
  std::string _currentWorkingDir;
};

class ParserUtilitiesTestSuite : public test_suite {
public:
  ParserUtilitiesTestSuite(std::string const& currentWorkingDirectory)
          : test_suite("ParserUtilitiesTestSuite") {
    boost::shared_ptr<ParserUtilsTestClass> parserUtilTest(new ParserUtilsTestClass(currentWorkingDirectory));
    add(BOOST_CLASS_TEST_CASE(&ParserUtilsTestClass::testGetCurrentWorkingDir, parserUtilTest));
  }
};

boost::unit_test::test_suite* init_unit_test_suite(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << "currentWorkingDir" << std::endl;
    return NULL;
  }
  return new ParserUtilitiesTestSuite(argv[1]);
}

void ParserUtilsTestClass::testGetCurrentWorkingDir() {
  BOOST_CHECK_EQUAL(mtca4u::parserUtilities::getCurrentWorkingDirectory() == "", true);
  std::cout << mtca4u::parserUtilities::getCurrentWorkingDirectory() << std::endl;
  std::cout << _currentWorkingDir << std::endl;
}

ParserUtilsTestClass::ParserUtilsTestClass(const std::string& currentWorkingDirectory)
    : _currentWorkingDir(currentWorkingDirectory) {}
