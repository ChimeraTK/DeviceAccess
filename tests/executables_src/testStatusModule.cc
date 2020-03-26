#include <future>

#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>

#include "Application.h"
#include "ApplicationModule.h"
#include "ControlSystemModule.h"
#include "ScalarAccessor.h"
#include "TestFacility.h"
#include "StatusMonitor.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/* dummy application */
template<typename T>
struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }

  void defineConnections() {
    findTag(".*").connectTo(cs);                              // publish everything to CS
    findTag("MY_MONITOR").connectTo(cs["MyNiceMonitorCopy"]); // cable again, checking that the tag is applied correctly
    findTag("MON_PARAMS")
        .connectTo(cs["MonitorParameters"]); // cable the parameters in addition (checking that tags are set correctly)
    findTag("MON_OUTPUT")
        .connectTo(cs["MonitorOutput"]); // cable the parameters in addition (checking that tags are set correctly)
  }
  ctk::ControlSystemModule cs;
  T monitor{this, "Monitor", "Now this is a nice monitor...", "watch", "status", ChimeraTK::HierarchyModifier::none,
      {"MON_OUTPUT"}, {"MON_PARAMS"}, {"MY_MONITOR"}};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMaxMonitor) {
  std::cout << "testMaxMonitor" << std::endl;
  TestApplication<ctk::MaxMonitor<double_t>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.cs.dump();

  auto warning = test.getScalar<double_t>(std::string("/Monitor/upperWarningThreshold"));
  warning = 50.0;
  warning.write();
  test.stepApplication();

  auto fault = test.getScalar<double_t>(std::string("/Monitor/upperFaultThreshold"));
  fault = 60.0;
  fault.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/watch"));
  watch = 40.0;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //   //just below the warning level
  watch = 49.99;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // drop in a disable test.
  auto disable = test.getScalar<int>("/Monitor/disable");
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // slightly above at the upper warning threshold (exact is not good due to rounding errors in floats/doubles)
  watch = 50.01;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //just below the fault threshold,. still warning
  watch = 59.99;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // slightly above at the upper fault threshold (exact is not good due to rounding errors in floats/doubles)
  watch = 60.01;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //increase well above the upper fault level
  watch = 65;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // now check that changing the status is updated correctly if we change the limits

  //increase fault value greater than watch
  fault = 68;
  fault.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //increase warning value greater than watch
  warning = 66;
  warning.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // Set the upper fault limit below the upper warning limit and below the current temperature. The warning is not active, but the fault.
  // Although this is not a reasonable configuration the fault limit must superseed the warning and the status has to be fault.
  fault = 60;
  fault.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<double>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(fault, test.readScalar<double>("/MonitorParameters/Monitor/upperFaultThreshold"));
  BOOST_CHECK_EQUAL(warning, test.readScalar<double>("/MonitorParameters/Monitor/upperWarningThreshold"));
}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testMinMonitor) {
  std::cout << "testMinMonitor" << std::endl;

  TestApplication<ctk::MinMonitor<uint>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warning = test.getScalar<uint>(std::string("/Monitor/lowerWarningThreshold"));
  warning = 40;
  warning.write();
  test.stepApplication();

  auto fault = test.getScalar<uint>(std::string("/Monitor/lowerFaultThreshold"));
  fault = 30;
  fault.write();
  test.stepApplication();

  auto watch = test.getScalar<uint>(std::string("/watch"));
  watch = 45;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // just abow the lower warning limit
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // drop in a disable test.
  auto disable = test.getScalar<int>("/Monitor/disable");
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //exactly at the lower warning limit
  watch = 40;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //just above the lower fault limit
  watch = 31;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //exactly at the lower fault limit (only well defined for int)
  watch = 30;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //way bellow the lower fault limit
  watch = 12;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // move the temperature back to the good range and check that the status updates correctly when changing the limits
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // change upper warning limit
  warning = 42;
  warning.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // rise the temperature above the lower warning limit
  watch = 43;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // Set the lower fault limit above the lower warning limit. The warning is not active, but the fault.
  // Although this is not a reasonable configuration the fault limit must superseed the warning and the status has to be fault.
  fault = 44;
  fault.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<uint>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(fault, test.readScalar<uint>("/MonitorParameters/Monitor/lowerFaultThreshold"));
  BOOST_CHECK_EQUAL(warning, test.readScalar<uint>("/MonitorParameters/Monitor/lowerWarningThreshold"));
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeMonitor) {
  std::cout << "testRangeMonitor" << std::endl;
  TestApplication<ctk::RangeMonitor<int>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warningUpperLimit = test.getScalar<int>(std::string("/Monitor/upperWarningThreshold"));
  warningUpperLimit = 50;
  warningUpperLimit.write();
  test.stepApplication();

  auto warningLowerLimit = test.getScalar<int>(std::string("/Monitor/lowerWarningThreshold"));
  warningLowerLimit = 40;
  warningLowerLimit.write();
  test.stepApplication();

  auto faultUpperLimit = test.getScalar<int>(std::string("/Monitor/upperFaultThreshold"));
  faultUpperLimit = 60;
  faultUpperLimit.write();
  test.stepApplication();

  auto faultLowerLimit = test.getScalar<int>(std::string("/Monitor/lowerFaultThreshold"));
  faultLowerLimit = 30;
  faultLowerLimit.write();
  test.stepApplication();

  // start with a good value
  auto watch = test.getScalar<int>(std::string("/watch"));
  watch = 45;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //just below the warning level
  watch = 49;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // drop in a disable test.
  auto disable = test.getScalar<int>("/Monitor/disable");
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //exactly at the upper warning threshold (only well defined for int)
  watch = 50;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //just below the fault threshold,. still warning
  watch = 59;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //exactly at the upper warning threshold (only well defined for int)
  watch = 60;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //increase well above the upper fault level
  watch = 65;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //back to ok, just abow the lower warning limit
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //exactly at the lower warning limit
  watch = 40;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //just above the lower fault limit
  watch = 31;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //exactly at the lower fault limit (only well defined for int)
  watch = 30;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //way bellow the lower fault limit
  watch = 12;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // Put the value back to the good range, then check that chaning the threshold also updated the status
  watch = 49;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // change upper warning limit
  warningUpperLimit = 48;
  warningUpperLimit.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // lower the temperature below the upper warning limit
  watch = 47;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // Set the upper fault limit below the upper warning limit. The warning is not active, but the fault.
  // Although this is not a reasonable configuration the fault limit must superseed the warning and the status has to be fault.
  faultUpperLimit = 46;
  faultUpperLimit.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // move the temperature back to the good range and repeat for the lower limits
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // change upper warning limit
  warningLowerLimit = 42;
  warningLowerLimit.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  // rise the temperature above the lower warning limit
  watch = 43;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // Set the lower fault limit above the lower warning limit. The warning is not active, but the fault.
  // Although this is not a reasonable configuration the fault limit must superseed the warning and the status has to be fault.
  faultLowerLimit = 44;
  faultLowerLimit.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<int>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(faultLowerLimit, test.readScalar<int>("/MonitorParameters/Monitor/lowerFaultThreshold"));
  BOOST_CHECK_EQUAL(warningLowerLimit, test.readScalar<int>("/MonitorParameters/Monitor/lowerWarningThreshold"));
  BOOST_CHECK_EQUAL(faultUpperLimit, test.readScalar<int>("/MonitorParameters/Monitor/upperFaultThreshold"));
  BOOST_CHECK_EQUAL(warningUpperLimit, test.readScalar<int>("/MonitorParameters/Monitor/upperWarningThreshold"));
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExactMonitor) {
  std::cout << "testExactMonitor" << std::endl;
  TestApplication<ctk::ExactMonitor<float>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto requiredValue = test.getScalar<float>(std::string("/Monitor/requiredValue"));
  requiredValue = 40.9;
  requiredValue.write();
  test.stepApplication();

  auto watch = test.getScalar<float>(std::string("/watch"));
  watch = 40.9;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // drop in a disable test.
  auto disable = test.getScalar<int>("/Monitor/disable");
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value different than required value
  watch = 41.4;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in FAULT state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  watch = 40.9;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set requiredValue value different than watch value
  requiredValue = 41.3;
  requiredValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  //set requiredValue value equals to watch value
  requiredValue = 40.9;
  requiredValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<float>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(requiredValue, test.readScalar<float>("/MonitorParameters/Monitor/requiredValue"));
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testStateMonitor) {
  std::cout << "testStateMonitor" << std::endl;
  TestApplication<ctk::StateMonitor<uint8_t>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto stateValue = test.getScalar<uint8_t>(std::string("/Monitor/nominalState"));
  stateValue = 1;
  stateValue.write();
  test.stepApplication();

  auto watch = test.getScalar<uint8_t>(std::string("/watch"));
  watch = 1;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // drop in a disable test.
  auto disable = test.getScalar<int>("/Monitor/disable");
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  watch = 0;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in FAULT state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  // drop in a disable test.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);

  stateValue = 0;
  stateValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in OFF state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  // State change while disabled is detected.
  disable=1;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  // it is still disabled
  stateValue = 1; // should be OK, but watch is still 0  => result would be FAULT if it was not disabled
  stateValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in OFF state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  // after enabling the FAULT becomes visible
  disable=0;
  disable.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::FAULT);


  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<uint8_t>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(stateValue, test.readScalar<uint8_t>("/MonitorParameters/Monitor/nominalState"));
}
