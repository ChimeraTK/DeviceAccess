#include <algorithm>
#include <math.h>
#include <boost/test/included/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <Device.h>
#include <BufferingRegisterAccessor.h>

using namespace boost::unit_test_framework;

using namespace mtca4u;

/**********************************************************************************************************************/
class BufferingRegisterTest {
  public:
    BufferingRegisterTest(){
      device = boost::shared_ptr<Device>( new Device() );
      device->open("DUMMYD1");
    }

    /// test the register accessor
    void testRegisterAccessor();

    /// test the register accessor
    void testMuxedRegisterAccessor();

  private:
    boost::shared_ptr<Device> device;
    friend class BufferingRegisterTestSuite;


};

/**********************************************************************************************************************/
class  BufferingRegisterTestSuite : public test_suite {
  public:
    BufferingRegisterTestSuite() : test_suite("DummyRegister test suite") {
      boost::shared_ptr<BufferingRegisterTest> bufferingRegisterTest( new BufferingRegisterTest );

      add( BOOST_CLASS_TEST_CASE( &BufferingRegisterTest::testRegisterAccessor, bufferingRegisterTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "DummyRegister test suite";
  framework::master_test_suite().add(new BufferingRegisterTestSuite);

  return NULL;
}


/**********************************************************************************************************************/
void BufferingRegisterTest::testRegisterAccessor() {
  std::cout << "testRegisterAccessor" << std::endl;

  // obtain register accessor
  BufferingRegisterAccessor<int> intRegister = device->getBufferingRegisterAccessor<int>("APP0","MODULE0");

  // variable to read to for comparison
  int compare;

  // check number of elements getter
  BOOST_CHECK( intRegister.getNumberOfElements() == 2 );

  // test operator=
  intRegister = 3;
  intRegister.write();
  device->readReg("MODULE0","APP0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == 3 );

  // test implicit type conversion
  compare = 42;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), 0);
  intRegister.read();
  BOOST_CHECK( intRegister == 42 );

  // test operator[] on r.h.s.
  compare = 5;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), 0);
  compare = 77;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  intRegister.read();
  BOOST_CHECK( intRegister[0] == 5 );
  BOOST_CHECK( intRegister[1] == 77 );

  // test operator[] on l.h.s.
  intRegister[0] = 666;
  intRegister[1] = 999;
  intRegister.write();
  device->readReg("MODULE0","APP0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == 666 );
  device->readReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  BOOST_CHECK( compare == 999 );

  // close the device
  device->close();
}
