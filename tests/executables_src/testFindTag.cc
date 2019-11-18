/*
 * testFindTag.cc
 *
 *  Created on: Feb 27, 2019
 *      Author: Martin Hierholzer
 */

#include <chrono>
#include <future>

#define BOOST_TEST_MODULE testFindTag

#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread.hpp>

#include "ApplicationCore.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Build hierarchy */

struct FirstHierarchy : ctk::ModuleGroup {
  using ctk::ModuleGroup::ModuleGroup;

  struct TestModule : ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    struct VarGroup : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<int> varA{this, "varA", "MV/m", "Desc"};
      ctk::ScalarPushInput<double> varB{this, "varB", "MV/m", "Desc"};
      ctk::ScalarOutput<int> varC{this, "varC", "MV/m", "Desc"};
      struct : ctk::VariableGroup {
        using ctk::VariableGroup::VariableGroup;
        ctk::ScalarPushInput<double> nieceOfVarGroup{this, "nieceOfVarGroup", "MV/m", "Desc"};
        struct : ctk::VariableGroup {
          using ctk::VariableGroup::VariableGroup;
          ctk::ScalarOutput<int> varC{this, "childOfNieceGroup", "MV/m", "Desc"};
        } moveMeAlong{this, "NieceGroup", ""};
      } movedUp{this, "SisterGroupOfVarGroup", "minus one test 1", ctk::HierarchyModifier::oneLevelUp, {"Partial"}};
      struct : ctk::VariableGroup {
        using ctk::VariableGroup::VariableGroup;
        ctk::ScalarPushInput<double> sisterVarOfVarGroup{this, "sisterVarOfVarGroup", "MV/m", "Desc"};
        struct : ctk::VariableGroup {
          using ctk::VariableGroup::VariableGroup;
          ctk::ScalarOutput<int> varC{this, "anotherNieceVar", "MV/m", "Desc"};
        } moveMeAlong{this, "AnotherSisterGroup", ""};
      } movedUpAndHidden{this, "YouLNeverSee", "minus one test 2", ctk::HierarchyModifier::oneUpAndHide, {"Partial"}};
    } varGroup{this, "VarGroup", "A group", ctk::HierarchyModifier::none, {"Exclude", "Partial"}};

    struct MoveToRoot : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<uint8_t> varX{this, "varX", "MV/m", "Desc"};
      ctk::ScalarPushInput<int16_t> varY{this, "varY", "MV/m", "Desc"};
      ctk::ScalarOutput<uint16_t> varZ{this, "varZ", "MV/m", "Desc"};
    } moveToRoot{this, "MoveMeToRoot", "Group moved to /", ctk::HierarchyModifier::moveToRoot, {"Partial"}};

    ctk::ScalarPushInput<std::string> varA{this, "varA", "MV/m", "Desc", {"Partial"}};
    ctk::ScalarOutput<float> varX{this, "varX", "MV/m", "Desc"};

    void mainLoop() {}
  } testModule{this, "TestModule", ""};

  struct SecondModule : ctk::ApplicationModule {
    SecondModule(EntityOwner* owner, const std::string& name, const std::string& description)
    : ctk::ApplicationModule(owner, name, description) {
      for(size_t i = 0; i < 22; ++i) myVec.emplace_back(this, "Var" + std::to_string(i), "Unit", "Foo");
    }
    SecondModule() { throw; } // work around for gcc bug: constructor must be present but is unused

    std::vector<ctk::ScalarPushInput<uint64_t>> myVec;

    void mainLoop() {}

  } secondModule{this, "SecondModule", ""};
};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testSuite") {}
  ~TestApplication() { shutdown(); }
  void defineConnections() {} // the setup is done in the tests

  FirstHierarchy first{this, "first", "The test module", ctk::HierarchyModifier::none, {"Everything"}};
  ctk::ControlSystemModule cs;
};

/*********************************************************************************************************************/
/* test tag on everything */

