/*
 * testModules.cc - Test ApplicationModule, ModuleGroup and VariableGroup
 *
 *  Created on: Sep 27, 2017
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testModules

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include "ApplicationCore.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                   \
    {                                                                                                               \
      std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                  \
      while(!(condition)) {                                                                                         \
        bool timeout_reached = (std::chrono::steady_clock::now()-t0) > std::chrono::milliseconds(maxMilliseconds);  \
        BOOST_CHECK( !timeout_reached );                                                                            \
        if(timeout_reached) break;                                                                                  \
        usleep(1000);                                                                                               \
      }                                                                                                             \
    }


/*********************************************************************************************************************/
/* Variable group used in the modules */

struct SomeGroup : ctk::VariableGroup {
  using ctk::VariableGroup::VariableGroup;
  ctk::ScalarPushInput<std::string> inGroup{this, "inGroup", "", "This is a string", {"C", "D"}};
  ctk::ArrayPushInput<int64_t> alsoInGroup{this, "alsoInGroup", "justANumber", 16, "A 64 bit number array", {"C"}};
};

/*********************************************************************************************************************/
/* A plain application module for testing */

struct TestModule : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;

    ctk::ScalarPushInput<int> someInput{this, "nameOfSomeInput", "cm", "This is just some input for testing", {"A", "B"}};
    ctk::ScalarOutput<double> someOutput{this, "someOutput", "V", "Description", {"A", "C"}};

    SomeGroup someGroup{this, "someGroup", "Description of my test group"};

    struct AnotherGroup : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<uint8_t> foo{this, "foo", "counts", "Some counter", {"D"}};
    } anotherGroup{this, "anotherName", "Description of my other group"};

    void mainLoop() {
      while(true) {
        someInput.read();
        int val = someInput;
        someOutput = val;
        someOutput.write();
      }
    }
};

/*********************************************************************************************************************/
/* Simple application with just one module */

struct OneModuleApp : public ctk::Application {
    OneModuleApp() : Application("myApp") {}
    ~OneModuleApp() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests

    TestModule testModule{this, "testModule", "Module to test"};
};

/*********************************************************************************************************************/
/* Application with a vector of modules */

struct VectorOfModulesApp : public ctk::Application {
    VectorOfModulesApp(size_t nInstances)
    : Application("myApp"),
      _nInstances(nInstances)
    {}
    ~VectorOfModulesApp() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.

    void defineConnections() {
      for(size_t i = 0; i < _nInstances; ++i) {
        std::string name = "testModule_" + std::to_string(i) + "_instance";
        vectorOfTestModule.emplace_back(this, name, "Description");
      }
    }

    size_t _nInstances;
    std::vector<TestModule> vectorOfTestModule;
};

/*********************************************************************************************************************/
/* test module and variable ownerships */

BOOST_AUTO_TEST_CASE( test_ownership ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_ownership" << std::endl;
  
  OneModuleApp app;

  BOOST_CHECK( app.testModule.getOwner() == &app );  
  BOOST_CHECK( app.testModule.someGroup.getOwner() == &(app.testModule) );  
  BOOST_CHECK( app.testModule.anotherGroup.getOwner() == &(app.testModule) );  

  BOOST_CHECK( app.testModule.someInput.getOwner() == &(app.testModule) );  
  BOOST_CHECK( app.testModule.someOutput.getOwner() == &(app.testModule) );  

  BOOST_CHECK( app.testModule.someGroup.inGroup.getOwner() == &(app.testModule.someGroup) );  
  BOOST_CHECK( app.testModule.someGroup.alsoInGroup.getOwner() == &(app.testModule.someGroup) );  

  BOOST_CHECK( app.testModule.anotherGroup.foo.getOwner() == &(app.testModule.anotherGroup) );  

}

/*********************************************************************************************************************/
/* test getSubmoduleList() and getSubmoduleListRecursive() */

