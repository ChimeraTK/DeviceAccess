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
template<typename UserType>
struct Dummy: public ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPushInput<UserType> in {this, "in", "", "Dummy input"};
  ChimeraTK::ScalarOutput<UserType> out {this, "out", "", "Dummy output"};

  void mainLoop() override{
    while(true){
      in.read();
      out = 1.0 * in;
      out.write();
    }
  }
};

template<typename UserType>
struct DummyArray: public ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ArrayPushInput<UserType> in {this, "in", "", 3, "Dummy input"};
  ChimeraTK::ArrayOutput<UserType> out {this, "out", "", 3, "Dummy output"};

  void mainLoop() override{
    while(true){
      in.read();
      for(size_t i = 0; i < 3; i++)
        out[i] = 1.0 * in[i];
      out.write();
    }
  }
};

/**
 * Define a test app to test the scalar History Module.
 */
template<typename UserType>
struct testApp : public ChimeraTK::Application {
  testApp() : Application("test"){ }
  ~testApp() {
    shutdown();
  }

  Dummy<UserType> dummy{this, "Dummy", "Dummy module"};
  ChimeraTK::history::ScalarServerHistory<UserType> hist { this, "ServerHistory", "History of selected process variables." };

  logging::LoggingModule log { this, "LoggingModule",
      "LoggingModule test" };
  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addHistory(&dummy.out, dummy.getName(), "out", 20);
    hist.findTag("CS").connectTo(cs["history"]);
    dummy.in >> cs("out");
    cs("in") >> dummy.in;
    log.addSource(&hist.logger);
   }
};

/**
 * Define a test app to test the array History Module.
 */
template<typename UserType>
struct testAppArray : public ChimeraTK::Application {
  testAppArray() : Application("test"){ }
  ~testAppArray() {
    shutdown();
  }

  DummyArray<UserType> dummy{this, "Dummy", "Dummy module"};
  ChimeraTK::history::ArrayServerHistory<UserType> hist { this, "ServerHistory", "History of selected process variables." };

  logging::LoggingModule log { this, "LoggingModule",
      "LoggingModule array test" };
  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addHistory(&dummy.out, 3, dummy.getName(), "out", 20);
    hist.findTag("CS").connectTo(cs["history"]);
    dummy.in >> cs("out");
    cs("in") >> dummy.in;
    log.addSource(&hist.logger);
   }
};

/**
 * Define a test app to test the array History Module.
 */
template<typename UserType>
struct testServerHistoryApp : public ChimeraTK::Application {
  testServerHistoryApp() : Application("test"){ }
  ~testServerHistoryApp() {
    shutdown();
  }

  Dummy<UserType> dummy{this, "Dummy", "Dummy module"};
  ChimeraTK::history::ServerHistory hist { this, "ServerHistory", "History of selected process variables." };

  logging::LoggingModule log { this, "LoggingModule",
      "LoggingModule array test" };
  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addHistory<UserType>(&dummy.out, dummy.getName(), "out", 20);
    hist.v_float.findTag("CS").connectTo(cs["history"]);
    dummy.in >> cs("out");
    cs("in") >> dummy.in;
    log.addSource(&hist.v_float.logger);
   }
};

BOOST_AUTO_TEST_CASE( testHistory_float) {
  testApp<float> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<float>("in");
  tf.runApplication();
  i = 42.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readScalar<float>("out"), 42.0);
  std::vector<float> v_ref(20);
  v_ref.back() = 42.0;
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

BOOST_AUTO_TEST_CASE( testHistory_double) {
  testApp<double> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<double>("in");
  tf.runApplication();
  i = 42.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readScalar<double>("out"), 42.0);
  std::vector<double> v_ref(20);
  v_ref.back() = 42.0;
  auto v = tf.readArray<double>("history/Dummy/out_hist");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i = 42.0;
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) = 42.0;
  v = tf.readArray<double>("history/Dummy/out_hist");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

}

BOOST_AUTO_TEST_CASE( testHistory_floatArray) {
  testAppArray<float> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getArray<float>("in");
  tf.runApplication();
  i[0] = 42.;
  i[1] = 43.;
  i[2] = 44.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[0], 42.0);
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[1], 43.0);
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[2], 44.0);
  std::vector<float> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = 42.0 + i;
    auto v = tf.readArray<float>("history/Dummy/out_" + std::to_string(i) + "_hist");
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }

  i[0] = 1.0;
  i[1] = 2.0;
  i[2] = 3.0;
  i.write();
  tf.stepApplication();
  for(size_t i = 0; i < 3; i++){
    *(v_ref.end()-2) = 42.0 + i;
    *(v_ref.end()-1) = 1.0 + i;
    auto v = tf.readArray<float>("history/Dummy/out_" + std::to_string(i) + "_hist");
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  }

}

BOOST_AUTO_TEST_CASE( testHistory_doubleArray) {
  testAppArray<double> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getArray<double>("in");
  tf.runApplication();
  i[0] = 42.;
  i[1] = 43.;
  i[2] = 44.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[0], 42.0);
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[1], 43.0);
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[2], 44.0);
  std::vector<double> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = 42.0 + i;
    auto v = tf.readArray<double>("history/Dummy/out_" + std::to_string(i) + "_hist");
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }

  i[0] = 1.0;
  i[1] = 2.0;
  i[2] = 3.0;
  i.write();
  tf.stepApplication();
  for(size_t i = 0; i < 3; i++){
    *(v_ref.end()-2) = 42.0 + i;
    *(v_ref.end()-1) = 1.0 + i;
    auto v = tf.readArray<double>("history/Dummy/out_" + std::to_string(i) + "_hist");
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }
}

BOOST_AUTO_TEST_CASE( testServerHistory) {
  testServerHistoryApp<float> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<float>("in");
  tf.runApplication();
  i = 42.;
  i.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readScalar<float>("out"), 42.0);
  std::vector<float> v_ref(20);
  v_ref.back() = 42.0;
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
