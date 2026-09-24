// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AsyncReadTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DeviceAccessVersion.h"
#include "DeviceBackendImpl.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ReadAnyGroup.h"

#include <boost/thread.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

std::string cdd = "(AsyncTestDummy)";

/**********************************************************************************************************************/

class AsyncTestDummy : public DeviceBackendImpl { public:
  explicit AsyncTestDummy() { FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl); }

  std::string readDeviceInfo() override { return "AsyncTestDummy"; }

  RegisterCatalogue getRegisterCatalogue() const override { throw; }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string>) {
    return boost::shared_ptr<DeviceBackend>(new AsyncTestDummy());
  }

  template<typename UserType>
  class Accessor : public NDRegisterAccessor<UserType> {
   public:
    Accessor(AsyncTestDummy* backend, const RegisterPath& registerPathName, AccessModeFlags& flags)
    : NDRegisterAccessor<UserType>(registerPathName, flags), _backend(backend) {
      buffer_2D.resize(1);
      buffer_2D[0].resize(1);
      this->_readQueue = {3}; // this accessor is using a queue length of 3
      _backend->notificationQueue[getName()] = this->_readQueue;
    }

    void doReadTransferSynchronously() override {}

    bool doWriteTransfer(ChimeraTK::VersionNumber) override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {}

    void doPostWrite(TransferType, VersionNumber) override {}

    void doPreRead(TransferType) override {}

    void doPostRead(TransferType, bool hasNewData) override {
      if constexpr(!std::is_same<UserType, Void>::value) { // Will not be used for Void-Type
        ++nPostReadCalled;
        if(!hasNewData) return;
        buffer_2D[0][0] = _backend->registers.at(getName());
        this->_versionNumber = {};
      }
    }

    [[nodiscard]] bool isReadOnly() const override { return false; }
    [[nodiscard]] bool isReadable() const override { return true; }
    [[nodiscard]] bool isWriteable() const override { return true; }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {this->shared_from_this()};
    }
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    size_t nPostReadCalled{0};

   protected:
    AsyncTestDummy* _backend;
    using NDRegisterAccessor<UserType>::getName;
    using NDRegisterAccessor<UserType>::buffer_2D;
  };

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    assert(numberOfWords == 1);
    assert(wordOffsetInRegister == 0);
    (void)numberOfWords;
    (void)wordOffsetInRegister;
    boost::shared_ptr<NDRegisterAccessor<UserType>> retval =
        boost::make_shared<Accessor<UserType>>(this, registerPathName, flags);
    retval->setExceptionBackend(shared_from_this());
    return retval;
  }

  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(AsyncTestDummy, getRegisterAccessor_impl, 4);

  void open() override {
    _opened = true;
    _hasActiveException = false;
  }

  void close() override { _opened = false; }

  std::map<std::string, cppext::future_queue<void>> notificationQueue;
  std::map<std::string, size_t> registers;

  void setExceptionImpl() noexcept override {
    // FIXME !!!!
    assert(false); // Wrong implementation. All notification queues must see an exception.
  }
  bool _hasActiveException{false};
};

/**********************************************************************************************************************/

struct Fixture {
  Fixture() {
    BackendFactory::getInstance().registerBackendType(
        "AsyncTestDummy", &AsyncTestDummy::createInstance, {}, CHIMERATK_DEVICEACCESS_VERSION);
    BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  }
};
static Fixture fixture;

