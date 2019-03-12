
/******************************************************************************
 * This file currently runs on real time, so it will take about a minute to
 *finish!
 * FIXME: Port this to virtual time
 ******************************************************************************/

#define BOOST_TEST_MODULE RebotHeartbeatCountTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "RebotDummyServer.h"
#include "testableRebotSleep_testingImpl.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(RebotHeartbeatCountTestSuite)

// This test is for protocol version 1
BOOST_AUTO_TEST_CASE(testHeartbeat1) {
  RebotDummyServer rebotServer(5001 /*port*/, "./mtcadummy_rebot.map", 1 /*protocol version*/);

  boost::thread serverThread(boost::bind(&RebotDummyServer::start, boost::ref(rebotServer)));

  Device d;
  d.open("sdm://./rebot=localhost,5001,mtcadummy_rebot.map");
  auto session = rebotServer.session();

  BOOST_CHECK_EQUAL(session->_helloCount, 1);

  // startup of the sleeping: get the lock, then tell the application it can try
  // to get the lock The app will continue once we free the lock
  RebotSleepSynchroniser::_lock.lock();
  RebotSleepSynchroniser::_clientMayGetLock = true;
  std::cout << "test locked manually , mayget is true" << std::endl;

  for(uint32_t i = 1; i < 5; ++i) {
    d.write("BOARD.WORD_USER", 42);
    testable_rebot_sleep::advance_until(boost::chrono::milliseconds(i * 2500));
  }

  BOOST_CHECK_EQUAL(session->_helloCount, 1);

  for(uint32_t i = 1; i < 5; ++i) {
    testable_rebot_sleep::advance_until(boost::chrono::milliseconds(i * 5000 + 10000));
    BOOST_CHECK_EQUAL(session->_helloCount, i + 1);
  }

  for(uint32_t i = 1; i < 5; ++i) {
    d.read<int>("BOARD.WORD_USER");
    testable_rebot_sleep::advance_until(boost::chrono::milliseconds(i * 2500 + 30000));
  }

  BOOST_CHECK_EQUAL(session->_helloCount, 5);

  for(uint32_t i = 1; i < 5; ++i) {
    testable_rebot_sleep::advance_until(boost::chrono::milliseconds(i * 5000 + 40000));
    BOOST_CHECK_EQUAL(session->_helloCount, i + 5);
  }

  // ****************
  // FIXME
  // This test freezes when advancing because the readout is the backend is not
  // return, so it has been turned off.
  // ****************

  // // Test error handling heartbeat loop.
  //
  // // Tell the server not to answer and advance the time so the another
  // heartbeat is send.
  // // This intentionally does not throw, because it's in anther thread. But it
  // closes the backend. BOOST_CHECK(d.isOpened() == true);
  //
  // //  std::cout << "setting rebot dummy server not to answer " << std::endl;
  // rebotServer._dont_answer=true;
  // std::cout << "advancing to 65000" << std::endl;
  // testable_rebot_sleep::advance_until(boost::chrono::milliseconds(65000));
  // std::cout << "If we are here everything is fine " << std::endl;
  //
  session.reset();

  BOOST_CHECK(d.isOpened() == true);
  d.close();
  BOOST_CHECK(d.isOpened() == false);

  std::cout << "test done" << std::endl;
  RebotSleepSynchroniser::_lock.unlock();

  // This is taking some time to run into a timeout, sometimes never finishes.
  // So we take it out at the moment.
  // Note: At this point the backend must have been closed, so the client
  // connection of the server is no longer open. Otherwise we will block here
  // forever.
  assert(d.isOpened() == false);
  rebotServer.stop();
  serverThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
