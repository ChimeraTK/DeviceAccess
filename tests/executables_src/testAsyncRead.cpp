#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/test/included/unit_test.hpp>

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "DeviceAccessVersion.h"
#include "ExperimentalFeatures.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

constexpr char dummySdm[] = "sdm://./AsyncTestDummy=goodMapFile.map";

/**********************************************************************************************************************/

class AsyncTestDummy : public DummyBackend {
  public:
    AsyncTestDummy(std::string mapFileName) : DummyBackend(mapFileName) {}

    static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::string, std::list<std::string> parameters, std::string) {
      return boost::shared_ptr<DeviceBackend>(new AsyncTestDummy(parameters.front()));
    }
    
    void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) override {
      std::cout << "BEGIN read " << address << std::endl;
      readMutex.at(address).lock();
      DummyBackend::read(bar,address,data,sizeInBytes);
      readMutex.at(address).unlock();
      std::cout << "END read " << address << std::endl;
    }
    
    std::map<int, std::mutex> readMutex;
};

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

    /// test the TransferElement::readAny() function
    void testReadAny();

};

/**********************************************************************************************************************/
class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAny, asyncReadTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  BackendFactory::getInstance().registerBackendType("AsyncTestDummy","",&AsyncTestDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
  ChimeraTK::ExperimentalFeatures::enable();
  
  framework::master_test_suite().p_name.value = "Async read test suite";
  framework::master_test_suite().add(new AsyncReadTestSuite);

  return NULL;
}

/**********************************************************************************************************************/
void AsyncReadTest::testAsyncRead() {
  std::cout << "testAsyncRead" << std::endl;

  Device device;
  device.open(dummySdm);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(dummySdm));
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

/**********************************************************************************************************************/

void AsyncReadTest::testReadAny() {
  std::cout << "testReadAny" << std::endl;

  Device device;
  device.open(dummySdm);
  auto backend = boost::dynamic_pointer_cast<AsyncTestDummy>(BackendFactory::getInstance().createBackend(dummySdm));
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
    std::thread thread([&backend, &a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
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

  device.close();
}
