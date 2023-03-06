// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
#include "NumericAddress.h"
#include "RegisterPath.h"
namespace ChimeraTK {
  using namespace ChimeraTK;
}

using namespace boost::unit_test_framework;
using namespace ChimeraTK;
using ChimeraTK::numeric_address::BAR;

class RegisterPathTest {
 public:
  void testRegisterPath();
  void testNumericAddresses();
  void testComponents();
};

class RegisterPathTestSuite : public test_suite {
 public:
  RegisterPathTestSuite() : test_suite("RegisterPath class test suite") {
    boost::shared_ptr<RegisterPathTest> registerPathTest(new RegisterPathTest);

    add(BOOST_CLASS_TEST_CASE(&RegisterPathTest::testRegisterPath, registerPathTest));
    add(BOOST_CLASS_TEST_CASE(&RegisterPathTest::testNumericAddresses, registerPathTest));
    add(BOOST_CLASS_TEST_CASE(&RegisterPathTest::testComponents, registerPathTest));
  }
};

bool init_unit_test() {
  framework::master_test_suite().p_name.value = "LogicalNameMap class test suite";
  framework::master_test_suite().add(new RegisterPathTestSuite());

  return true;
}

void RegisterPathTest::testRegisterPath() {
  RegisterPath path1;
  RegisterPath path2("module1");
  RegisterPath path3("//module//blah/");
  RegisterPath path4("moduleX..Yblah./sub");
  BOOST_CHECK(path1 == "/");
  BOOST_CHECK(path1.length() == 1);
  BOOST_CHECK(path2 == "/module1");
  BOOST_CHECK(path2.length() == 8);
  BOOST_CHECK(path3 == "/module/blah");
  BOOST_CHECK(path3.length() == 12);

  BOOST_CHECK(path3.getWithAltSeparator() == "module/blah");
  path3.setAltSeparator(".");
  BOOST_CHECK(path3 == "/module/blah");
  BOOST_CHECK(path3.getWithAltSeparator() == "module.blah");
  path3.setAltSeparator("");

  BOOST_CHECK(path4 == "/moduleX..Yblah./sub");
  path4.setAltSeparator(".");
  BOOST_CHECK(path4 == "/moduleX/Yblah/sub");
  BOOST_CHECK(path4.getWithAltSeparator() == "moduleX.Yblah.sub");
  BOOST_CHECK((path4 / "next.register").getWithAltSeparator() == "moduleX.Yblah.sub.next.register");
  path4.setAltSeparator("/"); // this should clear the alternate separator as well
  BOOST_CHECK(path4 == "/moduleX..Yblah./sub");
  path4.setAltSeparator("");
  BOOST_CHECK(path4 == "/moduleX..Yblah./sub");

  BOOST_CHECK(path3 / "register" == "/module/blah/register");
  BOOST_CHECK("root" / path3 / "register" == "/root/module/blah/register");
  BOOST_CHECK("root/" + path3 + "register" == "root//module/blahregister");
  BOOST_CHECK("root" / path3 + "register" == "/root/module/blahregister");
  BOOST_CHECK("root" + path3 / "register" == "root/module/blah/register");
  BOOST_CHECK(path2 / path3 == "/module1/module/blah");

  path3 /= "test";
  BOOST_CHECK(path3 == "/module/blah/test");
  path3--;
  BOOST_CHECK(path3 == "/module/blah");
  --path3;
  BOOST_CHECK(path3 == "/blah");
  path3--;
  BOOST_CHECK(path3 == "/");
  --path2;
  BOOST_CHECK(path2 == "/");
}

void RegisterPathTest::testNumericAddresses() {
  RegisterPath path1("/SomeModule/withSomeRegister/");
  BOOST_CHECK(path1 == "/SomeModule/withSomeRegister");

  RegisterPath path2;

  path2 = path1 * 3;
  BOOST_CHECK(path2 == "/SomeModule/withSomeRegister*3");

  path2 = path1 / 3;
  BOOST_CHECK(path2 == "/SomeModule/withSomeRegister/3");

  BOOST_CHECK(BAR() == "/#");
  BOOST_CHECK(BAR() / 0 / 32 * 8 == "/#/0/32*8");
}

void RegisterPathTest::testComponents() {
  RegisterPath path1("/SomeModule/withSubModules/and/withSomeRegister/");
  std::vector<std::string> comps1 = path1.getComponents();
  BOOST_CHECK(comps1.size() == 4);
  BOOST_CHECK(comps1[0] == "SomeModule");
  BOOST_CHECK(comps1[1] == "withSubModules");
  BOOST_CHECK(comps1[2] == "and");
  BOOST_CHECK(comps1[3] == "withSomeRegister");

  RegisterPath path2("");
  std::vector<std::string> comps2 = path2.getComponents();
  BOOST_CHECK(comps2.size() == 0);

  RegisterPath path3("singleComponent");
  std::vector<std::string> comps3 = path3.getComponents();
  BOOST_CHECK(comps3.size() == 1);
  BOOST_CHECK(comps3[0] == "singleComponent");
}
