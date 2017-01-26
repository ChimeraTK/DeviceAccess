/******************************************************************************
 * This file currently runs on real time, so it will take about a minute to finish!
 * FIXME: Port this to virtual time
 ******************************************************************************/

#define BOOST_TEST_MODULE RebotHeartbeatCountTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "RebotDummyServer.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace boost::unit_test_framework;
using namespace mtca4u;
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( RebotHeartbeatCountTestSuite )

BOOST_AUTO_TEST_CASE( testHeartbeat ){
  RebotDummyServer rebotServer(5001 /*port*/, "./mtcadummy_rebot.map", 1 /*protocol version*/);

  boost::thread serverThread(boost::bind(&RebotDummyServer::start, boost::ref(rebotServer) ));
    
  Device d;
  d.open("sdm://./rebot=localhost,5001,mtcadummy_rebot.map");

  for (uint32_t i=0; i<10;++i){
    d.write("BOARD.WORD_USER",42);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(2500));
  }

  BOOST_CHECK(rebotServer._heartbeatCount == 0 );

  for (uint32_t i=0; i <5; ++i){
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
    BOOST_CHECK(rebotServer._heartbeatCount == i );
  }
  
  for (uint32_t i=0; i<10;++i){
    d.read<int>("BOARD.WORD_USER");
    boost::this_thread::sleep_for(boost::chrono::milliseconds(2500));
  }

  BOOST_CHECK(rebotServer._heartbeatCount == 5 );

  for (uint32_t i=6; i <10; ++i){
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
    BOOST_CHECK(rebotServer._heartbeatCount == i );
  }
  
  //This is taking some time to run into a timeout.
  //So as long as we don't need multiple servers tries we
  //just let the server be killed on programme termination. Not clean, but faster in executsion.
  //rebotServer.stop(); 
  //serverThread.join();
}

BOOST_AUTO_TEST_SUITE_END()