/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAsyncRead) {
  std::cout << "testAsyncRead" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto accessor = device.getScalarRegisterAccessor<int>("REG", 0, {AccessMode::wait_for_new_data});

  // simple reading through readAsync without actual need
  backend->registers["/REG"] = 5;

  auto waitForRead = std::async(std::launch::async, [&accessor] { accessor.read(); });
  auto waitStatus = waitForRead.wait_for(std::chrono::seconds(1));
  BOOST_CHECK(waitStatus != std::future_status::ready); // future not ready yet, i.e. read() not fninished.

  backend->notificationQueue["/REG"].push(); // trigger transfer
  waitForRead.wait();                        // wait for the read to finish

  BOOST_CHECK(accessor == 5);
  BOOST_CHECK(backend->notificationQueue["/REG"].empty());

  backend->registers["/REG"] = 6;
  waitForRead = std::async(std::launch::async, [&accessor] { accessor.read(); });
  waitStatus = waitForRead.wait_for(std::chrono::seconds(1));
  BOOST_CHECK(waitStatus != std::future_status::ready); // future not ready yet, i.e. read() not fninished.

  backend->notificationQueue["/REG"].push(); // trigger transfer
  waitForRead.wait();

  BOOST_CHECK(accessor == 6);
  BOOST_CHECK(backend->notificationQueue["/REG"].empty());

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAny) {
  std::cout << "testReadAny" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
  auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
  auto a3 = device.getScalarRegisterAccessor<int32_t>("a3", 0, {AccessMode::wait_for_new_data});
  auto a4 = device.getScalarRegisterAccessor<int32_t>("a4", 0, {AccessMode::wait_for_new_data});

  // initialise the buffers of the accessors
  a1 = 1;
  a2 = 2;
  a3 = 3;
  a4 = 4;

  // initialise the dummy registers
  backend->registers["/a1"] = 42;
  backend->registers["/a2"] = 123;
  backend->registers["/a3"] = 120;
  backend->registers["/a4"] = 345;

  // Create ReadAnyGroup
  ReadAnyGroup group;
  BOOST_TEST(a1.getReadAnyGroup() == nullptr);
  group.add(a1);
  BOOST_TEST(a1.getReadAnyGroup() == &group);
  group.add(a2);
  group.add(a3);
  group.add(a4);
  group.finalise();

  TransferElementID id;

  // register 1
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a1"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 3);
    BOOST_CHECK(a4 == 4);
    BOOST_CHECK(id == a1.getId());
  }

  // register 3
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a3"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 120);
    BOOST_CHECK(a4 == 4);
    BOOST_CHECK(id == a3.getId());
  }

  // register 3 again
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->registers["/a3"] = 121;
    backend->notificationQueue["/a3"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 4);
    BOOST_CHECK(id == a3.getId());
  }

  // register 2
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a2"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 4);
    BOOST_CHECK(id == a2.getId());
  }

  // register 4
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a4"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 345);
    BOOST_CHECK(id == a4.getId());
  }

  // register 4 again
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a4"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 345);
    BOOST_CHECK(id == a4.getId());
  }

  // register 3 a 3rd time
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->registers["/a3"] = 122;
    backend->notificationQueue["/a3"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 122);
    BOOST_CHECK(a4 == 345);
    BOOST_CHECK(id == a3.getId());
  }

  // register 1 and then register 2 (order should be guaranteed)
  {
    // write to register 1 and trigger transfer
    backend->registers["/a1"] = 55;
    backend->notificationQueue["/a1"].push(); // trigger transfer

    // same with register 2
    backend->registers["/a2"] = 66;
    backend->notificationQueue["/a2"].push(); // trigger transfer

    BOOST_CHECK_EQUAL((int)a1, 42);
    BOOST_CHECK_EQUAL((int)a2, 123);

    // no point to use a thread here
    auto r = group.readAny();
    BOOST_CHECK(a1.getId() == r);
    BOOST_CHECK_EQUAL((int)a1, 55);
    BOOST_CHECK_EQUAL((int)a2, 123);

    r = group.readAny();
    BOOST_CHECK(a2.getId() == r);
    BOOST_CHECK(a1 == 55);
    BOOST_CHECK(a2 == 66);
  }

  // registers in order: 4, 2, 3 and 1
  {
    // register 4 (see above for explanation)
    backend->registers["/a4"] = 11;
    backend->notificationQueue["/a4"].push(); // trigger transfer

    // register 2
    backend->registers["/a2"] = 22;
    backend->notificationQueue["/a2"].push(); // trigger transfer

    // register 3
    backend->registers["/a3"] = 33;
    backend->notificationQueue["/a3"].push(); // trigger transfer

    // register 1
    backend->registers["/a1"] = 44;
    backend->notificationQueue["/a1"].push(); // trigger transfer

    // no point to use a thread here
    auto r = group.readAny();
    BOOST_CHECK(a4.getId() == r);
    BOOST_CHECK(a1 == 55);
    BOOST_CHECK(a2 == 66);
    BOOST_CHECK(a3 == 122);
    BOOST_CHECK(a4 == 11);

    r = group.readAny();
    BOOST_CHECK(a2.getId() == r);
    BOOST_CHECK(a1 == 55);
    BOOST_CHECK(a2 == 22);
    BOOST_CHECK(a3 == 122);
    BOOST_CHECK(a4 == 11);

    r = group.readAny();
    BOOST_CHECK(a3.getId() == r);
    BOOST_CHECK(a1 == 55);
    BOOST_CHECK(a2 == 22);
    BOOST_CHECK(a3 == 33);
    BOOST_CHECK(a4 == 11);

    r = group.readAny();
    BOOST_CHECK(a1.getId() == r);
    BOOST_CHECK(a1 == 44);
    BOOST_CHECK(a2 == 22);
    BOOST_CHECK(a3 == 33);
    BOOST_CHECK(a4 == 11);
  }

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAnyWithPoll) {
  std::cout << "testReadAnyWithPoll" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
  auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
  auto a3 = device.getScalarRegisterAccessor<int32_t>("a3");
  auto a4 = device.getScalarRegisterAccessor<int32_t>("a4");

  // initialise the buffers of the accessors
  a1 = 1;
  a2 = 2;
  a3 = 3;
  a4 = 4;

  // initialise the dummy registers
  backend->registers["/a1"] = 42;
  backend->registers["/a2"] = 123;
  backend->registers["/a3"] = 120;
  backend->registers["/a4"] = 345;

  // Create ReadAnyGroup
  ReadAnyGroup group;
  group.add(a1);
  group.add(a2);
  group.add(a3);
  group.add(a4);
  group.finalise();

  TransferElementID id;

  // register 1
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a1"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 120);
    BOOST_CHECK(a4 == 345);
    BOOST_CHECK(id == a1.getId());
  }

  backend->registers["/a3"] = 121;
  backend->registers["/a4"] = 346;

  // register 2
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &id] {
      id = group.readAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a2"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 346);
    BOOST_CHECK(id == a2.getId());
  }

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWaitAny) {
  std::cout << "testWaitAny" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
  auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
  auto a3 = device.getScalarRegisterAccessor<int32_t>("a3");
  auto a4 = device.getScalarRegisterAccessor<int32_t>("a4");

  // initialise the buffers of the accessors
  a1 = 1;
  a2 = 2;
  a3 = 3;
  a4 = 4;

  // initialise the dummy registers
  backend->registers["/a1"] = 42;
  backend->registers["/a2"] = 123;
  backend->registers["/a3"] = 120;
  backend->registers["/a4"] = 345;

  // Create ReadAnyGroup
  ReadAnyGroup group;
  group.add(a1);
  group.add(a2);
  group.add(a3);
  group.add(a4);
  group.finalise();

  ReadAnyGroup::Notification notification;

  // register 1
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &notification] {
      notification = group.waitAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a1"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(notification.getId() == a1.getId());
    BOOST_CHECK(a1 == 1);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 3);
    BOOST_CHECK(a4 == 4);
    BOOST_CHECK(notification.accept());
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 3);
    BOOST_CHECK(a4 == 4);
    group.processPolled();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 120);
    BOOST_CHECK(a4 == 345);
  }

  backend->registers["/a3"] = 121;
  backend->registers["/a4"] = 346;

  // register 2
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group, &flag, &notification] {
      notification = group.waitAny();
      flag = true;
    });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    backend->notificationQueue["/a2"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(notification.getId() == a2.getId());
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 2);
    BOOST_CHECK(a3 == 120);
    BOOST_CHECK(a4 == 345);
    BOOST_CHECK(notification.accept());
    group.processPolled();
    BOOST_CHECK(a1 == 42);
    BOOST_CHECK(a2 == 123);
    BOOST_CHECK(a3 == 121);
    BOOST_CHECK(a4 == 346);
  }

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAnyException) {
  std::cout << "testReadAnyException" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
  auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
  auto a3 = device.getScalarRegisterAccessor<int32_t>("a3", 0, {AccessMode::wait_for_new_data});
  auto a4 = device.getScalarRegisterAccessor<int32_t>("a4", 0, {AccessMode::wait_for_new_data});
  auto a1_casted = boost::dynamic_pointer_cast<AsyncTestDummy::Accessor<uint8_t>>(a1.getHighLevelImplElement());
  assert(a1_casted);

  // initialise the buffers of the accessors
  a1 = 1;
  a2 = 2;
  a3 = 3;
  a4 = 4;

  // initialise the dummy registers
  backend->registers["/a1"] = 42;
  backend->registers["/a2"] = 123;
  backend->registers["/a3"] = 120;
  backend->registers["/a4"] = 345;

  // Create ReadAnyGroup
  ReadAnyGroup group;
  group.add(a1);
  group.add(a2);
  group.add(a3);
  group.add(a4);
  group.finalise();

  // ChimeraTK::runtime_error
  {
    auto nPostReadCalledReference = a1_casted->nPostReadCalled;
    auto versionNumReference = a1.getVersionNumber();

    // launch the readAny in a background thread
    bool exceptionFound{false};
    std::thread thread([&group, &exceptionFound] {
      try {
        group.readAny();
      }
      catch(ChimeraTK::runtime_error&) {
        exceptionFound = true;
      }
    });

    // put exception to queue
    try {
      throw ChimeraTK::runtime_error("Test exception");
    }
    catch(...) {
      backend->notificationQueue["/a1"].push_exception(std::current_exception()); // trigger transfer
    }
    thread.join();
    BOOST_TEST(exceptionFound == true);
    BOOST_TEST(a1_casted->nPostReadCalled == nPostReadCalledReference + 1);
    BOOST_TEST(a1.getVersionNumber() == versionNumReference);
  }

  // boost::thread_interrupted
  {
    auto nPostReadCalledReference = a1_casted->nPostReadCalled;
    auto versionNumReference = a1.getVersionNumber();

    // launch the readAny in a background thread
    bool exceptionFound{false};
    std::thread thread([&group, &exceptionFound] {
      try {
        group.readAny();
      }
      catch(boost::thread_interrupted&) {
        exceptionFound = true;
      }
    });

    // put exception to queue
    backend->notificationQueue["/a1"].push_exception(
        std::make_exception_ptr(boost::thread_interrupted())); // trigger transfer

    thread.join();
    BOOST_TEST(exceptionFound == true);
    BOOST_TEST(a1_casted->nPostReadCalled == nPostReadCalledReference + 1);
    BOOST_TEST(a1.getVersionNumber() == versionNumReference);
  }

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAnyInvalid) {
  std::cout << "testReadAnyInvalid" << std::endl;

  Device device;
  device.open(cdd);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(cdd));
  BOOST_CHECK(backend != nullptr);

  // obtain register accessor with integral type
  auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});

  // Create ReadAnyGroup
  ReadAnyGroup group{a1};

  {
    // direct read on accessor in ReadAnyGroup is not allowed and should issue exception.
    BOOST_CHECK_THROW(a1.read(), ChimeraTK::logic_error);
  }

  device.close();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
