#include <algorithm>
#include <math.h>
#include <boost/test/included/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "Device.h"
#include "DummyRegisterAccessor.h"
#include "DummyBackend.h"
#include "ScalarRegisterAccessor.h"

#include "accessPrivateData.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

/**********************************************************************************************************************/
class ScalarRegisterTest {
  public:

    /// test creation of the accessor incl. exceptions
    void testCreation();

    /// test the register accessor with int type
    void testIntRegisterAccessor();

    /// test the register accessor with float type and fixed point conversion
    void testFloatRegisterAccessor();

    /// test the scalar accessor as one value in a larger register
    void testWordOffset();

};

/**********************************************************************************************************************/
class  ScalarRegisterTestSuite : public test_suite {
  public:
    ScalarRegisterTestSuite() : test_suite("ScalarRegisterAccessor test suite") {
      BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
      boost::shared_ptr<ScalarRegisterTest> scalarRegisterTest( new ScalarRegisterTest );

      add( BOOST_CLASS_TEST_CASE( &ScalarRegisterTest::testCreation, scalarRegisterTest ) );
      add( BOOST_CLASS_TEST_CASE( &ScalarRegisterTest::testIntRegisterAccessor, scalarRegisterTest ) );
      add( BOOST_CLASS_TEST_CASE( &ScalarRegisterTest::testFloatRegisterAccessor, scalarRegisterTest ) );
      add( BOOST_CLASS_TEST_CASE( &ScalarRegisterTest::testWordOffset, scalarRegisterTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "ScalarRegisterAccessor test suite";
  framework::master_test_suite().add(new ScalarRegisterTestSuite);

  return NULL;
}

/**********************************************************************************************************************/
void ScalarRegisterTest::testCreation() {
  std::cout << "testCreation" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr< DummyBackend > backend = boost::dynamic_pointer_cast<DummyBackend>( BackendFactory::getInstance().createBackend("DUMMYD2") );
  BOOST_CHECK( backend != NULL );

  // obtain register accessor in disconnected state
  ScalarRegisterAccessor<int> intRegisterDisconnected;
  BOOST_CHECK( intRegisterDisconnected.isInitialised() == false );
  intRegisterDisconnected.replace(device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS"));
  BOOST_CHECK( intRegisterDisconnected.isInitialised() == true );

  // obtain register accessor with integral type
  ScalarRegisterAccessor<int> intRegister = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");
  BOOST_CHECK( intRegister.isInitialised() == true );

  device.close();

}

/**********************************************************************************************************************/
void ScalarRegisterTest::testIntRegisterAccessor() {
  std::cout << "testRegisterAccessor" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr< DummyBackend > backend = boost::dynamic_pointer_cast<DummyBackend>( BackendFactory::getInstance().createBackend("DUMMYD2") );
  BOOST_CHECK( backend != NULL );

  // obtain register accessor with integral type
  ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");

  // dummy register accessor for comparison
  DummyRegisterAccessor<int> dummy(backend.get(),"APP0","WORD_STATUS");

    // test type conversion etc. for reading
  dummy = 5;
  accessor.read();
  BOOST_CHECK( accessor == 5 );
  BOOST_CHECK( int(accessor) == 5 );
  BOOST_CHECK( 2*accessor == 10 );
  BOOST_CHECK( accessor+2 == 7 );
  dummy = -654;
  BOOST_CHECK( accessor == 5 );
  accessor.read();
  BOOST_CHECK( accessor == -654 );

  // test assignment etc. for writing
  accessor = -666;
  accessor.write();
  BOOST_CHECK( dummy == -666 );
  accessor = 222;
  accessor.write();
  BOOST_CHECK( dummy == 222 );


  // test pre-increment operator
  ScalarRegisterAccessor<int> copy = ++accessor;

  BOOST_CHECK( accessor == 223 );
  BOOST_CHECK( copy == 223 );
  BOOST_CHECK( dummy == 222 );
  accessor.write();
  BOOST_CHECK( dummy == 223 );
  copy = 3;
  BOOST_CHECK( accessor == 3 );
  copy.write();
  BOOST_CHECK( dummy == 3 );

  // test pre-decrement operator
  copy.replace(--accessor);

  BOOST_CHECK( accessor == 2 );
  BOOST_CHECK( copy == 2 );
  BOOST_CHECK( dummy == 3 );
  accessor.write();
  BOOST_CHECK( dummy == 2 );
  copy = 42;
  BOOST_CHECK( accessor == 42 );
  copy.write();
  BOOST_CHECK( dummy == 42 );

  // test post-increment operator
  int oldValue = accessor++;

  BOOST_CHECK( accessor == 43 );
  BOOST_CHECK( copy == 43 );
  BOOST_CHECK( oldValue == 42 );
  BOOST_CHECK( dummy == 42 );
  accessor.write();
  BOOST_CHECK( dummy == 43 );

  // test post-decrement operator
  accessor = 120;
  oldValue = accessor--;

  BOOST_CHECK( accessor == 119 );
  BOOST_CHECK( copy == 119 );
  BOOST_CHECK( oldValue == 120 );
  BOOST_CHECK( dummy == 43 );
  accessor.write();
  BOOST_CHECK( dummy == 119 );

  device.close();

}

/**********************************************************************************************************************/
void ScalarRegisterTest::testFloatRegisterAccessor() {
  std::cout << "testFloatRegisterAccessor" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr< DummyBackend > backend = boost::dynamic_pointer_cast<DummyBackend>( BackendFactory::getInstance().createBackend("DUMMYD2") );
  BOOST_CHECK( backend != NULL );

  // obtain register accessor with integral type
  ScalarRegisterAccessor<float> accessor = device.getScalarRegisterAccessor<float>("MODULE1/WORD_USER2");

  // dummy register accessor for comparison
  DummyRegisterAccessor<float> dummy(backend.get(),"MODULE1","WORD_USER2");

    // test type conversion etc. for reading
  dummy = 5.3;
  float requiredVal = dummy;
  BOOST_CHECK_CLOSE( requiredVal, 5.3, 1 );

  accessor.read();
  float val = accessor;         // BOOST_CHECK_CLOSE requires implicit conversion in both directions, so we must help us here
  BOOST_CHECK_CLOSE( val, requiredVal, 0.01 );
  BOOST_CHECK_CLOSE( float(accessor), requiredVal, 0.01 );
  BOOST_CHECK_CLOSE( 2.*accessor, 2*requiredVal, 0.01 );
  BOOST_CHECK_CLOSE( accessor+2, 2+requiredVal, 0.01 );
  dummy = -10;
  BOOST_CHECK_CLOSE( float(accessor), requiredVal, 0.01 );
  accessor.read();
  BOOST_CHECK_CLOSE( float(accessor), 0, 0.01 );

  // test assignment etc. for writing
  accessor = -4;
  accessor.write();
  BOOST_CHECK_CLOSE( float(dummy), 0, 0.01 );
  accessor = 10.3125;
  accessor.write();
  BOOST_CHECK_CLOSE( float(dummy), 10.3125, 0.01 );

  device.close();

}
/// test the scalar accessor as one value in a larger register
void ScalarRegisterTest::testWordOffset(){
  std::cout << "testWordOffset" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr< DummyBackend > backend = boost::dynamic_pointer_cast<DummyBackend>( BackendFactory::getInstance().createBackend("DUMMYD2") );
  BOOST_CHECK( backend != NULL );

  // The second entry in module 1 is WORD_USER2
  DummyRegisterAccessor<float> dummy(backend.get(),"MODULE1","WORD_USER2");
  dummy=3.5;

  // obtain register accessor with integral type. We use and offset of 1 (second word in module1), and raw  mode to check that argument passing works
  ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>("APP0/MODULE1", 1, true);
  accessor.read();
  BOOST_CHECK(accessor==static_cast<int>(3.5 * (1<<5)) ); // 5 fractional bits, float value 3.5

  // Just to be safe that we don't accidentally have another register with content 112: modify it
  ++accessor;
  accessor.write();
  BOOST_CHECK(dummy == 3.53125);

  device.close();
}
