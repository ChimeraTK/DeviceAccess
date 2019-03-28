#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LMapMonostableTriggerPluginTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"
#include "BufferingRegisterAccessor.h"
#include "Device.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(LMapMonostableTriggerPluginTestSuite)

/********************************************************************************************************************/

class TestDummy : public ChimeraTK::DummyBackend {
 public:
  TestDummy(std::string mapFileName) : DummyBackend(mapFileName) {}

  size_t sequenceCounter{0};
  bool sequenceComplete{true};
  int32_t active{0};
  int32_t inactive{0};
  std::chrono::milliseconds delay;
  std::chrono::time_point<std::chrono::steady_clock> t0;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new TestDummy(parameters["map"]));
  }

  void write(uint8_t bar, uint32_t address, int32_t const* data, size_t sizeInBytes) override {
    if(bar == 0 && address == 0x44 && sizeInBytes == 4) { // ADC.WORD_ADC_ENA
      if(sequenceComplete) {
        // start new sequence
        sequenceComplete = false;
        active = data[0];
        t0 = std::chrono::steady_clock::now();
        return;
      }
      else {
        // finish the sequence
        delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0);
        sequenceComplete = true;
        ++sequenceCounter;
        inactive = data[0];
        return;
      }
    }
  }

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;
};

TestDummy::BackendRegisterer TestDummy::backendRegisterer;

TestDummy::BackendRegisterer::BackendRegisterer() {
  std::cout << "TestDummy::BackendRegisterer: registering backend type TestDummy" << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType("TestDummy", &TestDummy::createInstance);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDefaultValues) {
  std::cout << "testDefaultValues" << std::endl;
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=monostableTriggerPlugin.xlmap)");

  auto target = boost::dynamic_pointer_cast<TestDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(TestDummy?map=mtcadummy.map)"));
  assert(target != nullptr);
  auto accTrigger1 = device.getScalarRegisterAccessor<double>("testDefaultValues");

  usleep(10000);
  BOOST_CHECK_EQUAL(target->sequenceCounter, 0);
  BOOST_CHECK_EQUAL(target->sequenceComplete, true);

  for(size_t i = 1; i < 5; ++i) {
    accTrigger1 = 10 * i; // this should be ignored
    accTrigger1.write();
    BOOST_CHECK_EQUAL(target->sequenceCounter, i);
    BOOST_CHECK_EQUAL(target->sequenceComplete, true);
    BOOST_CHECK_EQUAL(target->active, 1);
    BOOST_CHECK_EQUAL(target->inactive, 0);
    BOOST_CHECK(target->delay.count() > 90);
    BOOST_CHECK(target->delay.count() < 200);
  }
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testNonDefaultValues) {
  std::cout << "testNonDefaultValues" << std::endl;
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=monostableTriggerPlugin.xlmap)");

  auto target = boost::dynamic_pointer_cast<TestDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(TestDummy?map=mtcadummy.map)"));
  assert(target != nullptr);
  auto accTrigger1 = device.getScalarRegisterAccessor<double>("testNonDefaultValues");

  usleep(10000);
  BOOST_CHECK_EQUAL(target->sequenceCounter, 0);
  BOOST_CHECK_EQUAL(target->sequenceComplete, true);

  for(size_t i = 1; i < 5; ++i) {
    accTrigger1 = 10 * i; // this should be ignored
    accTrigger1.write();
    BOOST_CHECK_EQUAL(target->sequenceCounter, i);
    BOOST_CHECK_EQUAL(target->sequenceComplete, true);
    BOOST_CHECK_EQUAL(target->active, 42);
    BOOST_CHECK_EQUAL(target->inactive, 120);
    BOOST_CHECK(target->delay.count() > 90);
    BOOST_CHECK(target->delay.count() < 200);
  }
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDifferentDelay) {
  std::cout << "testDifferentDelay" << std::endl;
  ChimeraTK::Device device;
  device.open("(logicalNameMap?map=monostableTriggerPlugin.xlmap)");

  auto target = boost::dynamic_pointer_cast<TestDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend("(TestDummy?map=mtcadummy.map)"));
  assert(target != nullptr);
  auto accTrigger1 = device.getScalarRegisterAccessor<double>("testDifferentDelay");

  usleep(10000);
  BOOST_CHECK_EQUAL(target->sequenceCounter, 0);
  BOOST_CHECK_EQUAL(target->sequenceComplete, true);

  for(size_t i = 1; i < 5; ++i) {
    accTrigger1 = 10 * i; // this should be ignored
    accTrigger1.write();
    BOOST_CHECK_EQUAL(target->sequenceCounter, i);
    BOOST_CHECK_EQUAL(target->sequenceComplete, true);
    BOOST_CHECK_EQUAL(target->active, 1);
    BOOST_CHECK_EQUAL(target->inactive, 0);
    BOOST_CHECK(target->delay.count() > 450);
    BOOST_CHECK(target->delay.count() < 600);
  }
}
/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