// Runtime 'selectedBy' gating on the interrupt path (plan tests I1-I5).
//
// The fixture selectedByInterrupt.jmap defines an interrupt-triggered, double-buffered data register
// DAQ.DATA that is only active while DAQ.MUX_SEL == 1, plus a mutually exclusive alternative DAQ.DATA_ALT
// (active while DAQ.MUX_SEL == 2) sharing the same buffer base and handshake. Both are serviced by interrupt
// domain 1. A consumer subscribing only to DAQ.DATA must not wake while MUX_SEL != 1, and must never observe
// data tagged with the wrong selection.

// Writes the selector register through a DummyRegisterAccessor so it can be changed between subscriptions.
class SelectedByInterruptFixture {
 public:
  SelectedByInterruptFixture()
  : dummy(openDeviceAndGetDummy(device)),
    accessor(
        device.getOneDRegisterAccessor<uint32_t>("/DAQ/DATA", 1, 0, {AccessMode::wait_for_new_data})),
    muxSel(dummy.get(), "DAQ", "MUX_SEL"), enable(dummy.get(), "DAQ/DOUBLE_BUF", "ENA"),
    inactive(dummy.get(), "DAQ/DOUBLE_BUF", "INACTIVE_BUF_ID"), buffer0(dummy.get(), "DAQ/DATA", "BUF0"),
    buffer1(dummy.get(), "DAQ/DATA", "BUF1") {
    // Enable double buffering for handshake index 0.
    enable[0] = 1;
  }

