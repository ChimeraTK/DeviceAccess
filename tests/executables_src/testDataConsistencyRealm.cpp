// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "async/DataConsistencyRealmStore.h"
#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE DataConsistencyRealmTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

namespace ctk = ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(DataConsistencyRealmTestSuite)

/**********************************************************************************************************************/

static std::string cdd(
    R"((dummy:1?map=testDataConsistencyRealm.map&DataConsistencyKeys={"/theKey":"MyIdRealm", "/anotherKey":"MyIdRealm"}))");
static auto dummy =
    boost::dynamic_pointer_cast<ctk::DummyBackend>(ctk::BackendFactory::getInstance().createBackend(cdd));
static auto& realmStore = ctk::async::DataConsistencyRealmStore::getInstance();

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestRealm) {
  std::cout << "TestRealm" << std::endl;
  auto realm = realmStore.getRealm("SingleRealm");

  auto b = realm->getVersion(ctk::async::DataConsistencyKey(42));
  auto c = realm->getVersion(ctk::async::DataConsistencyKey(42));
  BOOST_TEST(b == c);

  auto d = realm->getVersion(ctk::async::DataConsistencyKey(43));
  BOOST_TEST(d > b);

  ctk::VersionNumber last{nullptr};
  for(uint64_t i = 1; i < 42; ++i) {
    auto a = realm->getVersion(ctk::async::DataConsistencyKey(i));
    BOOST_TEST(a != ctk::VersionNumber{nullptr});
    BOOST_TEST(a < b);
    if(last != ctk::VersionNumber{nullptr}) {
      BOOST_TEST(a > last);
    }
    last = a;
  }

  auto b2 = realm->getVersion(ctk::async::DataConsistencyKey(42));
  BOOST_TEST(b2 == b);

  auto x = realm->getVersion(ctk::async::DataConsistencyKey(1000000000));
  BOOST_TEST(x > d);

  auto y = realm->getVersion(ctk::async::DataConsistencyKey(42));
  BOOST_TEST(y == ctk::VersionNumber{nullptr});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestMultipleRealms) {
  std::cout << "TestMultipleRealms" << std::endl;
  auto realmA = realmStore.getRealm("RealmA");
  auto realmB = realmStore.getRealm("RealmB");

  // Two different realms shall not hand out the same version for the same key value. Also the realm store shall
  // gives out different realms for different names.

  auto a = realmA->getVersion(ctk::async::DataConsistencyKey(42));
  auto b = realmB->getVersion(ctk::async::DataConsistencyKey(42));
  BOOST_TEST(a != b);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestVersionConsistencyBetweenAccessors) {
  std::cout << "TestVersionConsistencyBetweenAccessors" << std::endl;
  ctk::Device dev(cdd);
  dev.open();

  auto realm = realmStore.getRealm("MyIdRealm");
  ctk::VersionNumber v0;

  auto key = dev.getScalarRegisterAccessor<uint32_t>("/theKey.DUMMY_WRITEABLE");
  auto dataA = dev.getScalarRegisterAccessor<int32_t>("/dataA", 0, {ctk::AccessMode::wait_for_new_data});
  auto dataB = dev.getScalarRegisterAccessor<int32_t>("/dataB", 0, {ctk::AccessMode::wait_for_new_data});
  auto interrupt6 = dev.getScalarRegisterAccessor<int32_t>("/interrupt6", 0, {ctk::AccessMode::wait_for_new_data});

  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!interrupt6.readNonBlocking());

  // check for initial value
  key.setAndWrite(12);
  dev.activateAsyncRead();

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(dataB.readNonBlocking());
  BOOST_REQUIRE(interrupt6.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!interrupt6.readNonBlocking());

  auto v1 = realm->getVersion(ctk::async::DataConsistencyKey(key));
  BOOST_REQUIRE(v1 > v0);
  BOOST_TEST(dataA.getVersionNumber() == v1);
  BOOST_TEST(dataB.getVersionNumber() == v1);
  BOOST_TEST(interrupt6.getVersionNumber() == v1);
  BOOST_TEST(dataA.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(dataB.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(interrupt6.dataValidity() == ctk::DataValidity::ok);

  // check for triggered interrupt
  key.setAndWrite(42);
  dummy->triggerInterrupt(6);

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(dataB.readNonBlocking());
  BOOST_REQUIRE(interrupt6.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!interrupt6.readNonBlocking());

  auto v2 = realm->getVersion(ctk::async::DataConsistencyKey(key));
  BOOST_REQUIRE(v2 > v1);
  BOOST_TEST(dataA.getVersionNumber() == v2);
  BOOST_TEST(dataB.getVersionNumber() == v2);
  BOOST_TEST(interrupt6.getVersionNumber() == v2);
  BOOST_TEST(dataA.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(dataB.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(interrupt6.dataValidity() == ctk::DataValidity::ok);

  // check for repeated key value
  dummy->triggerInterrupt(6);

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(dataB.readNonBlocking());
  BOOST_REQUIRE(interrupt6.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!interrupt6.readNonBlocking());

  BOOST_TEST(dataA.getVersionNumber() == v2);
  BOOST_TEST(dataB.getVersionNumber() == v2);
  BOOST_TEST(interrupt6.getVersionNumber() == v2);
  BOOST_TEST(dataA.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(dataB.dataValidity() == ctk::DataValidity::ok);
  BOOST_TEST(interrupt6.dataValidity() == ctk::DataValidity::ok);

  // check with backwards going key value
  key.setAndWrite(40);
  dummy->triggerInterrupt(6);

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(dataB.readNonBlocking());
  BOOST_REQUIRE(interrupt6.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!interrupt6.readNonBlocking());

  auto v3 = realm->getVersion(ctk::async::DataConsistencyKey(key));
  BOOST_REQUIRE(v3 < v2);
  BOOST_REQUIRE(v3 > v1);
  BOOST_TEST(dataA.getVersionNumber() == v2); // cannot go backwards with version numbers
  BOOST_TEST(dataB.getVersionNumber() == v2);
  BOOST_TEST(interrupt6.getVersionNumber() == v2);
  BOOST_TEST(dataA.dataValidity() == ctk::DataValidity::faulty);
  BOOST_TEST(dataB.dataValidity() == ctk::DataValidity::faulty);
  BOOST_TEST(interrupt6.dataValidity() == ctk::DataValidity::ok);

  dev.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestMultiInterrupt) {
  std::cout << "TestMultiInterrupt" << std::endl;
  ctk::Device dev(cdd);
  dev.open();

  auto realm = realmStore.getRealm("MyIdRealm");
  ctk::VersionNumber v0;

  auto keyA = dev.getScalarRegisterAccessor<uint32_t>("/theKey.DUMMY_WRITEABLE");
  auto dataA = dev.getScalarRegisterAccessor<int32_t>("/dataA", 0, {ctk::AccessMode::wait_for_new_data});
  auto intA = dev.getScalarRegisterAccessor<int32_t>("/interrupt6", 0, {ctk::AccessMode::wait_for_new_data});

  auto keyB = dev.getScalarRegisterAccessor<uint32_t>("/anotherKey.DUMMY_WRITEABLE");
  auto dataB = dev.getScalarRegisterAccessor<int32_t>("/dataC", 0, {ctk::AccessMode::wait_for_new_data});
  auto intB = dev.getScalarRegisterAccessor<int32_t>("/interrupt123", 0, {ctk::AccessMode::wait_for_new_data});

  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!intA.readNonBlocking());
  BOOST_REQUIRE(!intB.readNonBlocking());

  // check for initial value
  keyA.setAndWrite(12);
  keyB.setAndWrite(15);
  dev.activateAsyncRead();

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(dataB.readNonBlocking());
  BOOST_REQUIRE(intA.readNonBlocking());
  BOOST_REQUIRE(intB.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!intA.readNonBlocking());
  BOOST_REQUIRE(!intB.readNonBlocking());

  auto v1 = realm->getVersion(ctk::async::DataConsistencyKey(keyA));
  auto v2 = realm->getVersion(ctk::async::DataConsistencyKey(keyB));
  BOOST_REQUIRE(v1 > v0);
  BOOST_REQUIRE(v2 > v1);
  BOOST_TEST(dataA.getVersionNumber() == v1);
  BOOST_TEST(intA.getVersionNumber() == v1);
  BOOST_TEST(dataB.getVersionNumber() == v2);
  BOOST_TEST(intB.getVersionNumber() == v2);

  // check for triggered interrupt
  keyA.setAndWrite(15);
  dummy->triggerInterrupt(6);

  BOOST_REQUIRE(dataA.readNonBlocking());
  BOOST_REQUIRE(intA.readNonBlocking());
  BOOST_REQUIRE(!dataA.readNonBlocking());
  BOOST_REQUIRE(!dataB.readNonBlocking());
  BOOST_REQUIRE(!intA.readNonBlocking());
  BOOST_REQUIRE(!intB.readNonBlocking());

  BOOST_TEST(dataA.getVersionNumber() == v2);
  BOOST_TEST(intA.getVersionNumber() == v2);

  dev.close();
}

/**********************************************************************************************************************/

namespace {

  /// Run one double-buffer data-consistency-key scenario: open the backend described by the CDD, consume the initial
  /// value of the double-buffered data register, then finish buffer 1 via the given firmware driver and check that the
  /// delivered data and version match a fresh read of buffer 1.
  void runDoubleBufferScenario(const std::string& keyCdd, const std::string& realmName, const std::string& keyRegister,
      const std::vector<uint16_t>& dataValue) {
    auto backend =
        boost::dynamic_pointer_cast<ctk::DummyBackend>(ctk::BackendFactory::getInstance().createBackend(keyCdd));
    ctk::Device dev(keyCdd);
    dev.open();
    auto realm = realmStore.getRealm(realmName);

    auto data = dev.getOneDRegisterAccessor<uint16_t>("/TEST/DBLASYNC.1", 0, 0, {ctk::AccessMode::wait_for_new_data});

    dev.activateAsyncRead();
    BOOST_REQUIRE(data.readNonBlocking()); // consume the initial value

    ctk::DummyRegisterAccessor<uint32_t> keyBuf1{backend.get(), "", keyRegister};
    ctk::DummyRegisterAccessor<uint16_t> dataBuf1{backend.get(), "TEST/DBLASYNC.1", "BUF1"};
    ctk::DummyRegisterAccessor<uint32_t> inactiveBuf{backend.get(), "TEST/DOUBLE_BUF", "INACTIVE_BUF_ID"};

    // write a distinguishable value into buffer 1 of both key and data
    for(size_t e = 0; e < dataValue.size(); ++e) {
      dataBuf1[e] = dataValue[e];
    }
    // set the inactive-buffer id to 0 so buffer 1 is the freshly finished buffer
    inactiveBuf[0] = 0;

    const uint32_t keyValue = 42;
    keyBuf1[0] = keyValue;
    backend->triggerInterrupt(7);

    BOOST_REQUIRE(data.readNonBlocking());
    BOOST_TEST(data.getVersionNumber() == realm->getVersion(ctk::async::DataConsistencyKey(keyValue)));
    for(size_t e = 0; e < data.getNElements(); ++e) {
      BOOST_TEST(data[e] == dataValue[e]);
    }

    dev.close();
  }

} // namespace

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestDataConsistencyKeyDoubleBuffer) {
  // The plain-key and the correlated-key configurations both drive an interrupt-driven wait_for_new_data read of the
  // double-buffered data register TEST/DBLASYNC.1, which belongs to the TEST.DOUBLE_BUF control state. Each scenario
  // finishes buffer 1 once. In the correlated configuration the key register TEST.KEYDB shares the data register's
  // control state, so both accessors resolve to one control state and swap together.

  std::cout << "TestDataConsistencyKeyDoubleBuffer: running the plain key configuration" << std::endl;
  std::string plainKeyCdd =
      R"((ExceptionDummy:1?map=muxedDataAccessor.jmap&DataConsistencyKeys={"/TEST.KEY":"DoubleBufferKeyRealm"}))";
  runDoubleBufferScenario(plainKeyCdd, "DoubleBufferKeyRealm", "TEST.KEY", std::vector<uint16_t>{100, 101, 102, 103});

  std::cout << "TestDataConsistencyKeyDoubleBuffer: running the correlated key configuration" << std::endl;
  std::string correlatedKeyCdd =
      R"((ExceptionDummy:1?map=muxedDataAccessor.jmap&DataConsistencyKeys={"/TEST.KEYDB":"DoubleBufferKeyRealm"}))";
  runDoubleBufferScenario(
      correlatedKeyCdd, "DoubleBufferKeyRealm", "TEST.KEYDB.BUF1", std::vector<uint16_t>{200, 201, 202, 203});
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
