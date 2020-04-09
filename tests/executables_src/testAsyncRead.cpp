#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AsyncReadTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <algorithm>
#include <atomic>
#include <thread>

#include <boost/thread.hpp>

#include "Device.h"
#include "DeviceAccessVersion.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "ReadAnyGroup.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

static std::set<std::string> sdmList = {"sdm://./AsyncTestDummy"};

/**********************************************************************************************************************/

class AsyncTestDummy : public DeviceBackendImpl {
 public:
  explicit AsyncTestDummy() { FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl); }

  ~AsyncTestDummy() override;

  std::string readDeviceInfo() override { return std::string("AsyncTestDummy"); }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string,
      std::string,
      std::list<std::string>,
      std::string) {
    return boost::shared_ptr<DeviceBackend>(new AsyncTestDummy());
  }

  template<typename UserType>
  class Accessor : public NDRegisterAccessor<UserType> {
   public:
    Accessor(AsyncTestDummy* backend, const RegisterPath& registerPathName, AccessModeFlags& flags)
    : NDRegisterAccessor<UserType>(registerPathName), _backend(backend), _flags(flags) {
      buffer_2D.resize(1);
      buffer_2D[0].resize(1);
    }

    ~Accessor() override {}

    TransferFuture doReadTransferAsync() override {
      // create future_queue if not already created and continue it to enusre
      // postRead is called (in the user thread, so we use the deferred launch
      // policy)
      if(!futureCreated) {
        _backend->notificationQueue[getName()] = cppext::future_queue<void>(2);
        activeFuture = TransferFuture(_backend->notificationQueue[getName()], this);
        futureCreated = true;
      }

      // return the TransferFuture
      return activeFuture;
    }

    void doReadTransfer() override { doReadTransferAsync().wait(); }

    bool doReadTransferNonBlocking() override { return true; }

    bool doReadTransferLatest() override { return true; }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      currentVersion = versionNumber;
      return true;
    }

    void doPreWrite(TransferType) override {}

    void doPostWrite(TransferType) override {}

    void doPreRead(TransferType) override {}

    void doPostRead(TransferType) override {
      buffer_2D[0][0] = _backend->registers.at(getName());
      currentVersion = {};
    }

    AccessModeFlags getAccessModeFlags() const override { return _flags; }
    bool isReadOnly() const override { return false; }
    bool isReadable() const override { return true; }
    bool isWriteable() const override { return true; }

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {this->shared_from_this()};
    }
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    bool futureCreated{false};

    VersionNumber getVersionNumber() const override { return currentVersion; }

   protected:
    AsyncTestDummy* _backend;
    AccessModeFlags _flags;
    using NDRegisterAccessor<UserType>::getName;
    using NDRegisterAccessor<UserType>::buffer_2D;
    using TransferElement::activeFuture;

    VersionNumber currentVersion;
  };

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(const RegisterPath& registerPathName,
      size_t numberOfWords,
      size_t wordOffsetInRegister,
      AccessModeFlags flags) {
    assert(numberOfWords == 1);
    assert(wordOffsetInRegister == 0);
    (void)numberOfWords;
    (void)wordOffsetInRegister;
    return boost::make_shared<Accessor<UserType>>(this, registerPathName, flags);
  }

  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(AsyncTestDummy, getRegisterAccessor_impl, 4);

  void open() override { _opened = true; }

  void close() override { _opened = false; }

  bool isFunctional() const override { return _opened; }

  std::map<std::string, cppext::future_queue<void>> notificationQueue;
  std::map<std::string, size_t> registers;
};

AsyncTestDummy::~AsyncTestDummy() {}

/**********************************************************************************************************************/

struct Fixture {
  Fixture() {
    BackendFactory::getInstance().registerBackendType(
        "AsyncTestDummy", "", &AsyncTestDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
    BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  }
};
static Fixture fixture;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAsyncRead) {
  for(auto& sdmToUse : sdmList) {
    std::cout << "testAsyncRead: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK(backend != nullptr);

    // obtain register accessor with integral type
    auto accessor = device.getScalarRegisterAccessor<int>("REG", 0, {AccessMode::wait_for_new_data});

    // simple reading through readAsync without actual need
    TransferFuture future;
    backend->registers["/REG"] = 5;
    future = accessor.readAsync();
    backend->notificationQueue["/REG"].push(); // trigger transfer
    future.wait();
    BOOST_CHECK(accessor == 5);
    BOOST_CHECK(backend->notificationQueue["/REG"].empty());

    backend->registers["/REG"] = 6;
    TransferFuture future2 = accessor.readAsync();
    backend->notificationQueue["/REG"].push(); // trigger transfer
    future2.wait();
    BOOST_CHECK(accessor == 6);
    BOOST_CHECK(backend->notificationQueue["/REG"].empty());

    // check that future's wait() function won't return before the read is
    // complete
    for(int i = 0; i < 5; ++i) {
      backend->registers["/REG"] = 42 + i;
      future = accessor.readAsync();
      std::atomic<bool> flag;
      flag = false;
      std::thread thread([&future, &flag] {
        future.wait();
        flag = true;
      });
      usleep(100000);
      BOOST_CHECK(flag == false);
      backend->notificationQueue["/REG"].push(); // trigger transfer
      thread.join();
      BOOST_CHECK(accessor == 42 + i);
      BOOST_CHECK(backend->notificationQueue["/REG"].empty());
    }

    // check that obtaining the same future multiple times works properly
    backend->registers["/REG"] = 666;
    for(int i = 0; i < 5; ++i) {
      future = accessor.readAsync();
      BOOST_CHECK(accessor == 46); // still the old value from the last test part
    }
    backend->notificationQueue["/REG"].push(); // trigger transfer
    future.wait();
    BOOST_CHECK(accessor == 666);
    BOOST_CHECK(backend->notificationQueue["/REG"].empty());

    // now try another asynchronous transfer
    backend->registers["/REG"] = 999;
    future = accessor.readAsync();
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] {
      future.wait();
      flag = true;
    });
    usleep(100000);
    BOOST_CHECK(flag == false);
    backend->notificationQueue["/REG"].push(); // trigger transfer
    thread.join();
    BOOST_CHECK(accessor == 999);
    BOOST_CHECK(backend->notificationQueue["/REG"].empty());

    device.close();
  }
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAny) {
  for(auto& sdmToUse : sdmList) {
    std::cout << "testReadAny: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
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
      BOOST_CHECK(a3 == 3);
      BOOST_CHECK(a4 == 4);
      BOOST_CHECK(id == a1.getId());

      // retrigger the transfer
      a1.readAsync();
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

      // retrigger the transfer
      a3.readAsync();
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

      // retrigger the transfer
      a3.readAsync();
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

      // retrigger the transfer
      a2.readAsync();
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

      // retrigger the transfer
      a4.readAsync();
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

      // retrigger the transfer
      a4.readAsync();
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

      // retrigger the transfer
      a3.readAsync();
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

      // retrigger the transfers
      a1.readAsync();
      a2.readAsync();
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
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadAnyWithPoll) {
  for(auto& sdmToUse : sdmList) {
    std::cout << "testReadAnyWithPoll: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
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

      // retrigger the transfer
      a1.readAsync();
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

      // retrigger the transfer
      a2.readAsync();
    }

    device.close();
  }
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWaitAny) {
  for(auto& sdmToUse : sdmList) {
    std::cout << "testWaitAny: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
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

      // retrigger the transfer
      a1.readAsync();
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

      // retrigger the transfer
      a2.readAsync();
    }

    device.close();
  }
}