  // Simulate the firmware finishing a buffer: fill the freshly finished buffer and point INACTIVE_BUF_ID at it,
  // then raise the interrupt on domain 1. This is only meaningful while DAQ.DATA is selected (MUX_SEL == 1).
  void firmwareFinishesBuffer(uint32_t value, uint32_t newInactiveBuffer) {
    if(newInactiveBuffer == 1) {
      buffer0 = value;
    }
    else {
      buffer1 = value;
    }
    inactive[0] = newInactiveBuffer;
    dummy->triggerInterrupt(1);
  }

  Device device;
  boost::shared_ptr<DummyBackend> dummy;
  OneDRegisterAccessor<uint32_t> accessor;
  DummyRegisterAccessor<uint32_t> muxSel;
  DummyRegisterAccessor<uint32_t> enable;
  DummyRegisterAccessor<uint32_t> inactive;
  DummyRegisterAccessor<uint32_t> buffer0;
  DummyRegisterAccessor<uint32_t> buffer1;

 private:
  static boost::shared_ptr<DummyBackend> openDeviceAndGetDummy(Device& dev) {
    dev.open("(dummy?map=selectedByInterrupt.jmap)");
    auto backend = boost::dynamic_pointer_cast<DummyBackend>(dev.getBackend());
    if(!backend) {
      BOOST_FAIL("Device did not produce a DummyBackend");
    }
    return backend;
  }
};

