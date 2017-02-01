#define ENABLE_EXPERIMENTAL_FEATURES

#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/test/included/unit_test.hpp>

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "DeviceAccessVersion.h"

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
      readMutex.lock();
      DummyBackend::read(bar,address,data,sizeInBytes);
      readMutex.unlock();
    }
    
    std::mutex readMutex;
};

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

};

/**********************************************************************************************************************/
class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  BackendFactory::getInstance().registerBackendType("AsyncTestDummy","",&AsyncTestDummy::createInstance, CHIMERATK_DEVICEACCESS_VERSION);
  
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

  // simple reading through readAsync without actual need
  TransferFuture future;
  dummy = 5;
  future = accessor.readAsync();
  future.wait();
  BOOST_CHECK( accessor == 5 );

  // check that future's wait() function won't return before the read is complete
  for(int i=0; i<5; ++i) {
    dummy = 42+i;
    backend->readMutex.lock();
    future = accessor.readAsync();
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] { future.wait(); flag = true; });
    usleep(100000);
    BOOST_CHECK(flag == false);
    backend->readMutex.unlock();
    thread.join();
    BOOST_CHECK( accessor == 42+i );
  }

  device.close();

}
