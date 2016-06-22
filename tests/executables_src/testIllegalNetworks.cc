/*
 * testIllegalNetworks.cc
 *
 *  Created on: Jun 21, 2016
 *      Author: Martin Hierholzer
 */

#include <future>

#define BOOST_TEST_MODULE testIllegalNetworks

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "ScalarAccessor.h"
#include "ApplicationModule.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t,uint8_t,
                         int16_t,uint16_t,
                         int32_t,uint32_t,
                         float,double>        test_types;

/*********************************************************************************************************************/
/* the ApplicationModule for the test is a template of the user type */

template<typename T>
class TestModule : public ctk::ApplicationModule {
  public:
    SCALAR_ACCESSOR(T, feedingPush, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(T, consumingPush, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(T, consumingPush2, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(T, consumingPush3, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::push);

    SCALAR_ACCESSOR(T, feedingPoll, ctk::VariableDirection::feeding, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(T, consumingPoll, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(T, consumingPoll2, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);
    SCALAR_ACCESSOR(T, consumingPoll3, ctk::VariableDirection::consuming, "MV/m", ctk::UpdateMode::poll);

    void mainLoop() {}
};

/*********************************************************************************************************************/
/* dummy application */

class TestApplication : public ctk::Application {
  public:
    using Application::Application;
    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void initialise() {}                    // the setup is done in the tests
};

/*********************************************************************************************************************/
/* test case for two scalar accessors, feeder in poll mode and consumer in push mode (without trigger) */

BOOST_AUTO_TEST_CASE_TEMPLATE( testTwoScalarPollPushAccessors, T, test_types ) {

  TestApplication app("Test Suite");
  TestModule<T> testModule;

  testModule.feedingPoll.connectTo(testModule.consumingPush);
  try {
    app.makeConnections();
    BOOST_ERROR("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalVariableNetwork> &e) {
  }

}