/**********************************************************************************************************************/

// I1: with MUX_SEL == 1, each interrupt returns the freshly filled buffer over several consecutive swaps.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptSelectedDelivers) {
  SelectedByInterruptFixture f;
  f.muxSel[0] = 1; // select DAQ.DATA
  f.device.activateAsyncRead();

  // An initial value is delivered when the async domain is activated.
  BOOST_REQUIRE(f.accessor.readNonBlocking());

  // No further data before the firmware completes the next buffer.
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 1: buffer0 (=100) finished.
  f.firmwareFinishesBuffer(100, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 100);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 2: buffer1 (=200) finished.
  f.firmwareFinishesBuffer(200, 0);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 200);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  f.device.close();
}

/**********************************************************************************************************************/

// I2: while the selector points to the OTHER alternative (MUX_SEL == 2), the DAQ.DATA consumer does not wake:
// the interrupt is handled but the data is delivered with an unchanged version number, so readNonBlocking()
// stays false.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptUnselectedDoesNotWake) {
  SelectedByInterruptFixture f;
  f.muxSel[0] = 2; // select DATA_ALT, so DAQ.DATA is inactive
  f.device.activateAsyncRead();

  // The initial value is delivered (marked faulty since the selection is not met).
  BOOST_REQUIRE(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() == ChimeraTK::DataValidity::faulty);

  // The firmware finishes several buffers while DAQ.DATA is unselected. The consumer must not wake.
  f.firmwareFinishesBuffer(111, 1);
  BOOST_CHECK(!f.accessor.readNonBlocking());
  f.firmwareFinishesBuffer(222, 0);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  f.device.close();
}

/**********************************************************************************************************************/

// I3: while a consumer is pending, switch the selector 1 -> 2 -> 1. The consumer only wakes after MUX_SEL is
// restored to 1, and never observes data tagged with the wrong selection.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptSwitchSelector) {
  SelectedByInterruptFixture f;
  f.muxSel[0] = 1; // select DAQ.DATA
  f.device.activateAsyncRead();

  // Initial value arrives immediately.
  BOOST_REQUIRE(f.accessor.readNonBlocking());

  // Point the selector away: the next interrupt must not wake the DAQ.DATA consumer.
  f.muxSel[0] = 2;
  f.firmwareFinishesBuffer(333, 1);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Still not selected: another inactive interrupt stays silent.
  f.firmwareFinishesBuffer(444, 0);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Restore the selection and fill a fresh buffer: the consumer now wakes with that buffer.
  f.muxSel[0] = 1;
  f.firmwareFinishesBuffer(555, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK_EQUAL(f.accessor[0], 555);

  f.device.close();
}

/**********************************************************************************************************************/

// I4: activateAsyncRead while the selection is not met delivers the initial value immediately, marked
// DataValidity::faulty; it becomes valid once the selector matches and new data arrives.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptInitialValueUnselected) {
  SelectedByInterruptFixture f;
  f.muxSel[0] = 2; // DAQ.DATA unselected at activation
  f.device.activateAsyncRead();

  // Initial value is delivered but marked faulty.
  BOOST_REQUIRE(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() == ChimeraTK::DataValidity::faulty);

  // While still unselected, further interrupts do not deliver new (valid) data.
  f.firmwareFinishesBuffer(10, 1);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Now select DAQ.DATA and finish a buffer: a valid value follows.
  f.muxSel[0] = 1;
  f.firmwareFinishesBuffer(20, 0);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() != ChimeraTK::DataValidity::faulty);
  BOOST_CHECK_EQUAL(f.accessor[0], 20);

  f.device.close();
}

/**********************************************************************************************************************/

