#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE GetSetAsCookedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <Device.h>
using namespace ChimeraTK;

BOOST_AUTO_TEST_CASE( testRawAccessor ){
  setDMapFilePath("dummies.dmap");

  Device d;
  d.open("DUMMYD3");

  auto scalarRawAccessor=d.getScalarRegisterAccessor<int32_t>("BOARD/WORD_USER", 0, {AccessMode::raw});
  scalarRawAccessor=25;
  // the register has 3 fractional bits
  BOOST_CHECK( std::fabs(scalarRawAccessor.getAsCooked<double>() - 25./8) < 0.0001);

  scalarRawAccessor.setAsCooked(31./8);
  BOOST_CHECK_EQUAL(int32_t(scalarRawAccessor), 31);

  auto oneDRawAccessor=d.getOneDRegisterAccessor<int32_t>("ADC/AREA_DMAABLE_FIXEDPOINT16_3", 0, 0, {AccessMode::raw});

  oneDRawAccessor[0]=12;
  oneDRawAccessor[1]=13;

   // the register has 3 fractional bits
  BOOST_CHECK( std::fabs(oneDRawAccessor.getAsCooked<double>(0) - 12./8) < 0.0001);
  BOOST_CHECK( std::fabs(oneDRawAccessor.getAsCooked<double>(1) - 13./8) < 0.0001);

  oneDRawAccessor.setAsCooked(0,42./8);
  oneDRawAccessor.setAsCooked(1,43./8);

  BOOST_CHECK_EQUAL(oneDRawAccessor[0], 42);
  BOOST_CHECK_EQUAL(oneDRawAccessor[1], 43);

}
