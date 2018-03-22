#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE GetSetAsCoockedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <Device.h>
//#include <AccessMode.h>
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE( testRawAccessor ){
  setDMapFilePath("dummies.dmap");

  Device d;
  d.open("PCIE1");

  auto scalarRawAccessor=d.getScalarRegisterAccessor<int32_t>("BOARD/WORD_USER", 0, {AccessMode::raw});
  scalarRawAccessor=25;
  // the register has 3 fractional bits
  BOOST_CHECK( std::fabs(scalarRawAccessor.getAsCoocked<double>() - 25./8) < 0.0001);

  scalarRawAccessor.setAsCoocked(31./8);
  BOOST_CHECK(scalarRawAccessor == 31);
}
