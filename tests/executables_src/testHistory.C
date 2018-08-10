/*
 * testHistory.C
 *
 *  Created on: Apr 5, 2018
 *      Author: zenker
 */

#define BOOST_TEST_MODULE HistoryTest

#include <fstream>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/thread.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerHistory.h"

#include "TestFacility.h"

using namespace boost::unit_test_framework;

struct Dummy: public ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPushInput<float> dummy_in {this, "dummy_in", "", "Dummy input"};
  ChimeraTK::ScalarOutput<float> dummy_out {this, "dummy_out", "", "Dummy output"};

  void mainLoop() override{
    while(true){
      dummy_in.read();
      dummy_out = 1.0 * dummy_in;
      dummy_out.write();
    }
  }
};


/**
 * Define a test app to test the SoftwareMasterModule.
 */
struct testApp : public ChimeraTK::Application {
  testApp() : Application("test"){ }
  ~testApp() {
    shutdown();
  }

  Dummy dummy{this, "Dummy", "Dummy module"};
  history::ServerHistory hist { this, "ServerHistory", "History of selected process variables." };

  logging::LoggingModule log { this, "LoggingModule",
      "LoggingModule test" };
  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addHistory(&dummy.dummy_out, dummy.getName(), "out", 20);
    hist.findTag("CS").connectTo(cs["history"]);
    dummy.dummy_in >> cs("out");
    cs("in") >> dummy.dummy_in;
    log.addSource(&hist.logger);
   }
};

BOOST_AUTO_TEST_CASE( testHistory) {
  testApp app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<float>("in");
  tf.runApplication();
  i = 42.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readScalar<float>("out"), 42.0);
  std::vector<float> v_ref(20);
  v_ref.back() = 42.0;
  BOOST_CHECK_EQUAL(tf.readArray<float>("history/Dummy/out_hist").back(), v_ref.back());
  auto v = tf.readArray<float>("history/Dummy/out_hist");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i = 42.0;
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) = 42.0;
  v = tf.readArray<float>("history/Dummy/out_hist");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

}
