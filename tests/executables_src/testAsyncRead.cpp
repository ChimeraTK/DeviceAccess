///@todo FIXME My dynamic init header is a hack. Change the test to use BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"

#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/thread.hpp>

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "DeviceAccessVersion.h"
#include "ExperimentalFeatures.h"
#include "NDRegisterAccessorDecorator.h"
#include "ReadAnyGroup.h"

namespace mtca4u{
  using namespace ChimeraTK;
}

using namespace boost::unit_test_framework;
using namespace mtca4u;

static std::set<std::string> sdmList = { "sdm://./AsyncTestDummy" };

/**********************************************************************************************************************/

class AsyncTestDummy : public DeviceBackendImpl {
  public:
    explicit AsyncTestDummy()
    {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    std::string readDeviceInfo() override {
      return std::string("AsyncTestDummy");
    }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string>, std::string) {
      return boost::shared_ptr<DeviceBackend>(new AsyncTestDummy());
    }

    template<typename UserType>
    class Accessor : public SyncNDRegisterAccessor<UserType> {
      public:
        Accessor(AsyncTestDummy *backend, const RegisterPath &registerPathName, AccessModeFlags &flags)
        : SyncNDRegisterAccessor<UserType>(registerPathName), _backend(backend), _flags(flags)
        {
          buffer_2D.resize(1);
          buffer_2D[0].resize(1);
        }

        ~Accessor() override { this->shutdown(); }

        void doReadTransfer() override {
          if(_flags.has(AccessMode::wait_for_new_data)) {
            while(!_backend->readMutex.at(getName()).try_lock_for(std::chrono::milliseconds(100))) {
              boost::this_thread::interruption_point();
            }
          }
          ++_backend->readCount.at(getName());
          if(_flags.has(AccessMode::wait_for_new_data)) {
            _backend->readMutex.at(getName()).unlock();
          }
        }

        bool doReadTransferNonBlocking() override {
          return true;
        }

        bool doReadTransferLatest() override {
          return true;
        }

        bool doWriteTransfer(ChimeraTK::VersionNumber={}) override {
          return true;
        }

        void doPreWrite() override {
        }

        void doPostWrite() override {
        }

        void doPreRead() override {
        }

        void doPostRead() override {
          buffer_2D[0][0] = _backend->registers.at(getName());
        }

        AccessModeFlags getAccessModeFlags() const override { return _flags; }
        bool isReadOnly() const override { return false; }
        bool isReadable() const override { return true; }
        bool isWriteable() const override { return true; }

        std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() override {
          return {this->shared_from_this()};
        }
        std::list< boost::shared_ptr<TransferElement> > getInternalElements() override { return {}; }

      protected:
        AsyncTestDummy *_backend;
        AccessModeFlags _flags;
        using NDRegisterAccessor<UserType>::getName;
        using NDRegisterAccessor<UserType>::buffer_2D;

    };

    template<typename UserType>
    boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
        const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
      assert(numberOfWords == 1);
      assert(wordOffsetInRegister == 0);
      return boost::make_shared<Accessor<UserType>>( this, registerPathName, flags );
    }

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( AsyncTestDummy, getRegisterAccessor_impl, 4 );

    void open() override {
      _opened = true;
    }

    void close() override {
      _opened = false;
    }

    std::map<std::string, std::timed_mutex> readMutex;
    std::map<std::string, size_t> readCount;
    std::map<std::string, size_t> registers;
};

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

    /// test the readAny() function
    void testReadAny();

    /// test the readAny() function when also including poll-type variables (no AccessMode::wait_for_new_data)
    void testReadAnyWithPoll();
};

/**********************************************************************************************************************/
class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAny, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAnyWithPoll, asyncReadTest ) );
    }};

/**********************************************************************************************************************/
bool init_unit_test(){
  std::cout << "This is the alternative init" << std::endl;
  BackendFactory::getInstance().registerBackendType("AsyncTestDummy","",&AsyncTestDummy::createInstance,
                                                    CHIMERATK_DEVICEACCESS_VERSION);
  ChimeraTK::ExperimentalFeatures::enable();

  framework::master_test_suite().p_name.value = "Async read test suite";
  framework::master_test_suite().add(new AsyncReadTestSuite);

  return true;
}


