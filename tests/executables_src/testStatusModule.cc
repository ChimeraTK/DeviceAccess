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

  void defineConnections() { findTag(".*").connectTo(cs); }
  ctk::ControlSystemModule cs;
  T monitor{this, "monitor", "", ChimeraTK::HierarchyModifier::none, "watch", "status", {"CS"}};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMaxMonitor) {
  std::cout << "testMaxMonitor" << std::endl;
  TestApplication<ctk::MaxMonitor<double_t>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warning = test.getScalar<double_t>(std::string("/monitor/upperWarningThreshold"));
  warning = 45.1;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<double_t>(std::string("/monitor/upperErrorThreshold"));
  error = 50.1;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/watch"));
  watch = 40.1;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/monitor/status"));
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
}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testMinMonitor) {
  std::cout << "testMinMonitor" << std::endl;

  TestApplication<ctk::MinMonitor<uint>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warning = test.getScalar<uint>(std::string("/monitor/lowerWarningThreshold"));
  warning = 50;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<uint>(std::string("/monitor/lowerErrorThreshold"));
  error = 45;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<uint>(std::string("/watch"));
  watch = 55;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/monitor/status"));
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
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeMonitor) {
  std::cout << "testRangeMonitor" << std::endl;
  TestApplication<ctk::RangeMonitor<int>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warningUpperLimit = test.getScalar<int>(std::string("/monitor/upperWarningThreshold"));
  warningUpperLimit = 50;
  warningUpperLimit.write();
  test.stepApplication();

  auto warningLowerLimit = test.getScalar<int>(std::string("/monitor/lowerWarningThreshold"));
  warningLowerLimit = 41;
  warningLowerLimit.write();
  test.stepApplication();

  auto errorUpperLimit = test.getScalar<int>(std::string("/monitor/upperErrorThreshold"));
  errorUpperLimit = 60;
  errorUpperLimit.write();
  test.stepApplication();

  auto errorLowerLimit = test.getScalar<int>(std::string("/monitor/lowerErrorThreshold"));
  errorLowerLimit = 51;
  errorLowerLimit.write();
  test.stepApplication();

  auto watch = test.getScalar<int>(std::string("/watch"));
  watch = 40;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/monitor/status"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);

  //set watch value exceeding warning level
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //still test warning range
  watch = 45;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //Test warningUpperLimit
  watch = 50;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  //Test errorUpperLimit
  watch = 60;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //Still test error
  watch = 58;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //Test errorLowerLimit
  watch = 51;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  //increase error upper limit value greater than watch
  errorUpperLimit = 70;
  errorUpperLimit.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::ERROR);

  errorLowerLimit = 61;
  errorLowerLimit.write();
  test.stepApplication();

  warningUpperLimit = 60;
  warningUpperLimit.write();
  test.stepApplication();

  warningLowerLimit = 51;
  warningLowerLimit.write();
  test.stepApplication();

  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::WARNING);

  warningLowerLimit = 55;
  warningLowerLimit.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status, ChimeraTK::States::OK);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExactMonitor) {
  std::cout << "testExactMonitor" << std::endl;
  TestApplication<ctk::ExactMonitor<float>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto requiredValue = test.getScalar<float>(std::string("/monitor/requiredValue"));
  requiredValue = 40.9;
  requiredValue.write();
  test.stepApplication();

  auto watch = test.getScalar<float>(std::string("/watch"));
  watch = 40.9;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/monitor/status"));
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
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testStateMonitor) {
  std::cout << "testStateMonitor" << std::endl;
  TestApplication<ctk::StateMonitor<uint8_t>> app;

  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto stateValue = test.getScalar<uint8_t>(std::string("/monitor/nominalState"));
  stateValue = 1;
  stateValue.write();
  test.stepApplication();

  auto watch = test.getScalar<uint8_t>(std::string("/watch"));
  watch = 1;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/monitor/status"));
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
}