// I5: double-buffered + muxed: drive several buffer swaps while alternating the selector and assert that every
// delivered version corresponds to a buffer filled while DAQ.DATA was selected.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptAlternatingSwaps) {
  SelectedByInterruptFixture f;
  f.device.activateAsyncRead();

  // Start unselected: the initial value arrives faulty.
  f.muxSel[0] = 2;
  BOOST_REQUIRE(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Swap 1 while selected: buffer0 (=1001) delivered.
  f.muxSel[0] = 1;
  f.firmwareFinishesBuffer(1001, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() != ChimeraTK::DataValidity::faulty);
  BOOST_CHECK_EQUAL(f.accessor[0], 1001);

  // Swap 2 while unselected: not delivered (stays on the previous valid version).
  f.muxSel[0] = 2;
  f.firmwareFinishesBuffer(9999, 0);
  BOOST_CHECK(!f.accessor.readNonBlocking());

  // Swap 3 while selected again: the newly finished buffer is delivered.
  f.muxSel[0] = 1;
  f.firmwareFinishesBuffer(1002, 1);
  BOOST_CHECK(f.accessor.readNonBlocking());
  BOOST_CHECK(f.accessor.dataValidity() != ChimeraTK::DataValidity::faulty);
  BOOST_CHECK_EQUAL(f.accessor[0], 1002);

  f.device.close();
}

/**********************************************************************************************************************/

// I6: Two subscriptions to mutually exclusive alternatives (DAQ.DATA sel 1, DAQ.DATA_ALT sel 2) that share the
// SAME selector register DAQ.MUX_SEL. This exercises the shared-selector path in buildSelectorGate(): both
// variables gate on one shared selector accessor that is added to the transfer group (read once per poll,
// deduplicated), and each consumer wakes only when its own selection is active.
BOOST_AUTO_TEST_CASE(testSelectedByInterruptSharedSelector) {
  Device device;
  device.open("(dummy?map=selectedByInterrupt.jmap)");
  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(device.getBackend());
  BOOST_REQUIRE(dummy);
  auto data = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DATA", 1, 0, {AccessMode::wait_for_new_data});
  auto dataAlt = device.getOneDRegisterAccessor<uint32_t>("/DAQ/DATA_ALT", 1, 0, {AccessMode::wait_for_new_data});
  DummyRegisterAccessor<uint32_t> muxSel(dummy.get(), "DAQ", "MUX_SEL");
  DummyRegisterAccessor<uint32_t> enable(dummy.get(), "DAQ/DOUBLE_BUF", "ENA");
  DummyRegisterAccessor<uint32_t> inactive(dummy.get(), "DAQ/DOUBLE_BUF", "INACTIVE_BUF_ID");
  DummyRegisterAccessor<uint32_t> buffer0(dummy.get(), "DAQ/DATA", "BUF0");
  DummyRegisterAccessor<uint32_t> buffer1(dummy.get(), "DAQ/DATA", "BUF1");
  enable[0] = 1;

  auto finishBuffer = [&](uint32_t v, uint32_t buf) {
    if(buf == 1) {
      buffer0 = v;
    }
    else {
      buffer1 = v;
    }
    inactive[0] = buf;
    dummy->triggerInterrupt(1);
  };

  // Select DATA_ALT (MUX_SEL==2). Both initial values arrive: DATA is faulty, DATA_ALT valid.
  muxSel[0] = 2;
  device.activateAsyncRead();
  BOOST_REQUIRE(data.readNonBlocking());
  BOOST_CHECK(data.dataValidity() == ChimeraTK::DataValidity::faulty);
  BOOST_REQUIRE(dataAlt.readNonBlocking());
  BOOST_CHECK(dataAlt.dataValidity() != ChimeraTK::DataValidity::faulty);

  // A finished buffer while DATA_ALT is selected: DATA_ALT wakes with it, DATA stays quiet.
  finishBuffer(100, 1);
  BOOST_CHECK(!data.readNonBlocking());
  BOOST_CHECK(dataAlt.readNonBlocking());
  BOOST_CHECK_EQUAL(dataAlt[0], 100);

  // Switch to DATA: DATA wakes with the freshly finished buffer, DATA_ALT stays quiet.
  muxSel[0] = 1;
  finishBuffer(200, 0);
  BOOST_CHECK(data.readNonBlocking());
  BOOST_CHECK(data.dataValidity() != ChimeraTK::DataValidity::faulty);
  BOOST_CHECK_EQUAL(data[0], 200);
  BOOST_CHECK(!dataAlt.readNonBlocking());

  device.close();
}

/**********************************************************************************************************************/
