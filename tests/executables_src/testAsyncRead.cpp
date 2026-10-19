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

class AsyncTestDummy : public DeviceBackendImpl {
 public:
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

// A gated register read with 'wait_for_new_data' is served by the triggered-poll path. While its gate is open an
// interrupt delivers a new value; while the gate is closed the interrupt does not wake the consumer and publishes no
// new version number.
BOOST_AUTO_TEST_CASE(testAsyncReadGatedBy) {
  std::cout << "testAsyncReadGatedBy" << std::endl;

  Device device("(dummy?map=gatedByInterrupt.jmap)");
  device.open();
  device.activateAsyncRead();

  auto backend = boost::dynamic_pointer_cast<DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend != nullptr);

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  // A writable register at the payload address lets the test place the raw value that an interrupt reads.
  auto payload = device.getScalarRegisterAccessor<uint32_t>("DATA/SCRATCH");

  // Open the gate for INT_GATED and write the value the accessor must pick up as its initial value.
  sel = 1;
  sel.write();
  payload = 0x1234;
  payload.write();

  auto gated = device.getScalarRegisterAccessor<uint32_t>("DATA/INT_GATED", 0, {AccessMode::wait_for_new_data});
  // The closed alternative at the same address is observed through a plain polled accessor, which does not join the
  // asynchronous transfer group and therefore does not merge with the gated accessor's payload element.
  auto gatedAlt = device.getScalarRegisterAccessor<uint32_t>("DATA/INT_GATED_ALT");

  // The mandatory initial value after activation is delivered even before the first interrupt.
  gated.read();
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(static_cast<uint32_t>(gated) == 0x1234u);
  auto versionAfterInitial = gated.getVersionNumber();

  // An interrupt through the open gate delivers a new value and advances the version number.
  payload = 0x5678;
  payload.write();
  backend->triggerInterrupt(1);
  BOOST_TEST(gated.readNonBlocking());
  BOOST_TEST(static_cast<uint32_t>(gated) == 0x5678u);
  BOOST_TEST(gated.getVersionNumber() > versionAfterInitial);

  // The alternative register is closed (CTRL.SEL == 1 opens INT_GATED, not INT_GATED_ALT).
  gatedAlt.read();
  BOOST_TEST(gatedAlt.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Close the gate: an interrupt must not wake the consumer and must not publish a new version number.
  sel = 0;
  sel.write();

  auto waitForRead = std::async(std::launch::async, [&gated] { gated.read(); });
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  payload = 0x9999;
  payload.write();
  backend->triggerInterrupt(1);
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(1)) == std::future_status::timeout); // no wake
  BOOST_TEST(!gated.readNonBlocking());                                                     // no new version
  BOOST_TEST(gated.getVersionNumber() == versionAfterInitial); // version number unchanged
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty); // gate stays closed

  // Reopen the gate while the consumer is pending: the next interrupt wakes it and delivers the fresh value.
  sel = 1;
  sel.write();
  payload = 0xabcd;
  payload.write();
  backend->triggerInterrupt(1);
  BOOST_CHECK(waitForRead.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  waitForRead.wait();
  BOOST_TEST(static_cast<uint32_t>(gated) == 0xabcdu);
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(gated.getVersionNumber() > versionAfterInitial);

  device.close();
}

/**********************************************************************************************************************/

// Two gated alternatives at the same address are served by the triggered-poll path at once. Their payload elements
// merge in the transfer group, so the second gate accessor is re-registered with the merged element's gate group. A
// change of the gating register while the consumer is pending picks the matching alternative; the other one stays
// closed and is never delivered.
BOOST_AUTO_TEST_CASE(testAsyncReadGatedByAlternative) {
  std::cout << "testAsyncReadGatedByAlternative" << std::endl;

  Device device("(dummy?map=gatedByInterrupt.jmap)");
  device.open();
  device.activateAsyncRead();

  auto backend = boost::dynamic_pointer_cast<DummyBackend>(device.getBackend());
  BOOST_REQUIRE(backend != nullptr);

  auto sel = device.getScalarRegisterAccessor<uint32_t>("CTRL/SEL");
  auto payload = device.getScalarRegisterAccessor<uint32_t>("DATA/SCRATCH");

  // Start with the alternative's gate open (CTRL.SEL == 2) and write a value for it.
  sel = 2;
  sel.write();
  payload = 0x2222;
  payload.write();

  auto gated = device.getScalarRegisterAccessor<uint32_t>("DATA/INT_GATED", 0, {AccessMode::wait_for_new_data});
  auto gatedAlt = device.getScalarRegisterAccessor<uint32_t>("DATA/INT_GATED_ALT", 0, {AccessMode::wait_for_new_data});

  gatedAlt.read();
  gated.read();
  BOOST_TEST(gatedAlt.dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_TEST(gated.dataValidity() == ChimeraTK::DataValidity::faulty);
  BOOST_TEST(static_cast<uint32_t>(gatedAlt) == 0x2222u);

  // While the alternative is pending, switch the gate to the first register and place a value for it.
  auto waitForAlt = std::async(std::launch::async, [&gatedAlt] { gatedAlt.read(); });
  BOOST_CHECK(waitForAlt.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  sel = 1;
  sel.write();
  payload = 0x1111;
  payload.write();

  // The interrupt now goes through the first register's gate; the alternative stays closed and is not woken.
  backend->triggerInterrupt(1);
  BOOST_TEST(gated.readNonBlocking());
  BOOST_TEST(static_cast<uint32_t>(gated) == 0x1111u);
  BOOST_CHECK(waitForAlt.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);
  BOOST_TEST(gatedAlt.dataValidity() == ChimeraTK::DataValidity::faulty);

  // Reopen the alternative's gate so the pending read can finish before the device is closed.
  sel = 2;
  sel.write();
  backend->triggerInterrupt(1);
  BOOST_CHECK(waitForAlt.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  waitForAlt.wait();
  BOOST_TEST(gatedAlt.dataValidity() == ChimeraTK::DataValidity::ok);

  device.close();
}

/**********************************************************************************************************************/
