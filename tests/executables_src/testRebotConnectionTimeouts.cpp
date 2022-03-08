#define BOOST_TEST_MODULE RebotConnectionTimeoutTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "RebotDummyServer.h"

#include <thread>
#include <chrono>
#include <condition_variable>

using namespace ChimeraTK;

// Test fixture for setup and teardown
struct F {
  F()
  : rebotServer{0 /*use random port*/, "./mtcadummy_rebot.map", 1 /*protocol version*/},
    serverThread([&]() { rebotServer.start(); }) {
    while(not rebotServer.is_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~F() {
    rebotServer.stop();
    serverThread.join();
  }
  RebotDummyServer rebotServer;
  boost::thread serverThread;
};

BOOST_FIXTURE_TEST_CASE(testOpenConnection, F) {
  rebotServer.stop();

  uint32_t timeout_sec = 1;
  auto accetable_completion_time = std::chrono::seconds(timeout_sec * 5);
  DeviceRenamedToFailDownstream d("sdm://./rebot=localhost," + std::to_string(rebotServer.port()) + ",mtcadummy_rebot.map," +
      std::to_string(timeout_sec));

  BOOST_CHECK(d.isFunctional() == false);

  auto begin = std::chrono::system_clock::now();
  { BOOST_CHECK_THROW(d.open(), ChimeraTK::runtime_error); }
  auto end = std::chrono::system_clock::now();

  BOOST_CHECK(d.isFunctional() == false);

  auto execution_duration = end - begin;
  BOOST_CHECK(execution_duration < accetable_completion_time);
}

BOOST_FIXTURE_TEST_CASE(testReadTimeout, F) {
  uint32_t timeout_sec = 1;
  auto accetable_completion_time = std::chrono::seconds(timeout_sec * 5);
  DeviceRenamedToFailDownstream d("sdm://./rebot=localhost," + std::to_string(rebotServer.port()) + ",mtcadummy_rebot.map," +
      std::to_string(timeout_sec));

  BOOST_CHECK(d.isFunctional() == false);

  d.open();

  BOOST_CHECK(d.isFunctional() == true);

  rebotServer.stop();
  auto begin = std::chrono::system_clock::now();
  { BOOST_CHECK_THROW(d.read<int>("BOARD.WORD_USER"), ChimeraTK::runtime_error); }
  auto end = std::chrono::system_clock::now();

  BOOST_CHECK(d.isFunctional() == false);

  auto execution_duration = end - begin;
  BOOST_CHECK(execution_duration < accetable_completion_time);
}

BOOST_FIXTURE_TEST_CASE(testWriteTimeout, F) {
  uint32_t timeout_sec = 1;
  auto accetable_completion_time = std::chrono::seconds(timeout_sec * 5);
  DeviceRenamedToFailDownstream d("sdm://./rebot=localhost," + std::to_string(rebotServer.port()) + ",mtcadummy_rebot.map," +
      std::to_string(timeout_sec));

  BOOST_CHECK(d.isFunctional() == false);

  d.open();

  BOOST_CHECK(d.isFunctional() == true);

  rebotServer.stop();
  auto begin = std::chrono::system_clock::now();
  { BOOST_CHECK_THROW(d.write("BOARD.WORD_USER", 42), ChimeraTK::runtime_error); }
  auto end = std::chrono::system_clock::now();

  BOOST_CHECK(d.isFunctional() == false);

  auto execution_duration = end - begin;
  BOOST_CHECK(execution_duration < accetable_completion_time);
}
