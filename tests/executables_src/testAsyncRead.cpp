#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/thread.hpp>
#include <boost/test/included/unit_test.hpp>

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "DeviceAccessVersion.h"
#include "ExperimentalFeatures.h"
#include "NDRegisterAccessorDecorator.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

std::set<std::string> sdmList = { "sdm://./AsyncDefaultImplTestDummy=goodMapFile.map",
                                  "sdm://./AsyncTestDummy=goodMapFile.map"              };

// We test with two different backends, one is using the default implementation of readAsync(), the other is
// implementing readAsync() itself. Since we base both backends on DummyBackend, we use in both cases actually the
// default implementation. The difference is that in case of the AsyncTestDummy a decorator is used for both the
// register accessor and the TransferFuture, so it can be tested that no part is relying on the exact default
// implementation.

/**********************************************************************************************************************/

class AsyncDefaultImplTestDummy : public DummyBackend {
  public:
    explicit AsyncDefaultImplTestDummy(std::string mapFileName) : DummyBackend(mapFileName) {}

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string> parameters, std::string) {
      return boost::shared_ptr<DeviceBackend>(new AsyncDefaultImplTestDummy(parameters.front()));
    }

    void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) override {
      while(!readMutex.at(address).try_lock_for(std::chrono::milliseconds(100))) {
        boost::this_thread::interruption_point();
      }
      DummyBackend::read(bar,address,data,sizeInBytes);
      readMutex.at(address).unlock();
    }

    std::map<int, std::timed_mutex> readMutex;
};

/**********************************************************************************************************************/

class AsyncTestDummy : public AsyncDefaultImplTestDummy {
  public:
    explicit AsyncTestDummy(std::string mapFileName) : AsyncDefaultImplTestDummy(mapFileName) {
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
    }

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string> parameters, std::string) {
      return boost::shared_ptr<DeviceBackend>(new AsyncTestDummy(parameters.front()));
    }

    template<typename UserType>
    boost::shared_ptr< NDRegisterAccessor<UserType> > getRegisterAccessor_impl(
        const RegisterPath &registerPathName, size_t wordOffsetInRegister, size_t numberOfWords, AccessModeFlags flags) {

      auto acc = NumericAddressedBackend::getRegisterAccessor_impl<UserType>(registerPathName, wordOffsetInRegister,
                                                                             numberOfWords, flags);

      return boost::make_shared<mtca4u::NDRegisterAccessorDecorator<UserType>>(acc);
    }
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( AsyncTestDummy, getRegisterAccessor_impl, 4 );

};

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

    /// test the TransferElement::readAny() function
    void testReadAny();

    /// test mixing the various read functions
    void testMixing();

};

/**********************************************************************************************************************/
class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAny, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testMixing, asyncReadTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  BackendFactory::getInstance().registerBackendType("AsyncTestDummy","",&AsyncTestDummy::createInstance,
                                                    CHIMERATK_DEVICEACCESS_VERSION);
  BackendFactory::getInstance().registerBackendType("AsyncDefaultImplTestDummy","",&AsyncDefaultImplTestDummy::createInstance,
                                                    CHIMERATK_DEVICEACCESS_VERSION);
  ChimeraTK::ExperimentalFeatures::enable();

  framework::master_test_suite().p_name.value = "Async read test suite";
  framework::master_test_suite().add(new AsyncReadTestSuite);

  return NULL;
}

