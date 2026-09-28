// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RebotConnectionCloseOnResetTest

#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "RebotDummyServer.h"

#include <chrono>
#include <thread>

using namespace ChimeraTK;

// Test fixture for setup and teardown.
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

  std::string uri(std::string ip) {
    return "(rebot?ip=" + ip + "&port=" + std::to_string(rebotServer.port()) + "&map=mtcadummy_rebot.map&timeout=1)";
  }
};

/********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testSecondConnectionReset, F) {
  // First device occupies the mock server's single session.
  Device d1(uri("localhost"));
  d1.open();
  BOOST_CHECK(d1.isFunctional());

  // A second device (a different URI on the same server port) connects while the
  // session is occupied. The server accepts and then closes the second
  // connection, which the client sees as a connection reset. Depending on timing
  // the error surfaces at open() or at the first IO, so assert both only raise
  // ChimeraTK::runtime_error, and that closing afterwards does not throw.
  Device d2(uri("127.0.0.1"));
  bool opened = false;
  try {
    d2.open();
    opened = true;
  }
  catch(const ChimeraTK::runtime_error&) {
    // acceptable: the reset surfaced at open()
  }

  if(opened) {
    // The first read must fail with ChimeraTK::runtime_error, never a
    // boost::system::system_error.
    BOOST_CHECK_THROW([[maybe_unused]] auto result = d2.read<int>("BOARD.WORD_USER"), ChimeraTK::runtime_error);
  }

  // Closing a device whose connection was reset must not throw.
  BOOST_CHECK_NO_THROW(d2.close());
  BOOST_CHECK_NO_THROW(d1.close());
}

/********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testCloseAfterServerReset, F) {
  Device d(uri("localhost"));
  d.open();
  BOOST_CHECK(d.isFunctional());

  // Reset the connection from the server side. The session socket is owned by
  // the server io thread, so resetConnection() posts the close onto the server
  // io loop.
  rebotServer.resetConnection();

  // The lost connection must surface at the next IO as ChimeraTK::runtime_error,
  // never a boost::system::system_error.
  BOOST_CHECK_THROW([[maybe_unused]] auto result = d.read<int>("BOARD.WORD_USER"), ChimeraTK::runtime_error);

  // Closing a device whose connection was reset mid-operation must not throw.
  BOOST_CHECK_NO_THROW(d.close());
}
