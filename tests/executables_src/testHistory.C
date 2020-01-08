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

// list of user types the accessors are tested with
typedef boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, float, double> test_types;

template<typename UserType>
struct Dummy: public ChimeraTK::ApplicationModule{
  using ChimeraTK::ApplicationModule::ApplicationModule;
  ChimeraTK::ScalarPushInput<UserType> in {this, "in", "", "Dummy input"};
  ChimeraTK::ScalarOutput<UserType> out {this, "out", "", "Dummy output", {"history"}};

  void mainLoop() override{
    while(true){
      in.read();
      out = (UserType)in;
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
        out[i] = (UserType)in[i];
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
    dummy.connectTo(cs);
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
    dummy.connectTo(cs);
   }
};

/**
 * Define a test app to test the device module in combination with the History Module.
 */
struct testAppDev : public ChimeraTK::Application {
  testAppDev() : Application("test"){
    ChimeraTK::BackendFactory::getInstance().setDMapFilePath("test.dmap");
  }
  ~testAppDev() {
    shutdown();
  }

  ChimeraTK::history::ServerHistory hist { this, "ServerHistory", "History of selected process variables." ,20 };

  ChimeraTK::DeviceModule dev{this, "Dummy1Mapped"};

  DummyArray<int> dummy{this, "Dummy", "Dummy module"};

  ChimeraTK::ControlSystemModule cs;

  void defineConnections() override{
    dummy.connectTo(cs);
    hist.addSource(dev.virtualiseFromCatalog(),"history",dummy.out);
    hist.findTag("CS").connectTo(cs);
   }
};

BOOST_AUTO_TEST_CASE_TEMPLATE( testScalarHistory, T, test_types) {
  testApp<T> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<T>("in");
  tf.runApplication();
  i = 42.;
  i.write();
  tf.stepApplication();
  std::vector<T> v_ref(20);
  v_ref.back() =  42.;
  auto v = tf.readArray<T>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i =  42.;
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) =   42.;
  v = tf.readArray<T>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

}

/* 'if constexpr' not working with gcc version < 7 so
 * add string case manually.
 */
BOOST_AUTO_TEST_CASE( testScalarHistoryString) {
  testApp<std::string> app;
  ChimeraTK::TestFacility tf;
  auto i = tf.getScalar<std::string>("in");
  tf.runApplication();
  i = "42";
  i.write();
  tf.stepApplication();
  std::vector<std::string> v_ref(20);
  v_ref.back() =  "42";
  auto v = tf.readArray<std::string>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  i =  "42";
  i.write();
  tf.stepApplication();
  *(v_ref.end()-2) =   "42";
  v = tf.readArray<std::string>("history/Dummy/out");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

}


BOOST_AUTO_TEST_CASE_TEMPLATE( testArrayHistory, T, test_types) {
  testAppArray<T> app;
  ChimeraTK::TestFacility tf;
  auto arr = tf.getArray<T>("in");
  tf.runApplication();
  arr[0] = 42.;
  arr[1] = 43.;
  arr[2] = 44.;
  arr.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<T>("out")[0], 42.0);
  BOOST_CHECK_EQUAL(tf.readArray<T>("out")[1], 43.0);
  BOOST_CHECK_EQUAL(tf.readArray<T>("out")[2], 44.0);
  std::vector<T> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = 42.0 + i;
    auto v = tf.readArray<T>("history/Dummy/out_" + std::to_string(i));
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
    auto v = tf.readArray<T>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  }

}

/* 'if constexpr' not working with gcc version < 7 so
 * add string case manually.
 */
BOOST_AUTO_TEST_CASE( testArrayHistoryString) {
  testAppArray<std::string> app;
  ChimeraTK::TestFacility tf;
  auto arr = tf.getArray<std::string>("in");
  tf.runApplication();
  arr[0] = "42";
  arr[1] = "43";
  arr[2] = "44";
  arr.write();
  tf.stepApplication();
  BOOST_CHECK_EQUAL(tf.readArray<std::string>("out")[0], "42");
  BOOST_CHECK_EQUAL(tf.readArray<std::string>("out")[1], "43");
  BOOST_CHECK_EQUAL(tf.readArray<std::string>("out")[2], "44");
  std::vector<std::string> v_ref(20);
  for(size_t i = 0; i < 3; i++){
    v_ref.back() = std::to_string(42 + i);
    auto v = tf.readArray<std::string>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
  }

  arr[0] = "1";
  arr[1] = "2";
  arr[2] = "3";
  arr.write();
  tf.stepApplication();
  for(size_t i = 0; i < 3; i++){
    *(v_ref.end()-2) = std::to_string(42 + i);
    *(v_ref.end()-1) = std::to_string(1 + i);
    auto v = tf.readArray<std::string>("history/Dummy/out_" + std::to_string(i));
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  }

}

BOOST_AUTO_TEST_CASE( testDeviceHistory ) {
  testAppDev app;
  ChimeraTK::TestFacility tf;

  // We use this device directly to change its values
  ChimeraTK::Device dev;
  // Use Dummy1 to change device values, since Dummy1Mapped is read only
  dev.open("Dummy1");
  dev.write("/FixedPoint/value",42);

  // Trigger the reading of the device
  auto i = tf.getScalar<int>("in");
  tf.runApplication();
  i = 1.;
  i.write();

  tf.stepApplication();

  // check new history buffer that ends with 42
  std::vector<double> v_ref(20);
  v_ref.back() =  42;
  auto v = tf.readArray<double>("history/Device/signed32");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  // Trigger the reading of the device
  i = 1.;
  i.write();

  tf.stepApplication();

  // check new history buffer that ends with 42,42
  *(v_ref.end()-2) =  42;
  v = tf.readArray<double>("history/Device/signed32");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());

  dev.write("/FixedPoint/value",43);

  // Trigger the reading of the device
  i = 1.;
  i.write();

  tf.stepApplication();

  // check new history buffer that ends with 42,42,43
  *(v_ref.end()-1) =  43;
  *(v_ref.end()-3) =  42;
  v = tf.readArray<double>("history/Device/signed32");
  BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(),
      v_ref.begin(), v_ref.end());
}

