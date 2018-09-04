#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE PredicatesTest
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "predicates.h"
namespace ChimeraTK{
  using namespace ChimeraTK;
}

BOOST_AUTO_TEST_SUITE(  PredicatesTestSuite )

BOOST_AUTO_TEST_CASE( test_ompareModuleName_pred){
  ChimeraTK::compareModuleName_pred myModulePredicate("MyModule");
  ChimeraTK::compareModuleName_pred theirModulePredicate("TheirModule");

  ChimeraTK::RegisterInfoMap::RegisterInfo myRegisterInfoent("REGISTER_1", 1, 0x0, 4, 0, 32, 0, true, 0, "MyModule");
  BOOST_CHECK( myModulePredicate( myRegisterInfoent ) == true );
  BOOST_CHECK( theirModulePredicate( myRegisterInfoent ) == false );
}

BOOST_AUTO_TEST_SUITE_END()
