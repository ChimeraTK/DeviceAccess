#define BOOST_TEST_MODULE PredicatesTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "predicates.h"

BOOST_AUTO_TEST_SUITE(  PredicatesTestSuite )

BOOST_AUTO_TEST_CASE( test_ompareModuleName_pred){
  mtca4u::compareModuleName_pred myModulePredicate("MyModule");
  mtca4u::compareModuleName_pred theirModulePredicate("TheirModule");

  mtca4u::RegisterInfoMap::RegisterInfo myRegisterInfoent("REGISTER_1", 1, 0x0, 4, 0, 32, 0, true, 0, "MyModule");
  BOOST_CHECK( myModulePredicate( myRegisterInfoent ) == true );
  BOOST_CHECK( theirModulePredicate( myRegisterInfoent ) == false );
}

BOOST_AUTO_TEST_SUITE_END()
