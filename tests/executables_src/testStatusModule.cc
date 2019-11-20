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
  warning = 45.1;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<double_t>(std::string("/Monitor/upperErrorThreshold"));
  error = 50.1;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/watch"));
  watch = 40.1;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value exceeding warning level
  watch = 46.1;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //set watch value exceeding error level
  watch = 51.1;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //increase error value greater than watch
  error = 60.1;
  error.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //increase warning value greater than watch
  warning = 55.1;
  warning.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value exceeding error level
  watch = 65.1;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //decrease watch value lower than error level but still greater than warning level
  watch = 58.1;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //decrease watch value lower than warning level
  watch = 54.1;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<double>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(error, test.readScalar<double>("/MonitorParameters/Monitor/upperErrorThreshold"));
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
  warning = 50;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<uint>(std::string("/Monitor/lowerErrorThreshold"));
  error = 45;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<uint>(std::string("/watch"));
  watch = 55;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/Monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value lower than warning threshold
  watch = 48;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //set watch value greater than error threshold
  watch = 42;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //decrease error value lower than watch
  error = 35;
  error.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //decrease warning value lower than watch
  warning = 40;
  warning.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value lower than error threshold
  watch = 33;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //decrease watch value greater than error level but still lower than warning level
  watch = 36;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //decrease watch value lower than warning level
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<uint>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(error, test.readScalar<uint>("/MonitorParameters/Monitor/lowerErrorThreshold"));
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

  auto errorUpperLimit = test.getScalar<int>(std::string("/Monitor/upperErrorThreshold"));
  errorUpperLimit = 60;
  errorUpperLimit.write();
  test.stepApplication();

  auto errorLowerLimit = test.getScalar<int>(std::string("/Monitor/lowerErrorThreshold"));
  errorLowerLimit = 30;
  errorLowerLimit.write();
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

  //exactly at the upper warning threshold (only well defined for int)
  watch = 50;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //just below the error threshold,. still warning
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
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //increase well above the upper error level
  watch = 65;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

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

  //just above the lower error limit
  watch = 31;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //exactly at the lower error limit (only well defined for int)
  watch = 30;
  watch.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //way bellow the lower error limit
  errorUpperLimit = 12;
  errorUpperLimit.write();
  test.stepApplication();
  status.readLatest();
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //  errorLowerLimit = 61;
  //  errorLowerLimit.write();
  //  test.stepApplication();

  //  warningUpperLimit = 60;
  //  warningUpperLimit.write();
  //  test.stepApplication();

  //  warningLowerLimit = 51;
  //  warningLowerLimit.write();
  //  test.stepApplication();

  //  status.readLatest();
  //  //should be in WARNING state.
  //  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //  warningLowerLimit = 55;
  //  warningLowerLimit.write();
  //  test.stepApplication();
  //  status.readLatest();
  //  //should be in OK state.
  //  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<int>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(errorLowerLimit, test.readScalar<int>("/MonitorParameters/Monitor/lowerErrorThreshold"));
  BOOST_CHECK_EQUAL(warningLowerLimit, test.readScalar<int>("/MonitorParameters/Monitor/lowerWarningThreshold"));
  BOOST_CHECK_EQUAL(errorUpperLimit, test.readScalar<int>("/MonitorParameters/Monitor/upperErrorThreshold"));
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

  //set watch value different than required value
  watch = 41.4;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

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
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

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

  watch = 0;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  stateValue = 0;
  stateValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in OFF state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OFF);

  // check that the tags are applied correctly
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MyNiceMonitorCopy/Monitor/status"));
  BOOST_CHECK_EQUAL(status, test.readScalar<uint16_t>("/MonitorOutput/Monitor/status"));
  BOOST_CHECK_EQUAL(watch, test.readScalar<uint8_t>("/MyNiceMonitorCopy/watch"));
  BOOST_CHECK_EQUAL(stateValue, test.readScalar<uint8_t>("/MonitorParameters/Monitor/nominalState"));
}