BOOST_AUTO_TEST_CASE(testEverythingTag) {
  std::cout << "*****************************************************************************************" << std::endl;
  std::cout << "==> testEverythingTag" << std::endl;

  TestApplication app;
  app.findTag("Everything").connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();
  app.cs.dump();

  // check if all variables are found on the ControlSystem - read/write dummy values as a consistency check. We have
  // different types and input/output mixed, so mixing up variables will be noticed.
  test.writeScalar<int>("/first/TestModule/VarGroup/varA", 42);
  test.writeScalar<double>("/first/TestModule/VarGroup/varB", 3.14);
  test.readScalar<int>("/first/TestModule/VarGroup/varC");
  test.writeScalar<double>("/first/TestModule/SisterGroupOfVarGroup/nieceOfVarGroup", 9.9);
  test.readScalar<int>("/first/TestModule/SisterGroupOfVarGroup/NieceGroup/childOfNieceGroup");
  test.writeScalar<double>("/first/TestModule/sisterVarOfVarGroup", -9.9);
  test.readScalar<int>("/first/TestModule/AnotherSisterGroup/anotherNieceVar");
  test.writeScalar<std::string>("/first/TestModule/varA", "Hallo123");
  test.readScalar<float>("/first/TestModule/varX");
  for(size_t i = 0; i < 22; ++i) {
    test.writeScalar<uint64_t>("/first/SecondModule/Var" + std::to_string(i), i);
  }
  test.writeScalar<uint8_t>("/MoveMeToRoot/varX", 0);
  test.writeScalar<int16_t>("/MoveMeToRoot/varY", 0);
  test.readScalar<uint16_t>("/MoveMeToRoot/varZ");
}

/*********************************************************************************************************************/
/* test searching for a tag which is applied to only some variables */

BOOST_AUTO_TEST_CASE(testPartialTag) {
  std::cout << "*****************************************************************************************" << std::endl;
  std::cout << "==> testPartialTag" << std::endl;

  TestApplication app;
  app.findTag("Partial").connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();

  // check if all variables are found on the ControlSystem - read/write dummy values as a consistency check. We have
  // different types and input/output mixed, so mixing up variables will be noticed.
  test.writeScalar<int>("/first/TestModule/VarGroup/varA", 42);
  test.writeScalar<double>("/first/TestModule/VarGroup/varB", 3.14);
  test.readScalar<int>("/first/TestModule/VarGroup/varC");
  test.writeScalar<std::string>("/first/TestModule/varA", "Hallo123");
  test.writeScalar<uint8_t>("/MoveMeToRoot/varX", 0);
  test.writeScalar<int16_t>("/MoveMeToRoot/varY", 0);
  test.readScalar<uint16_t>("/MoveMeToRoot/varZ");
  // the rest is not part of our search result
  BOOST_CHECK_THROW(test.readScalar<float>("/first/TestModule/varX"), ChimeraTK::logic_error);
  for(size_t i = 0; i < 22; ++i) {
    BOOST_CHECK_THROW(
        test.writeScalar<uint64_t>("/first/SecondModule/Var" + std::to_string(i), i), ChimeraTK::logic_error);
  }
}

/*********************************************************************************************************************/
/* test searching for a tag and excluding another */

BOOST_AUTO_TEST_CASE(testExcludeTag) {
  std::cout << "*****************************************************************************************" << std::endl;
  std::cout << "==> testExcludeTag" << std::endl;

  TestApplication app;
  app.findTag("Partial").excludeTag("Exclude").connectTo(app.cs);
  ctk::TestFacility test;
  test.runApplication();

  // check if all variables are found on the ControlSystem - read/write dummy values as a consistency check. We have
  // different types and input/output mixed, so mixing up variables will be noticed.
  test.writeScalar<std::string>("/first/TestModule/varA", "Hallo123");
  test.writeScalar<uint8_t>("/MoveMeToRoot/varX", 0);
  test.writeScalar<int16_t>("/MoveMeToRoot/varY", 0);
  test.readScalar<uint16_t>("/MoveMeToRoot/varZ");
  // the rest is not part of our search result
  BOOST_CHECK_THROW(test.writeScalar<int>("/first/TestModule/VarGroup/varA", 42), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(test.writeScalar<double>("/first/TestModule/VarGroup/varB", 3.14), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(test.readScalar<int>("/first/TestModule/VarGroup/varC"), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(test.readScalar<float>("/first/TestModule/varX"), ChimeraTK::logic_error);
  for(size_t i = 0; i < 22; ++i) {
    BOOST_CHECK_THROW(
        test.writeScalar<uint64_t>("/first/SecondModule/Var" + std::to_string(i), i), ChimeraTK::logic_error);
  }
}