/**********************************************************************************************************************/
void AsyncReadTest::testAsyncRead() {

  for(auto &sdmToUse : sdmList) {
    std::cout << "testAsyncRead: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != nullptr );

    // obtain register accessor with integral type
    auto accessor = device.getScalarRegisterAccessor<int>("REG", 0, {AccessMode::wait_for_new_data});

    // create the mutex for the register
    backend->readMutex["/REG"].unlock();
    backend->readCount["/REG"] = 0;

    // simple reading through readAsync without actual need
    TransferFuture future;
    backend->registers["/REG"] = 5;
    future = accessor.readAsync();
    future.wait();
    BOOST_CHECK( accessor == 5 );
    BOOST_CHECK( backend->readCount["/REG"] == 1 );

    backend->registers["/REG"] = 6;
    TransferFuture future2 = accessor.readAsync();
    future2.wait();
    BOOST_CHECK( accessor == 6 );
    BOOST_CHECK( backend->readCount["/REG"] == 2 );

    // check that future's wait() function won't return before the read is complete
    for(int i=0; i<5; ++i) {
      backend->registers["/REG"] = 42+i;
      backend->readMutex["/REG"].lock();
      future = accessor.readAsync();
      std::atomic<bool> flag;
      flag = false;
      std::thread thread([&future, &flag] { future.wait(); flag = true; });
      usleep(100000);
      BOOST_CHECK(flag == false);
      backend->readMutex["/REG"].unlock();
      thread.join();
      BOOST_CHECK( accessor == 42+i );
      BOOST_CHECK( backend->readCount["/REG"] == 3+(unsigned)i );
    }

    // check that obtaining the same future multiple times works properly
    backend->readMutex["/REG"].lock();
    backend->registers["/REG"] = 666;
    for(int i=0; i<5; ++i) {
      future = accessor.readAsync();
      BOOST_CHECK( accessor == 46 );    // still the old value from the last test part
    }
    backend->readMutex["/REG"].unlock();
    future.wait();
    BOOST_CHECK( accessor == 666 );

    // now try another asynchronous transfer
    backend->registers["/REG"] = 999;
    backend->readMutex["/REG"].lock();
    future = accessor.readAsync();
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] { future.wait(); flag = true; });
    usleep(100000);
    BOOST_CHECK(flag == false);
    backend->readMutex["/REG"].unlock();
    thread.join();
    BOOST_CHECK( accessor == 999 );

    device.close();

  }

}

/**********************************************************************************************************************/

