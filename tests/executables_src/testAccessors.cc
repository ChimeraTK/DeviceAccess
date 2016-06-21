/*
 * testAccessors.cc
 *
 *  Created on: Jun 21, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

class AccessorTest {
  public:
    //void testExceptions();
    void testScalarPushAccessor();
};

class AccessorTestSuite : public test_suite {
  public:
    AccessorTestSuite() : test_suite("Test suite for accessors") {
      boost::shared_ptr<AccessorTest> test(new AccessorTest);

      add( BOOST_CLASS_TEST_CASE(&AccessorTest::testScalarPushAccessor, test) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "Accessor test suite";
  framework::master_test_suite().add(new AccessorTestSuite());

  return NULL;
}

class TestModule : public ctk::ApplicationModule {
  public:
    SCALAR_ACCESSOR(int, consumingPushInt, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(int, feedingPushInt, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::push);

    void mainLoop() {}
};

class TestApplication : public ctk::Application {
  public:
    using Application::Application;
    using Application::makeConnections;
    void initialise() {}
};
TestApplication app("Test Suite");

void AccessorTest::testScalarPushAccessor() {

  TestModule testModule;

  testModule.feedingPushInt.connectTo(testModule.consumingPushInt);
  app.makeConnections();

  testModule.consumingPushInt = 0;
  testModule.feedingPushInt = 42;
  BOOST_CHECK(testModule.consumingPushInt == 0);
  testModule.feedingPushInt.write();
  BOOST_CHECK(testModule.consumingPushInt == 0);
  testModule.consumingPushInt.read();
  BOOST_CHECK(testModule.consumingPushInt == 42);




}
