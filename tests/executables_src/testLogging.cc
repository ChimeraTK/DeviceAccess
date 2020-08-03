/*
 * testLogging.cc
 *
 *  Created on: Apr 11, 2018
 *      Author: zenker
 */

//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LoggingTest

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread.hpp>

#include "Logging.h"
using namespace logging;

#include "TestFacility.h"

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

/**
 * Define a test app to test the SoftwareMasterModule.
 */
struct testApp : public ChimeraTK::Application {
  testApp() : Application("test"), fileCreated(false) {
    // remove possible left overs
    boost::filesystem::remove("/tmp/testLogging/test.log");
    boost::filesystem::remove("/tmp/testLogging/");
  }
  ~testApp() override {
    shutdown();
    if(fileCreated) {
      BOOST_CHECK_EQUAL(boost::filesystem::remove("/tmp/testLogging/test.log"), true);
      BOOST_CHECK_EQUAL(boost::filesystem::remove("/tmp/testLogging/"), true);
    }
  }

  LoggingModule log{this, "LoggingModule", "LoggingModule test"};

  boost::shared_ptr<Logger> logger{new Logger(&log)};

  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override {
    log.addSource(logger);
    log.findTag("CS").connectTo(cs);
  }

  bool fileCreated;
};

BOOST_AUTO_TEST_CASE(testLogMsg) {
  testApp app;
  ChimeraTK::TestFacility tf;
  tf.runApplication();
  auto tailLength = tf.getScalar<uint>("maxTailLength");
  tailLength = 1;
  tailLength.write();
  app.logger->sendMessage("test", LogLevel::DEBUG);
  tf.stepApplication();
  std::string ss = tf.readScalar<std::string>("logTail");
  BOOST_CHECK_EQUAL(ss.substr(ss.find("->") + 3), std::string("test\n"));
}

BOOST_AUTO_TEST_CASE(testLogfileFails) {
  testApp app;
  ChimeraTK::TestFacility tf;

  auto logFile = tf.getScalar<std::string>("logFile");
  tf.runApplication();
  logFile = std::string("/tmp/testLogging/test.log");
  logFile.write();
  // message not considered here but used to step through the application
  app.logger->sendMessage("test", LogLevel::DEBUG);
  tf.stepApplication();
  std::string ss = (std::string)tf.readScalar<std::string>("logTail");
  std::vector<std::string> strs;
  boost::split(strs, ss, boost::is_any_of("\n"), boost::token_compress_on);
  BOOST_CHECK_EQUAL(strs.at(2).substr(strs.at(2).find("->") + 3),
      std::string("Failed to open log file for writing: /tmp/testLogging/test.log"));
}

BOOST_AUTO_TEST_CASE(testLogfile) {
  testApp app;
  ChimeraTK::TestFacility tf;

  auto logFile = tf.getScalar<std::string>("logFile");

  if(!boost::filesystem::is_directory("/tmp/testLogging/")) boost::filesystem::create_directory("/tmp/testLogging/");
  tf.runApplication();
  logFile = std::string("/tmp/testLogging/test.log");
  logFile.write();
  // message not considered here but used to step through the application
  app.logger->sendMessage("test", LogLevel::DEBUG);
  tf.stepApplication();
  std::fstream file;
  file.open("/tmp/testLogging/test.log");
  BOOST_CHECK_EQUAL(file.good(), true);
  if(file.good()) app.fileCreated = true;
  std::string line;

  std::getline(file, line);
  BOOST_CHECK_EQUAL(
      line.substr(line.find("->") + 3), std::string("Opened log file for writing: /tmp/testLogging/test.log"));
  std::getline(file, line);
  BOOST_CHECK_EQUAL(line.substr(line.find("->") + 3), std::string("test"));
}

BOOST_AUTO_TEST_CASE(testLogging) {
  testApp app;
  ChimeraTK::TestFacility tf;

  auto logLevel = tf.getScalar<uint>("logLevel");
  auto tailLength = tf.getScalar<uint>("maxTailLength");

  tf.runApplication();
  logLevel = 0;
  logLevel.write();
  tailLength = 2;
  tailLength.write();
  app.logger->sendMessage("1st test message", LogLevel::DEBUG);
  tf.stepApplication();
  app.logger->sendMessage("2nd test message", LogLevel::DEBUG);
  tf.stepApplication();
  auto tail = tf.readScalar<std::string>("logTail");
  std::vector<std::string> result;
  boost::algorithm::split(result, tail, boost::is_any_of("\n"));
  // result length should be 3 not 2, because new line is used to split, which
  // results in 3 items although there are only two messages.
  BOOST_CHECK_EQUAL(result.size(), 3);

  /**** Test log level ****/
  logLevel = 2;
  logLevel.write();
  app.logger->sendMessage("3rd test message", LogLevel::DEBUG);
  tf.stepApplication();
  tail = tf.readScalar<std::string>("logTail");
  boost::algorithm::split(result, tail, boost::is_any_of("\n"));
  // should still be 3 because log level was too low!
  BOOST_CHECK_EQUAL(result.size(), 3);

  /**** Test tail length ****/
  tailLength = 3;
  tailLength.write();
  //  tf.stepApplication();
  app.logger->sendMessage("4th test message", LogLevel::ERROR);
  tf.stepApplication();
  tail = tf.readScalar<std::string>("logTail");
  boost::algorithm::split(result, tail, boost::is_any_of("\n"));
  BOOST_CHECK_EQUAL(result.size(), 4);

  app.logger->sendMessage("5th test message", LogLevel::ERROR);
  tf.stepApplication();
  tail = tf.readScalar<std::string>("logTail");
  boost::algorithm::split(result, tail, boost::is_any_of("\n"));
  // should still be 4 because tailLength is 3!
  BOOST_CHECK_EQUAL(result.size(), 4);
}