void AsyncReadTest::testReadAny() {

  for(auto &sdmToUse : sdmList) {
    std::cout << "testReadAny: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != nullptr );

    // obtain register accessor with integral type
    auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
    auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
    auto a3 = device.getScalarRegisterAccessor<int32_t>("a3", 0, {AccessMode::wait_for_new_data});
    auto a4 = device.getScalarRegisterAccessor<int32_t>("a4", 0, {AccessMode::wait_for_new_data});

    // lock all mutexes so no read can complete
    backend->readMutex["/a1"].lock();
    backend->readMutex["/a2"].lock();
    backend->readMutex["/a3"].lock();
    backend->readMutex["/a4"].lock();
    backend->readCount["/a1"] = 0;
    backend->readCount["/a2"] = 0;
    backend->readCount["/a3"] = 0;
    backend->readCount["/a4"] = 0;

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
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex.at("/a1").unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 2 );
      BOOST_CHECK( a3 == 3 );
      BOOST_CHECK( a4 == 4 );
      BOOST_CHECK( id == a1.getId() );
      backend->readMutex.at("/a1").lock();

      // retrigger the transfer
      a1.readAsync();
    }

    // register 3
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex.at("/a3").unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 2 );
      BOOST_CHECK( a3 == 120 );
      BOOST_CHECK( a4 == 4 );
      BOOST_CHECK( id == a3.getId() );
      backend->readMutex.at("/a3").lock();

      // retrigger the transfer
      a3.readAsync();
    }

    // register 3 again
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->registers["/a3"] = 121;
      backend->readMutex["/a3"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 2 );
      BOOST_CHECK( a3 == 121 );
      BOOST_CHECK( a4 == 4 );
      BOOST_CHECK( id == a3.getId() );
      backend->readMutex["/a3"].lock();

      // retrigger the transfer
      a3.readAsync();
    }

    // register 2
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex["/a2"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 123 );
      BOOST_CHECK( a3 == 121 );
      BOOST_CHECK( a4 == 4 );
      BOOST_CHECK( id == a2.getId() );
      backend->readMutex["/a2"].lock();

      // retrigger the transfer
      a2.readAsync();
    }

    // register 4
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex["/a4"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 123 );
      BOOST_CHECK( a3 == 121 );
      BOOST_CHECK( a4 == 345 );
      BOOST_CHECK( id == a4.getId() );
      backend->readMutex["/a4"].lock();

      // retrigger the transfer
      a4.readAsync();
    }

    // register 4 again
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex["/a4"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 123 );
      BOOST_CHECK( a3 == 121 );
      BOOST_CHECK( a4 == 345 );
      BOOST_CHECK( id == a4.getId() );
      backend->readMutex["/a4"].lock();

      // retrigger the transfer
      a4.readAsync();
    }

    // register 3 a 3rd time
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->registers["/a3"] = 122;
      backend->readMutex["/a3"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 123 );
      BOOST_CHECK( a3 == 122 );
      BOOST_CHECK( a4 == 345 );
      BOOST_CHECK( id == a3.getId() );
      backend->readMutex["/a3"].lock();

      // retrigger the transfer
      a3.readAsync();
    }

    // register 1 and then register 2 (order should be guaranteed)
    {
      // write to register 1 and launch the asynchronous read on it - but only wait on the underlying BOOST future
      // so postRead() is not yet called.
      backend->registers["/a1"] = 55;
      backend->readCount["/a1"] = 0;
      while(backend->readCount["/a1"] == 0) {
        backend->readMutex["/a1"].unlock();
        backend->readMutex["/a1"].lock();
      }

      // same with register 2
      backend->registers["/a2"] = 66;
      backend->readCount["/a2"] = 0;
      while(backend->readCount["/a2"] == 0) {
        backend->readMutex["/a2"].unlock();
        backend->readMutex["/a2"].lock();
      }
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
      backend->readCount["/a4"] = 0;
      while(backend->readCount["/a4"] == 0) {
        backend->readMutex["/a4"].unlock();
        backend->readMutex["/a4"].lock();
      }

      // register 2
      backend->registers["/a2"] = 22;
      backend->readCount["/a2"] = 0;
      while(backend->readCount["/a2"] == 0) {
        backend->readMutex["/a2"].unlock();
        backend->readMutex["/a2"].lock();
      }

      // register 3
      backend->registers["/a3"] = 33;
      backend->readCount["/a3"] = 0;
      while(backend->readCount["/a3"] == 0) {
        backend->readMutex["/a3"].unlock();
        backend->readMutex["/a3"].lock();
      }

      // register 1
      backend->registers["/a1"] = 44;
      backend->readCount["/a1"] = 0;
      while(backend->readCount["/a1"] == 0) {
        backend->readMutex["/a1"].unlock();
        backend->readMutex["/a1"].lock();
      }

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

void AsyncReadTest::testReadAnyWithPoll() {

  for(auto &sdmToUse : sdmList) {
    std::cout << "testReadAnyWithPoll: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != nullptr );

    // obtain register accessor with integral type
    auto a1 = device.getScalarRegisterAccessor<uint8_t>("a1", 0, {AccessMode::wait_for_new_data});
    auto a2 = device.getScalarRegisterAccessor<int32_t>("a2", 0, {AccessMode::wait_for_new_data});
    auto a3 = device.getScalarRegisterAccessor<int32_t>("a3");
    auto a4 = device.getScalarRegisterAccessor<int32_t>("a4");

    // lock all mutexes so no read can complete
    backend->readMutex["/a1"].lock();
    backend->readMutex["/a2"].lock();
    backend->readMutex["/a3"].unlock();
    backend->readMutex["/a4"].unlock();
    backend->readCount["/a1"] = 0;
    backend->readCount["/a2"] = 0;
    backend->readCount["/a3"] = 0;
    backend->readCount["/a4"] = 0;

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
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex.at("/a1").unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 2 );
      BOOST_CHECK( a3 == 120 );
      BOOST_CHECK( a4 == 345 );
      BOOST_CHECK( id == a1.getId() );
      backend->readMutex.at("/a1").lock();

      // retrigger the transfer
      a1.readAsync();
    }

    backend->registers["/a3"] = 121;
    backend->registers["/a4"] = 346;

    // register 2
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&group,&flag,&id] { id = group.readAny(); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex["/a2"].unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      BOOST_CHECK( a2 == 123 );
      BOOST_CHECK( a3 == 121 );
      BOOST_CHECK( a4 == 346 );
      BOOST_CHECK( id == a2.getId() );
      backend->readMutex["/a2"].lock();

      // retrigger the transfer
      a2.readAsync();
    }

    device.close();

  }

}
