#include <future>

#define BOOST_TEST_MODULE testExceptionHandling

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

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

  void defineConnections() {} // the setup is done in the tests
  ctk::ControlSystemModule cs;
  T monitor{this, "MONITOR","",ChimeraTK::HierarchyModifier::none,"WATCH","STATUS",{"CS"}};
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMaxMonitor) {
  std::cout << "testMaxMonitor" << std::endl;
  TestApplication<ctk::MaxMonitor> app;

  app.monitor.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warning = test.getScalar<double_t>(std::string("/MAX_MONITOR.WARNING.THRESHOLD"));
  warning = 45;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<double_t>(std::string("/MAX_MONITOR.ERROR.THRESHOLD"));
  error = 50;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
  watch = 40;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value exceeding warning level
  watch = 46;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //set watch value exceeding error level
  watch = 51;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //increase error value greater than watch
  error = 60;
  error.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //increase warning value greater than watch
  warning = 55;
  warning.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value exceeding error level
  watch = 65;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //decrease watch value lower than error level but still greater than warning level
  watch = 58;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //decrease watch value lower than warning level
  watch = 54;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
}

/*********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testMinMonitor) {
  std::cout << "testMinMonitor" << std::endl;

  TestApplication<ctk::MinMonitor> app;

  app.monitor.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto warning = test.getScalar<double_t>(std::string("MIN_MONITOR.WARNING.THRESHOLD"));
  warning = 50;
  warning.write();
  test.stepApplication();

  auto error = test.getScalar<double_t>(std::string("MIN_MONITOR.ERROR.THRESHOLD"));
  error = 45;
  error.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
  watch = 55;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value lower than warning threshold
  watch = 48;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //set watch value greater than error threshold
  watch = 42;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //decrease error value lower than watch
  error = 35;
  error.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //decrease warning value lower than watch
  warning = 40;
  warning.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value lower than error threshold
  watch = 33;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //decrease watch value greater than error level but still lower than warning level
  watch = 36;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //decrease watch value lower than warning level
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRangeMonitor) {
  std::cout << "testRangeMonitor" << std::endl;
  TestApplication<ctk::RangeMonitor> app;
  
  app.monitor.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();
  
  auto warningUpperLimit = test.getScalar<double_t>(std::string("/RANGE_MONITOR.WARNING.UPPER_LIMIT"));
  warningUpperLimit = 50;
  warningUpperLimit.write();
  test.stepApplication();
  
  auto warningLowerLimit = test.getScalar<double_t>(std::string("/RANGE_MONITOR.WARNING.LOWER_LIMIT"));
  warningLowerLimit = 41;
  warningLowerLimit.write();
  test.stepApplication();
  
  auto errorUpperLimit = test.getScalar<double_t>(std::string("/RANGE_MONITOR.ERROR.UPPER_LIMIT"));
  errorUpperLimit = 60;
  errorUpperLimit.write();
  test.stepApplication();
  
  auto errorLowerLimit = test.getScalar<double_t>(std::string("/RANGE_MONITOR.ERROR.LOWER_LIMIT"));
  errorLowerLimit = 51;
  errorLowerLimit.write();
  test.stepApplication();
  
  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
  watch = 40;
  watch.write();
  test.stepApplication();
  
  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
  status.readLatest();
  
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value exceeding warning level
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //still test warning range
  watch = 45;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //Test warningUpperLimit
  watch = 50;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  //Test errorUpperLimit
  watch = 60;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //Still test error
  watch = 58;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //Test errorLowerLimit
  watch = 51;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //increase error upper limit value greater than watch
  errorUpperLimit = 70;
  errorUpperLimit.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

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
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::WARNING);

  warningLowerLimit = 55;
  warningLowerLimit.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExactMonitor) {
  std::cout << "testExactMonitor" << std::endl;
  TestApplication<ctk::ExactMonitor> app;

  app.monitor.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto requiredValue = test.getScalar<double_t>(std::string("/EXACT_MONITOR.REQUIRED_VALUE"));
  requiredValue = 40;
  requiredValue.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
  watch = 40;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
  status.readLatest();

  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set watch value different than required value
  watch = 41;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  watch = 40;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);

  //set requiredValue value different than watch value
  requiredValue = 41;
  requiredValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);

  //set requiredValue value equals to watch value
  requiredValue = 40;
  requiredValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in WARNING state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testStateMonitor) {
  std::cout << "testStateMonitor" << std::endl;
  TestApplication<ctk::StateMonitor> app;

  app.monitor.connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  //app.dumpConnections();

  auto stateValue = test.getScalar<int>(std::string("/STATE_MONITOR.ON"));
  stateValue = 1;
  stateValue.write();
  test.stepApplication();

  auto watch = test.getScalar<double_t>(std::string("/WATCH"));
  watch = 1;
  watch.write();
  test.stepApplication();

  auto status = test.getScalar<uint16_t>(std::string("/STATUS"));
  status.readLatest();
  //should be in OK state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OK);
  
  watch = 0;
  watch.write();
  test.stepApplication();
  status.readLatest();
  //should be in ERROR state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::ERROR);
  
  stateValue = 0;
  stateValue.write();
  test.stepApplication();
  status.readLatest();
  //should be in OFF state.
  BOOST_CHECK_EQUAL(status,ChimeraTK::States::OFF);
}
