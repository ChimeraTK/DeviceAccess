/*
 * testHistory.C
 *
 *  Created on: Apr 5, 2018
 *      Author: zenker
 */

#define BOOST_TEST_MODULE HistoryTest

#include <fstream>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerHistory.h"

#include "TestFacility.h"

using namespace boost::unit_test_framework;
template<typename UserType>
struct Dummy: public ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPushInput<UserType> in {this, "in", "", "Dummy input"};
  ChimeraTK::ScalarOutput<UserType> out {this, "out", "", "Dummy output", {"history"}};

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
  ChimeraTK::ArrayOutput<UserType> out {this, "out", "", 3, "Dummy output", {"history"}};

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
  ChimeraTK::history::ServerHistory hist { this, "ServerHistory", "History of selected process variables." , 20};

  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addSource(dummy.findTag("history"), "history/" + dummy.getName());
    hist.findTag("CS").connectTo(cs);
    dummy.in >> cs("out");
    cs("in") >> dummy.in;
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
  ChimeraTK::history::ServerHistory hist { this, "ServerHistory", "History of selected process variables." ,20 };

  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    hist.addSource(dummy.findTag("history"), "history/" + dummy.getName());
    hist.findTag("CS").connectTo(cs);
    dummy.in >> cs("out");
    cs("in") >> dummy.in;
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
  auto v = tf.readArray<float>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i = 42.0;
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) = 42.0;
  v = tf.readArray<float>("history/Dummy/out");
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
  auto v = tf.readArray<double>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i = 42.0;
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) = 42.0;
  v = tf.readArray<double>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

}

BOOST_AUTO_TEST_CASE( testHistory_floatArray) {
  testAppArray<float> app;
  ChimeraTK::TestFacility tf;
  auto arr = tf.getArray<float>("in");
  tf.runApplication();
  arr[0] = 42.;
  arr[1] = 43.;
  arr[2] = 44.;
  arr.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[0], 42.0);
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[1], 43.0);
  BOOST_CHECK_EQUAL(tf.readArray<float>("out")[2], 44.0);
  std::vector<float> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = 42.0 + i;
    auto v = tf.readArray<float>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }

  arr[0] = 1.0;
  arr[1] = 2.0;
  arr[2] = 3.0;
  arr.write();
  tf.stepApplication();
  for(size_t i = 0; i < 3; i++){
    *(v_ref.end()-2) = 42.0 + i;
    *(v_ref.end()-1) = 1.0 + i;
    auto v = tf.readArray<float>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  }

}

BOOST_AUTO_TEST_CASE( testHistory_doubleArray) {
  testAppArray<double> app;
  ChimeraTK::TestFacility tf;
  auto arr = tf.getArray<double>("in");
  tf.runApplication();
  arr[0] = 42.;
  arr[1] = 43.;
  arr[2] = 44.;
  arr.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[0], 42.0);
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[1], 43.0);
  BOOST_CHECK_EQUAL(tf.readArray<double>("out")[2], 44.0);
  std::vector<double> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = 42.0 + i;
    auto v = tf.readArray<double>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }

  arr[0] = 1.0;
  arr[1] = 2.0;
  arr[2] = 3.0;
  arr.write();
  tf.stepApplication();
  for(size_t i = 0; i < 3; i++){
    *(v_ref.end()-2) = 42.0 + i;
    *(v_ref.end()-1) = 1.0 + i;
    auto v = tf.readArray<double>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }
}