BOOST_AUTO_TEST_CASE( test_getSubmoduleList ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_getSubmoduleList" << std::endl;

  OneModuleApp app;

  {
    std::list<ctk::Module*> list = app.getSubmoduleList();
    BOOST_CHECK( list.size() == 1 );
    BOOST_CHECK( list.front() == &(app.testModule) );
  }
  
  {
    std::list<ctk::Module*> list = app.testModule.getSubmoduleList();
    BOOST_CHECK( list.size() == 2 );
    size_t foundSomeGroup = 0;
    size_t foundAnotherGroup = 0;
    for(auto mod : list) {
      if(mod == &(app.testModule.someGroup)) foundSomeGroup++;
      if(mod == &(app.testModule.anotherGroup)) foundAnotherGroup++;
    }
    BOOST_CHECK( foundSomeGroup == 1 );
    BOOST_CHECK( foundAnotherGroup == 1 );
  }
  
  {
    std::list<ctk::Module*> list = app.getSubmoduleListRecursive();
    BOOST_CHECK( list.size() == 3 );
    size_t foundTestModule = 0;
    size_t foundSomeGroup = 0;
    size_t foundAnotherGroup = 0;
    for(auto mod : list) {
      if(mod == &(app.testModule)) foundTestModule++;
      if(mod == &(app.testModule.someGroup)) foundSomeGroup++;
      if(mod == &(app.testModule.anotherGroup)) foundAnotherGroup++;
    }
    BOOST_CHECK( foundTestModule == 1 );
    BOOST_CHECK( foundSomeGroup == 1 );
    BOOST_CHECK( foundAnotherGroup == 1 );
  }
  
  {
    std::list<ctk::Module*> list = app.testModule.getSubmoduleListRecursive();    // identical to getSubmoduleList(), since no deeper hierarchies
    BOOST_CHECK( list.size() == 2 );
    size_t foundSomeGroup = 0;
    size_t foundAnotherGroup = 0;
    for(auto mod : list) {
      if(mod == &(app.testModule.someGroup)) foundSomeGroup++;
      if(mod == &(app.testModule.anotherGroup)) foundAnotherGroup++;
    }
    BOOST_CHECK( foundSomeGroup == 1 );
    BOOST_CHECK( foundAnotherGroup == 1 );
  }

}

/*********************************************************************************************************************/
/* test function call operator of the ApplicationModule */

BOOST_AUTO_TEST_CASE( testApplicatioModuleFnCallOp ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testApplicatioModuleFnCallOp" << std::endl;

  OneModuleApp app;

  BOOST_CHECK( app.testModule("nameOfSomeInput") == static_cast<ctk::VariableNetworkNode>(app.testModule.someInput) );
  BOOST_CHECK( app.testModule("nameOfSomeInput") != static_cast<ctk::VariableNetworkNode>(app.testModule.someOutput) );
  BOOST_CHECK( app.testModule("someOutput") == static_cast<ctk::VariableNetworkNode>(app.testModule.someOutput) );
  
  BOOST_CHECK( app.testModule("nameOfSomeInput").getType() == ctk::NodeType::Application );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getMode() == ctk::UpdateMode::push );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getDirection() == ctk::VariableDirection::consuming );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getValueType() == typeid(int) );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getName() == "nameOfSomeInput" );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getQualifiedName() == "/myApp/testModule/nameOfSomeInput" );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getUnit() == "cm" );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getDescription() == "This is just some input for testing" );
  BOOST_CHECK( app.testModule("nameOfSomeInput").getTags() == std::unordered_set<std::string>({"A", "B"}) );

}

/*********************************************************************************************************************/
/* test function call operator of the ApplicationModule */

BOOST_AUTO_TEST_CASE( testApplicatioModuleSubscriptOp ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testApplicatioModuleSubscriptOp" << std::endl;

  OneModuleApp app;

  BOOST_CHECK( &(app.testModule["someGroup"]) == &(app.testModule.someGroup) );
  BOOST_CHECK( &(app.testModule["anotherName"]) == &(app.testModule.anotherGroup) );

}

/*********************************************************************************************************************/
/* test correct behaviour when using a std::vector of ApplicatioModules */