/**********************************************************************************************************************/
void AsyncReadTest::testAsyncRead() {

  for(auto &sdmToUse : sdmList) {
    std::cout << "testAsyncRead: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncDefaultImplTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != NULL );

    // obtain register accessor with integral type
    auto accessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");

    // dummy register accessor for comparison
    DummyRegisterAccessor<int> dummy(backend.get(),"APP0","WORD_STATUS");

    // create the mutex for the register
    backend->readMutex[0x08].unlock();

    // simple reading through readAsync without actual need
    TransferFuture future;
    dummy = 5;
    future = accessor.readAsync();
    future.wait();
    BOOST_CHECK( accessor == 5 );

    dummy = 6;
    TransferFuture future2 = accessor.readAsync();
    future2.wait();
    BOOST_CHECK( accessor == 6 );

    // check that future's wait() function won't return before the read is complete
    for(int i=0; i<5; ++i) {
      dummy = 42+i;
      backend->readMutex[0x08].lock();
      future = accessor.readAsync();
      std::atomic<bool> flag;
      flag = false;
      std::thread thread([&future, &flag] { future.wait(); flag = true; });
      usleep(100000);
      BOOST_CHECK(flag == false);
      backend->readMutex[0x08].unlock();
      thread.join();
      BOOST_CHECK( accessor == 42+i );
    }

    // check that obtaining the same future multiple times works properly
    backend->readMutex[0x08].lock();
    dummy = 666;
    for(int i=0; i<5; ++i) {
      future = accessor.readAsync();
      BOOST_CHECK( accessor == 46 );    // still the old value from the last test part
    }
    backend->readMutex[0x08].unlock();
    future.wait();
    BOOST_CHECK( accessor == 666 );

    // now try another asynchronous transfer
    dummy = 999;
    backend->readMutex[0x08].lock();
    future = accessor.readAsync();
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] { future.wait(); flag = true; });
    usleep(100000);
    BOOST_CHECK(flag == false);
    backend->readMutex[0x08].unlock();
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
    auto backend = boost::dynamic_pointer_cast<AsyncDefaultImplTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != NULL );

    // obtain register accessor with integral type
    auto a1 = device.getScalarRegisterAccessor<uint8_t>("MODULE0/WORD_USER1");
    auto a2 = device.getScalarRegisterAccessor<int32_t>("MODULE0/WORD_USER2");
    auto a3 = device.getScalarRegisterAccessor<int32_t>("MODULE1/WORD_USER1");
    auto a4 = device.getScalarRegisterAccessor<int32_t>("MODULE1/WORD_USER2");

    // dummy register accessor for comparison
    DummyRegisterAccessor<uint8_t> dummy1(backend.get(),"MODULE0","WORD_USER1");
    DummyRegisterAccessor<int32_t> dummy2(backend.get(),"MODULE0","WORD_USER2");
    DummyRegisterAccessor<int32_t> dummy3(backend.get(),"MODULE1","WORD_USER1");
    DummyRegisterAccessor<int32_t> dummy4(backend.get(),"MODULE1","WORD_USER2");

    // lock all mutexes so no read can complete
    backend->readMutex[0x10].lock();  // MODULE0/WORD_USER1
    backend->readMutex[0x14].lock();  // MODULE0/WORD_USER2
    backend->readMutex[0x20].lock();  // MODULE1/WORD_USER1
    backend->readMutex[0x24].lock();  // MODULE1/WORD_USER2

    // initialise the buffers of the accessors
    a1 = 1;
    a2 = 2;
    a3 = 3;
    a4 = 4;

    // initialise the dummy registers
    dummy1 = 42;
    dummy2 = 123;
    dummy3 = 120;
    dummy4 = 345;

    // register 1
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex.at(0x10).unlock();
      thread.join();
      BOOST_CHECK( a1 == 42 );
      backend->readMutex.at(0x10).lock();
    }

    // register 3
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex.at(0x20).unlock();
      thread.join();
      BOOST_CHECK( a3 == 120 );
      backend->readMutex.at(0x20).lock();
    }

    // register 3 again
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      dummy3 = 121;
      backend->readMutex[0x20].unlock();
      thread.join();
      BOOST_CHECK( a3 == 121 );
      backend->readMutex[0x20].lock();
    }

    // register 2
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex[0x14].unlock();
      thread.join();
      BOOST_CHECK( a2 == 123 );
      backend->readMutex[0x14].lock();
    }

    // register 4
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex[0x24].unlock();
      thread.join();
      BOOST_CHECK( a4 == 345 );
      backend->readMutex[0x24].lock();
    }

    // register 4 again
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      backend->readMutex[0x24].unlock();
      thread.join();
      BOOST_CHECK( a4 == 345 );
      backend->readMutex[0x24].lock();
    }

    // register 3 a 3rd time
    {
      // launch the readAny in a background thread
      std::atomic<bool> flag{false};
      std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });

      // check that it doesn't return too soon
      usleep(100000);
      BOOST_CHECK(flag == false);

      // write register and check that readAny() completes
      dummy3 = 122;
      backend->readMutex[0x20].unlock();
      thread.join();
      BOOST_CHECK( a3 == 122 );
      backend->readMutex[0x20].lock();
    }

    // register 1 and then register 2 (order should be guaranteed)
    {
      // write to register 1 and launch the asynchronous read on it - but only wait on the underlying BOOST future
      // so postRead() is not yet called.
      dummy1 = 55;
      backend->readMutex[0x10].unlock();
      TransferFuture f1 = a1.readAsync();
      f1.getBoostFuture().wait();
      backend->readMutex[0x10].lock();

      // same with register 2
      dummy2 = 66;
      backend->readMutex[0x14].unlock();
      TransferFuture f2 = a2.readAsync();
      f2.getBoostFuture().wait();
      backend->readMutex[0x14].lock();
      f1.getBoostFuture().wait();
      f2.getBoostFuture().wait();
      BOOST_CHECK_EQUAL((int)a1, 42);
      BOOST_CHECK_EQUAL((int)a2, 123);

      // no point to use a thread here
      auto r = TransferElement::readAny({a1,a2,a3,a4});
      BOOST_CHECK(a1.getId() == r);
      BOOST_CHECK_EQUAL((int)a1, 55);
      BOOST_CHECK_EQUAL((int)a2, 123);

      r = TransferElement::readAny({a1,a2,a3,a4});
      BOOST_CHECK(a2.getId() == r);
      BOOST_CHECK(a1 == 55);
      BOOST_CHECK(a2 == 66);
    }

    // registers in order: 4, 2, 3 and 1
    {
      // register 4 (see above for explanation)
      dummy4 = 11;
      backend->readMutex[0x24].unlock();
      TransferFuture f4 = a4.readAsync();
      f4.getBoostFuture().wait();
      backend->readMutex[0x24].lock();

      // register 2
      dummy2 = 22;
      backend->readMutex[0x14].unlock();
      TransferFuture f2 = a2.readAsync();
      f2.getBoostFuture().wait();
      backend->readMutex[0x14].lock();

      // register 3
      dummy3 = 33;
      backend->readMutex[0x20].unlock();
      TransferFuture f3 = a3.readAsync();
      f3.getBoostFuture().wait();
      backend->readMutex[0x20].lock();

      // register 1
      dummy1 = 44;
      backend->readMutex[0x10].unlock();
      TransferFuture f1 = a1.readAsync();
      f1.getBoostFuture().wait();
      backend->readMutex[0x10].lock();

      // no point to use a thread here
      auto r = TransferElement::readAny({a1,a2,a3,a4});
      BOOST_CHECK(a4.getId() == r);
      BOOST_CHECK(a1 == 55);
      BOOST_CHECK(a2 == 66);
      BOOST_CHECK(a3 == 122);
      BOOST_CHECK(a4 == 11);

      r = TransferElement::readAny({a1,a2,a3,a4});
      BOOST_CHECK(a2.getId() == r);
      BOOST_CHECK(a1 == 55);
      BOOST_CHECK(a2 == 22);
      BOOST_CHECK(a3 == 122);
      BOOST_CHECK(a4 == 11);

      r = TransferElement::readAny({a1,a2,a3,a4});
      BOOST_CHECK(a3.getId() == r);
      BOOST_CHECK(a1 == 55);
      BOOST_CHECK(a2 == 22);
      BOOST_CHECK(a3 == 33);
      BOOST_CHECK(a4 == 11);

      r = TransferElement::readAny({a1,a2,a3,a4});
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
void AsyncReadTest::testMixing() {

  for(auto &sdmToUse : sdmList) {
    std::cout << "testMixing: " << sdmToUse << std::endl;

    Device device;
    device.open(sdmToUse);
    auto backend = boost::dynamic_pointer_cast<AsyncDefaultImplTestDummy>(BackendFactory::getInstance().createBackend(sdmToUse));
    BOOST_CHECK( backend != NULL );

    // obtain register accessor with integral type
    auto accessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");

    // dummy register accessor for comparison
    DummyRegisterAccessor<int> dummy(backend.get(),"APP0","WORD_STATUS");

    // create the mutex for the register
    backend->readMutex[0x08].unlock();

    // start reading with readAsync but do not wait on the future - then perform normal read()
    TransferFuture future;
    dummy = 5;
    future = accessor.readAsync();
    future.getBoostFuture().wait();     // this makes sure the actual read is finished but does not affect DeviceAccess in any way
    BOOST_CHECK_EQUAL( (int)accessor, 0 );
    dummy = 6;
    accessor.read();
    BOOST_CHECK_EQUAL( (int)accessor, 5 );
    accessor.read();
    BOOST_CHECK_EQUAL( (int)accessor, 6 );

    // start reading with readAsync but do not wait on the future - then perform normal readNonBlocking()
    dummy = 8;
    future = accessor.readAsync();
    future.getBoostFuture().wait();     // this makes sure the actual read is finished but does not affect DeviceAccess in any way
    BOOST_CHECK_EQUAL( (int)accessor, 6 );
    dummy = 9;
    BOOST_CHECK( accessor.readNonBlocking() == true );
    BOOST_CHECK_EQUAL( (int)accessor, 8 );
    BOOST_CHECK( accessor.readNonBlocking() == true );
    BOOST_CHECK_EQUAL( (int)accessor, 9 );

    // start reading with readAsync but do not wait on the future - then perform normal readLatest()
    dummy = 10;
    future = accessor.readAsync();
    future.getBoostFuture().wait();     // this makes sure the actual read is finished but does not affect DeviceAccess in any way
    BOOST_CHECK_EQUAL( (int)accessor, 9 );
    BOOST_CHECK( accessor.readLatest() == true );
    BOOST_CHECK_EQUAL( (int)accessor, 10 );

    // start reading with readAsync but do not wait on the future - then perform normal readLatest()
    dummy = 11;
    future = accessor.readAsync();
    future.getBoostFuture().wait();     // this makes sure the actual read is finished but does not affect DeviceAccess in any way
    BOOST_CHECK_EQUAL( (int)accessor, 10 );
    dummy = 12;
    BOOST_CHECK( accessor.readLatest() == true );
    BOOST_CHECK_EQUAL( (int)accessor, 12 );

    device.close();
  }
}
