/*
 * testRegisterPath.cpp
 *
 *  Created on: Mar 22, 2016
 *      Author: Martin Hierholzer
 */

#include <boost/test/included/unit_test.hpp>

#include "RegisterPath.h"
#include "NumericAddress.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;
using mtca4u::numeric_address::BAR;

class RegisterPathTest {
  public:
    void testRegisterPath();
    void testNumericAddresses();
};

class RegisterPathTestSuite : public test_suite {
  public:
    RegisterPathTestSuite() : test_suite("RegisterPath class test suite") {
      boost::shared_ptr<RegisterPathTest> registerPathTest(new RegisterPathTest);

      add( BOOST_CLASS_TEST_CASE(&RegisterPathTest::testRegisterPath, registerPathTest) );
      add( BOOST_CLASS_TEST_CASE(&RegisterPathTest::testNumericAddresses, registerPathTest) );
    }
};

test_suite* init_unit_test_suite(int /*argc*/, char * /*argv*/ []) {
  framework::master_test_suite().p_name.value = "LogicalNameMap class test suite";
  framework::master_test_suite().add(new RegisterPathTestSuite());

  return NULL;
}


void RegisterPathTest::testRegisterPath() {
  RegisterPath path1;
  RegisterPath path2("module1");
  RegisterPath path3("//module//blah/");
  RegisterPath path4("moduleX..Yblah/sub");
  BOOST_CHECK( path1 == "/" );
  BOOST_CHECK( path2 == "/module1" );
  BOOST_CHECK( path3 == "/module/blah" );
  BOOST_CHECK( path3.getWithAltSeparator() == "module.blah" );
  BOOST_CHECK( path4 == "/moduleX/Yblah/sub" );
  BOOST_CHECK( (path4/"next.register").getWithAltSeparator() == "moduleX.Yblah.sub.next.register" );
  BOOST_CHECK( path3/"register" == "/module/blah/register");
  BOOST_CHECK( "root"/path3/"register" == "/root/module/blah/register");
  BOOST_CHECK( "root/"+path3+"register" == "root//module/blahregister");
  BOOST_CHECK( "root"/path3+"register" == "/root/module/blahregister");
  BOOST_CHECK( "root"+path3/"register" == "root/module/blah/register");
  BOOST_CHECK( path2/path3 == "/module1/module/blah" );

  path3 /= "test";
  BOOST_CHECK( path3 == "/module/blah/test" );
  path3--;
  BOOST_CHECK( path3 == "/module/blah" );
  --path3;
  BOOST_CHECK( path3 == "/blah" );
  path3--;
  BOOST_CHECK( path3 == "/" );
  --path2;
  BOOST_CHECK( path2 == "/" );

}

void RegisterPathTest::testNumericAddresses() {
  RegisterPath path1("/SomeModule/withSomeRegister/");
  BOOST_CHECK( path1 == "/SomeModule/withSomeRegister" );

  RegisterPath path2;

  path2 = path1*3;
  BOOST_CHECK( path2 == "/SomeModule/withSomeRegister*3" );

  path2 = path1/3;
  BOOST_CHECK( path2 == "/SomeModule/withSomeRegister/3" );

  BOOST_CHECK( BAR == "/#" );
  BOOST_CHECK( BAR/0/32*8 == "/#/0/32*8" );
}