BOOST_AUTO_TEST_CASE( testVectorOfApplicationModule ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testVectorOfApplicationModule" << std::endl;

  // create app with a vector containing 10 modules
  size_t nInstances = 10;
  VectorOfModulesApp app(nInstances);

  // the app creates the 10 module instances in defineConnections, check if this is done proplery (a quite redundant test...)
  BOOST_CHECK(app.vectorOfTestModule.size() == 0);
  app.defineConnections();
  BOOST_CHECK(app.vectorOfTestModule.size() == nInstances);

  // some direct checks on the created instances
  for(size_t i=0; i<nInstances; ++i) {
    std::string name = "testModule_" + std::to_string(i) + "_instance";
    BOOST_CHECK( app.vectorOfTestModule[i].getName() == name );
    auto node = static_cast<ctk::VariableNetworkNode>(app.vectorOfTestModule[i].someInput);
    BOOST_CHECK( node.getQualifiedName() == "/myApp/"+name+"/nameOfSomeInput" );
  }

  // check if instances appear properly in getSubmoduleList()
  {
    std::list<ctk::Module*> list = app.getSubmoduleList();
    BOOST_CHECK( list.size() == nInstances );
    std::map<size_t, size_t> instancesFound;
    for(size_t i = 0; i < nInstances; ++i) instancesFound[i] = 0;
    for(auto mod : list) {
      for(size_t i = 0; i < nInstances; ++i) {
        if(mod == &(app.vectorOfTestModule[i])) instancesFound[i]++;
      }
    }
    for(size_t i = 0; i < nInstances; ++i) {
      BOOST_CHECK( instancesFound[i] == 1 );
    }
  }

  // check if instances appear properly in getSubmoduleListRecursive() as well
  {
    std::list<ctk::Module*> list = app.getSubmoduleListRecursive();
    BOOST_CHECK( list.size() == 3*nInstances );
    std::map<size_t, size_t> instancesFound, instancesSomeGroupFound, instancesAnotherGroupFound;
    for(size_t i = 0; i < nInstances; ++i) {
      instancesFound[i] = 0;
      instancesSomeGroupFound[i] = 0;
      instancesAnotherGroupFound[i] = 0;
    }
    for(auto mod : list) {
      for(size_t i = 0; i < nInstances; ++i) {
        if(mod == &(app.vectorOfTestModule[i])) instancesFound[i]++;
        if(mod == &(app.vectorOfTestModule[i].someGroup)) instancesSomeGroupFound[i]++;
        if(mod == &(app.vectorOfTestModule[i].anotherGroup)) instancesAnotherGroupFound[i]++;
      }
    }
    for(size_t i = 0; i < nInstances; ++i) {
      BOOST_CHECK( instancesFound[i] == 1 );
      BOOST_CHECK( instancesSomeGroupFound[i] == 1 );
      BOOST_CHECK( instancesAnotherGroupFound[i] == 1 );
    }
  }
  
  // check ownerships
  for(size_t i = 0; i < nInstances; ++i) {
    BOOST_CHECK( app.vectorOfTestModule[i].getOwner() == &app );
    BOOST_CHECK( app.vectorOfTestModule[i].someInput.getOwner() == &(app.vectorOfTestModule[i]) );
    BOOST_CHECK( app.vectorOfTestModule[i].someOutput.getOwner() == &(app.vectorOfTestModule[i]) );
    BOOST_CHECK( app.vectorOfTestModule[i].someGroup.getOwner() == &(app.vectorOfTestModule[i]) );
    BOOST_CHECK( app.vectorOfTestModule[i].someGroup.inGroup.getOwner() == &(app.vectorOfTestModule[i].someGroup) );
    BOOST_CHECK( app.vectorOfTestModule[i].someGroup.alsoInGroup.getOwner() == &(app.vectorOfTestModule[i].someGroup) );
    BOOST_CHECK( app.vectorOfTestModule[i].anotherGroup.getOwner() == &(app.vectorOfTestModule[i]) );
    BOOST_CHECK( app.vectorOfTestModule[i].anotherGroup.foo.getOwner() == &(app.vectorOfTestModule[i].anotherGroup) );
  }

}
