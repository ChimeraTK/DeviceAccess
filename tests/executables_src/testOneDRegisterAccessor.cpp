#include <algorithm>
#include <math.h>
#include <boost/test/included/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Device.h"
#include "OneDRegisterAccessor.h"
#include "DMapFileDefaults.h"
#include "BackendFactory.h"

#include "accessPrivateData.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

// we need to access the private implementation of the accessor (see accessPrivateData.h)
struct OneDRegisterAccessor_double_impl {
    typedef boost::shared_ptr< NDRegisterAccessor<double> >(NDRegisterAccessorBridge<double>::*type);
};
template class accessPrivateData::stow_private<OneDRegisterAccessor_double_impl, &mtca4u::NDRegisterAccessorBridge<double>::_impl>;

/**********************************************************************************************************************/
class OneDRegisterTest {
  public:
    OneDRegisterTest(){
      device = boost::shared_ptr<Device>( new Device() );
      device->open("DUMMYD1");
    }

    /// test the register accessor
    void testRegisterAccessor();

  private:
    boost::shared_ptr<Device> device;
    friend class OneDRegisterTestSuite;


};

/**********************************************************************************************************************/
class  OneDRegisterTestSuite : public test_suite {
  public:
    OneDRegisterTestSuite() : test_suite("OneDRegisterAccessor test suite") {
      BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
      boost::shared_ptr<OneDRegisterTest> bufferingRegisterTest( new OneDRegisterTest );

      add( BOOST_CLASS_TEST_CASE( &OneDRegisterTest::testRegisterAccessor, bufferingRegisterTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "DummyRegister test suite";
  framework::master_test_suite().add(new OneDRegisterTestSuite);

  return NULL;
}


/**********************************************************************************************************************/
void OneDRegisterTest::testRegisterAccessor() {
  std::cout << "testRegisterAccessor" << std::endl;

  // obtain register accessor with integral type
  OneDRegisterAccessor<int> intRegister = device->getOneDRegisterAccessor<int>("APP0/MODULE0");

  // variable to read to for comparison
  int compare;

  // check number of elements getter
  BOOST_CHECK( intRegister.getNElements() == 2 );

  // test operator[] on r.h.s.
  compare = 5;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), 0);
  compare = -77;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  intRegister.read();
  BOOST_CHECK( intRegister[0] == 5 );
  BOOST_CHECK( intRegister[1] == -77 );

  // test operator[] on l.h.s.
  intRegister[0] = -666;
  intRegister[1] = 999;
  intRegister.write();
  device->readReg("MODULE0","APP0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == -666 );
  device->readReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  BOOST_CHECK( compare == 999 );

  // test iterators with begin and end
  int ic = 0;
  for(OneDRegisterAccessor<int>::iterator it = intRegister.begin(); it != intRegister.end(); ++it) {
    *it = 1000*(ic+1);
    ic++;
  }
  intRegister.write();
  device->readReg("MODULE0","APP0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == 1000 );
  device->readReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  BOOST_CHECK( compare == 2000 );

  // test iterators with rbegin and rend
  ic = 0;
  for(OneDRegisterAccessor<int>::reverse_iterator it = intRegister.rbegin(); it != intRegister.rend(); ++it) {
    *it = 333*(ic+1);
    ic++;
  }
  intRegister.write();
  device->readReg("MODULE0","APP0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == 666 );
  device->readReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  BOOST_CHECK( compare == 333 );

  // test const iterators in both directions
  compare = 1234;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), 0);
  compare = 2468;
  device->writeReg("MODULE0","APP0", &compare, sizeof(int), sizeof(int));
  intRegister.read();
  const OneDRegisterAccessor<int> const_intRegister = intRegister;
  ic = 0;
  for(OneDRegisterAccessor<int>::const_iterator it = const_intRegister.begin(); it != const_intRegister.end(); ++it) {
    BOOST_CHECK( *it == 1234*(ic+1) );
    ic++;
  }
  ic = 0;
  for(OneDRegisterAccessor<int>::const_reverse_iterator it = const_intRegister.rbegin(); it != const_intRegister.rend(); ++it) {
    BOOST_CHECK( *it == 1234*(2-ic) );
    ic++;
  }

  // test swap with std::vector
  std::vector<int> x(2);
  x[0] = 11;
  x[1] = 22;
  intRegister.swap(x);
  BOOST_CHECK( x[0] == 1234 );
  BOOST_CHECK( x[1] == 2468 );
  BOOST_CHECK( intRegister[0] == 11 );
  BOOST_CHECK( intRegister[1] == 22 );

  // obtain register accessor with fractional type, to check if fixed-point conversion is working (3 fractional bits)
  OneDRegisterAccessor<double> floatRegister = device->getOneDRegisterAccessor<double>("MODULE0/WORD_USER1");

  // test operator[] on r.h.s.
  compare = -120;
  device->writeReg("WORD_USER1","MODULE0", &compare, sizeof(int), 0);
  floatRegister.read();
  BOOST_CHECK( floatRegister[0] == -120./8. );

  // test operator[] on l.h.s.
  floatRegister[0] = 42. / 8.;
  floatRegister.write();
  device->readReg("WORD_USER1","MODULE0", &compare, sizeof(int), 0);
  BOOST_CHECK( compare == 42 );

}
