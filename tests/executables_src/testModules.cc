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
using namespace boost::unit_test_framework;

#include <boost/mpl/list.hpp>

#include "ApplicationCore.h"

namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Variable group used in the modules */

struct SomeGroup : ctk::VariableGroup {
  using ctk::VariableGroup::VariableGroup;
  ctk::ScalarPushInput<std::string> inGroup{this, "inGroup", "", "This is a string", {"C", "A"}};
  ctk::ArrayPushInput<int64_t> alsoInGroup{this, "alsoInGroup", "justANumber", 16, "A 64 bit number array", {"A", "D"}};
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
/* An application module with a vector of a variable group*/

struct VectorModule : public ctk::ApplicationModule {
    VectorModule(ctk::EntityOwner *owner, const std::string &name, const std::string &description, size_t nInstances,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={})
    : ctk::ApplicationModule(owner, name, description, eliminateHierarchy, tags)
    {
      for(size_t i=0; i < nInstances; ++i) {
        std::string name = "testGroup_" + std::to_string(i);
        vectorOfSomeGroup.emplace_back(this, name, "Description 2");
      }
    }
    VectorModule() {}

    ctk::ScalarPushInput<int> someInput{this, "nameOfSomeInput", "cm", "This is just some input for testing", {"A", "B"}};
    ctk::ArrayOutput<double> someOutput{this, "someOutput", "V", 1, "Description", {"A", "C"}};

    std::vector<SomeGroup> vectorOfSomeGroup;

    struct AnotherGroup : ctk::VariableGroup {
      using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<uint8_t> foo{this, "foo", "counts", "Some counter", {"D"}};
    } anotherGroup{this, "anotherName", "Description of my other group"};

    void mainLoop() {
      while(true) {
        someInput.read();
        int val = someInput;
        someOutput[0] = val;
        someOutput.write();
      }
    }
};

/*********************************************************************************************************************/
/* An module group with a vector of a application moduoles */

struct VectorModuleGroup : public ctk::ModuleGroup {
    VectorModuleGroup(EntityOwner *owner, const std::string &name, const std::string &description, size_t nInstances,
            bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={})
    : ctk::ModuleGroup(owner, name, description, eliminateHierarchy, tags)
    {
      for(size_t i=0; i < nInstances; ++i) {
        std::string name = "test_" + std::to_string(i);
        vectorOfVectorModule.emplace_back(this, name, "Description 3", nInstances);
      }
    }
    
    VectorModuleGroup() {}
    
    std::vector<VectorModule> vectorOfVectorModule;
    
};

/*********************************************************************************************************************/
/* Application with a vector of module groups containing a vector of modules containing a vector of variable groups */

struct VectorOfEverythingApp : public ctk::Application {
    VectorOfEverythingApp(size_t nInstances)
    : Application("myApp"),
      _nInstances(nInstances)
    {}
    ~VectorOfEverythingApp() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.

    void defineConnections() {
      for(size_t i = 0; i < _nInstances; ++i) {
        std::string name = "testModule_" + std::to_string(i) + "_instance";
        vectorOfVectorModuleGroup.emplace_back(this, name, "Description", _nInstances);
      }
    }

    size_t _nInstances;
    std::vector<VectorModuleGroup> vectorOfVectorModuleGroup;
};

/*********************************************************************************************************************/
/* Application with various modules that get initialised only during defineConnections(). */

struct AssignModuleLaterApp : public ctk::Application {
    AssignModuleLaterApp()
    : Application("myApp")
    {}
    ~AssignModuleLaterApp() { shutdown(); }

    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.

    void defineConnections() {
      modGroupInstanceToAssignLater = VectorModuleGroup(this, "modGroupInstanceToAssignLater",
                                                "This instance of VectorModuleGroup was assigned using the operator=()", 42);
      modInstanceToAssignLater = VectorModule(this, "modInstanceToAssignLater",
                                                "This instance of VectorModule was assigned using the operator=()", 13);
    }

    VectorModuleGroup modGroupInstanceToAssignLater;
    VectorModule modInstanceToAssignLater;
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
/* test that modules cannot be owned by the wrong types */

BOOST_AUTO_TEST_CASE( test_badHierarchies ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_badHierarchies" << std::endl;
  
  // ******************************************
  // *** Tests for ApplicationModule
  
  // check app ApplicationModules cannot be owned by other app modules
  {
    OneModuleApp app;
    try {
      TestModule willFail(&(app.testModule), "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app ApplicationModules cannot be owned by variable groups
  {
    OneModuleApp app;
    try {
      TestModule willFail(&(app.testModule.someGroup), "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app ApplicationModules cannot be owned by nothing
  {
    OneModuleApp app;
    try {
      TestModule willFail(nullptr, "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // ******************************************
  // *** Tests for VariableGroup
  
  // check app VariableGroup cannot be owned by Applications
  {
    OneModuleApp app;
    try {
      SomeGroup willFail(&(app), "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app VariableGroup cannot be owned by ModuleGroups
  {
    VectorOfEverythingApp app(1);
    app.defineConnections();
    try {
      SomeGroup willFail(&(app.vectorOfVectorModuleGroup[0]), "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app VariableGroup cannot be owned by nothing
  {
    OneModuleApp app;
    try {
      SomeGroup willFail(nullptr, "willFail", "");
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // ******************************************
  // *** Tests for ModuleGroup
  
  // check app ModuleGroups cannot be owned by ApplicationModules
  {
    OneModuleApp app;
    try {
      VectorModuleGroup willFail(&(app.testModule), "willFail", "", 1);
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app ModuleGroups cannot be owned by VariableGroups
  {
    OneModuleApp app;
    try {
      VectorModuleGroup willFail(&(app.testModule.someGroup), "willFail", "", 1);
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }
  
  // check app ModuleGroups cannot be owned by nothing
  {
    OneModuleApp app;
    try {
      VectorModuleGroup willFail(nullptr, "willFail", "", 1);
      BOOST_FAIL("Exception expected");
    }
    catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>&) {
    }
  }

}

/*********************************************************************************************************************/
/* test that modules can be owned by the right types */

BOOST_AUTO_TEST_CASE( test_allowedHierarchies ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_allowedHierarchies" << std::endl;
  
  // ******************************************
  // *** Tests for ApplicationModule
  // check ApplicationModules can be owned by Applications
  {
    OneModuleApp app;
    TestModule shouldNotFail(&(app), "shouldNotFail", "");
  }

  // check ApplicationModules can be owned by ModuleGroups
  {
    VectorOfEverythingApp app(1);
    app.defineConnections();
    TestModule shouldNotFail(&(app.vectorOfVectorModuleGroup[0]), "shouldNotFail", "");
  }
  
  // ******************************************
  // *** Tests for VariableGroup
  
  // check VariableGroup can be owned by ApplicationModules
  {
    OneModuleApp app;
    SomeGroup shouldNotFail(&(app.testModule), "shouldNotFail", "");
  }

  // check VariableGroup can be owned by VariableGroup
  {
    OneModuleApp app;
    SomeGroup shouldNotFail(&(app.testModule.someGroup), "shouldNotFail", "");
  }
  
  // ******************************************
  // *** Tests for ModuleGroup
  
  // check ModuleGroup can be owned by Applications
  {
    OneModuleApp app;
    VectorModuleGroup shouldNotFail(&(app), "shouldNotFail", "", 1);
  }

  // check ModuleGroup can be owned by ModuleGroups
  {
    VectorOfEverythingApp app(1);
    app.defineConnections();
    VectorModuleGroup shouldNotFail(&(app.vectorOfVectorModuleGroup[0]), "shouldNotFail", "", 1);
  }
  
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
/* test getAccessorList() and getAccessorListRecursive() */

BOOST_AUTO_TEST_CASE( test_getAccessorList ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_getAccessorList" << std::endl;
  std::cout << std::endl;
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << " TODO -> IMPLEMENT THIS TEST!!!" << std::endl;
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << std::endl;
}

/*********************************************************************************************************************/
/* test function call operator of the ApplicationModule */

BOOST_AUTO_TEST_CASE( testApplicationModuleFnCallOp ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testApplicationModuleFnCallOp" << std::endl;

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

BOOST_AUTO_TEST_CASE( testApplicationModuleSubscriptOp ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testApplicationModuleSubscriptOp" << std::endl;

  OneModuleApp app;

  BOOST_CHECK( &(app.testModule["someGroup"]) == &(app.testModule.someGroup) );
  BOOST_CHECK( &(app.testModule["anotherName"]) == &(app.testModule.anotherGroup) );

}

/*********************************************************************************************************************/
/* test correct behaviour when using a std::vector of ApplicationModules */

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

    // check accessor list
    std::list<ctk::VariableNetworkNode> accList = app.vectorOfTestModule[i].getAccessorList();
    BOOST_CHECK( accList.size() == 2 );
    size_t foundSomeInput = 0;
    size_t foundSomeOutput = 0;
    for(auto &acc : accList) {
      if(acc == app.vectorOfTestModule[i].someInput) foundSomeInput++;
      if(acc == app.vectorOfTestModule[i].someOutput) foundSomeOutput++;
    }
    BOOST_CHECK( foundSomeInput == 1 );
    BOOST_CHECK( foundSomeOutput == 1 );

    // check submodule list
    std::list<ctk::Module*> modList = app.vectorOfTestModule[i].getSubmoduleList();
    BOOST_CHECK( modList.size() == 2 );
    size_t foundSomeGroup = 0;
    size_t foundAnotherGroup = 0;
    for(auto mod : modList) {
      if(mod == &(app.vectorOfTestModule[i].someGroup)) foundSomeGroup++;
      if(mod == &(app.vectorOfTestModule[i].anotherGroup)) foundAnotherGroup++;
    }
    BOOST_CHECK( foundSomeGroup == 1 );
    BOOST_CHECK( foundAnotherGroup == 1 );
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

/*********************************************************************************************************************/
/* test correct behaviour when using a std::vector of ModuleGroup, ApplicationModule and VariableGroup at the same time */

BOOST_AUTO_TEST_CASE( testVectorsOfAllModules ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testVectorsOfAllModules" << std::endl;

  // create app with a vector containing 10 modules
  size_t nInstances = 10;
  VectorOfEverythingApp app(nInstances);
  
  //-------------------------------------------------------------------------------------------------------------------
  // the app creates the 10 module instances in defineConnections, check if this is done proplery (a quite redundant
  // test...)
  BOOST_CHECK(app.vectorOfVectorModuleGroup.size() == 0);
  app.defineConnections();
  BOOST_CHECK(app.vectorOfVectorModuleGroup.size() == nInstances);
  for(size_t i=0; i < nInstances; ++i) {
    BOOST_CHECK(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule.size() == nInstances);
    for(size_t k=0; k < nInstances; ++k) {
      BOOST_CHECK(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup.size() == nInstances);
    }
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  // check presence in lists (getSubmoduleList() and getAccessorList())

  { // checks on first hierarchy level (application has the list of module groups)
    std::list<ctk::Module*> list = app.getSubmoduleList();
    BOOST_CHECK( list.size() == nInstances );
    std::map<size_t, size_t> found;
    for(size_t i=0; i < nInstances; ++i) found[i] = 0;
    for(auto mod : list) {
      for(size_t i=0; i < nInstances; ++i) {
        if(mod == &(app.vectorOfVectorModuleGroup[i])) found[i]++;
      }
    }
    for(size_t i=0; i < nInstances; ++i) BOOST_CHECK( found[i] == 1 );
    
  }

  { // checks on second hierarchy level (each module group has the list of modules)
    for(size_t i=0; i < nInstances; ++i) {
      std::list<ctk::Module*> list = app.vectorOfVectorModuleGroup[i].getSubmoduleList();
      BOOST_CHECK( list.size() == nInstances );

      std::map<size_t, size_t> found;
      for(size_t k=0; k < nInstances; ++k) found[k] = 0;
      for(auto mod : list) {
        for(size_t k=0; k < nInstances; ++k) {
          if(mod == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k])) found[k]++;
        }
      }
      for(size_t k=0; k < nInstances; ++k) BOOST_CHECK( found[k] == 1 );

    }
  }

  { // checks on third hierarchy level (each module has accessors and variable groups)
    for(size_t i=0; i < nInstances; ++i) {
      for(size_t k=0; k < nInstances; ++k) {
        
        // search for accessors
        std::list<ctk::VariableNetworkNode> accList = app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].getAccessorList();
        BOOST_CHECK_EQUAL( accList.size(), 2 );
        size_t someInputFound = 0;
        size_t someOutputFound = 0;
        for(auto acc : accList) {
          if(acc == app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someInput) someInputFound++;
          if(acc == app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someOutput) someOutputFound++;
        }
        BOOST_CHECK_EQUAL( someInputFound, 1 );
        BOOST_CHECK_EQUAL( someOutputFound, 1 );

        // search for variable groups
        std::list<ctk::Module*> modList = app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].getSubmoduleList();
        BOOST_CHECK_EQUAL( modList.size(), nInstances + 1 );

        std::map<size_t, size_t> someGroupFound;
        for(size_t m=0; m < nInstances; ++m) someGroupFound[m] = 0;
        size_t anotherGroupFound = 0;
        for(auto mod : modList) {
          for(size_t m=0; m < nInstances; ++m) {
            if(mod == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m])) someGroupFound[m]++;
          }
          if(mod == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].anotherGroup)) anotherGroupFound++;
        }
        for(size_t m=0; m < nInstances; ++m) {
          BOOST_CHECK_EQUAL( someGroupFound[m], 1 );
        }
        BOOST_CHECK_EQUAL( anotherGroupFound, 1 );
        
      }
    }
  }

  { // checks on fourth hierarchy level (each variable group has accessors)
    for(size_t i=0; i < nInstances; ++i) {
      for(size_t k=0; k < nInstances; ++k) {
        for(size_t m=0; m < nInstances; ++m) {

          // search for accessors
          std::list<ctk::VariableNetworkNode> accList =
                  app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].getAccessorList();
          BOOST_CHECK_EQUAL(accList.size(), 2);
          size_t inGroupFound = 0;
          size_t alsoInGroupFound = 0;
          for(auto acc : accList) {
            if(acc == app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].inGroup) {
              inGroupFound++;
            }
            if(acc == app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].alsoInGroup) {
              alsoInGroupFound++;
            }
          }
          BOOST_CHECK_EQUAL(inGroupFound, 1);
          BOOST_CHECK_EQUAL(alsoInGroupFound, 1);

          // make sure no further subgroups exist
          BOOST_CHECK_EQUAL(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].
                                vectorOfSomeGroup[m].getSubmoduleList().size(), 0);

        }
      }
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  // check ownerships
  for(size_t i = 0; i < nInstances; ++i) {
    BOOST_CHECK( app.vectorOfVectorModuleGroup[i].getOwner() == &app );
    for(size_t k = 0; k < nInstances; ++k) {
      BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].getOwner() == &(app.vectorOfVectorModuleGroup[i]) );
      BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someInput.getOwner()
                     == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k]) );
      BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someOutput.getOwner()
                     == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k]) );
      for(size_t m = 0; m < nInstances; ++m) {
        BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].getOwner()
                       == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k]) );
        BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].inGroup.getOwner()
                       == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m]) );
        BOOST_CHECK( app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].alsoInGroup.getOwner()
                       == &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m]) );
      }
    }
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  // check pointers to accessors in VariableNetworkNode
  for(size_t i = 0; i < nInstances; ++i) {
    for(size_t k = 0; k < nInstances; ++k) {
      {
        auto a = &(static_cast<ctk::VariableNetworkNode>(
                        app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someInput).getAppAccessorNoType());
        auto b = &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someInput);
        BOOST_CHECK( a == b );
      }
      {
        auto a = &(static_cast<ctk::VariableNetworkNode>(
                        app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someOutput).getAppAccessorNoType());
        auto b = &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].someOutput);
        BOOST_CHECK( a == b );
      }
      for(size_t m = 0; m < nInstances; ++m) {
        {
          auto a = &(static_cast<ctk::VariableNetworkNode>(
                          app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].inGroup).getAppAccessorNoType());
          auto b = &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].inGroup);
          BOOST_CHECK( a == b );
        }
        {
          auto a = &(static_cast<ctk::VariableNetworkNode>(
                          app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].alsoInGroup).getAppAccessorNoType());
          auto b = &(app.vectorOfVectorModuleGroup[i].vectorOfVectorModule[k].vectorOfSomeGroup[m].alsoInGroup);
          BOOST_CHECK( a == b );
        }
      }
    }
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  // search for tags and check result
  auto searchResult = app.findTag("A");
  std::list<ctk::Module*> list = searchResult.getSubmoduleList();
  
  { // checks on first hierarchy level
    BOOST_CHECK(list.size() == nInstances);
    std::map<std::string, int> nameMap;
    for(auto mod : list) {
      nameMap[mod->getName()]++;
    }
    for(size_t i=0; i < nInstances; ++i) {
      std::string name = "testModule_" + std::to_string(i) + "_instance";
      BOOST_CHECK(nameMap[name] == 1);
    }
  }

  { // checks on second hierarchy level
    for(auto mod : list) {
      std::list<ctk::Module*> list2 = mod->getSubmoduleList();

      BOOST_CHECK(list2.size() == nInstances);
      std::map<std::string, int> nameMap;
      for(auto mod2 : list2) {
        nameMap[mod2->getName()]++;
      }
      for(size_t i=0; i < nInstances; ++i) {
        std::string name = "test_" + std::to_string(i);
        BOOST_CHECK(nameMap[name] == 1);
      }
    }
  }

  { // checks on third hierarchy level
    for(auto mod : list) {
      std::list<ctk::Module*> list2 = mod->getSubmoduleList();
      for(auto mod2 : list2) {
        std::list<ctk::Module*> list3 = mod2->getSubmoduleList();

        BOOST_CHECK(list3.size() == nInstances);
        std::map<std::string, int> nameMap;
        for(auto mod3 : list3) {
          nameMap[mod3->getName()]++;
        }
        for(size_t i=0; i < nInstances; ++i) {
          std::string name = "testGroup_" + std::to_string(i);
          BOOST_CHECK(nameMap[name] == 1);
        }
      }
    }
  }

  { // checks on fourth hierarchy level (actual variables)
    for(auto mod : list) {
      std::list<ctk::Module*> list2 = mod->getSubmoduleList();
      for(auto mod2 : list2) {
        std::list<ctk::Module*> list3 = mod2->getSubmoduleList();
        for(auto mod3 : list3) {
          std::list<ctk::VariableNetworkNode> vars = mod3->getAccessorList();
          BOOST_CHECK(vars.size() == 2);

          size_t foundInGroup = 0;
          size_t foundAlsoInGroup = 0;
          for(auto var : vars) {
            if(var.getName() == "inGroup") ++foundInGroup;
            if(var.getName() == "alsoInGroup") ++foundAlsoInGroup;
          }
          BOOST_CHECK( foundInGroup == 1 );
          BOOST_CHECK( foundAlsoInGroup == 1 );
         
        }
      }
    }
  }

}

/*********************************************************************************************************************/
/* test late initialisation of modules using the assignment operator */

BOOST_AUTO_TEST_CASE( test_assignmentOperator ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> test_assignmentOperator" << std::endl;
  std::cout << std::endl;

  AssignModuleLaterApp app;

  BOOST_CHECK( app.getSubmoduleList().size() == 0 );
  BOOST_CHECK_EQUAL( app.modGroupInstanceToAssignLater.getSubmoduleList().size(), 0 );
  
  app.defineConnections();

  BOOST_CHECK( app.modGroupInstanceToAssignLater.getName() == "modGroupInstanceToAssignLater" );
  BOOST_CHECK( app.modGroupInstanceToAssignLater.getDescription() == "This instance of VectorModuleGroup was assigned using the operator=()" );

  BOOST_CHECK( app.modInstanceToAssignLater.getName() == "modInstanceToAssignLater" );
  BOOST_CHECK( app.modInstanceToAssignLater.getDescription() == "This instance of VectorModule was assigned using the operator=()" );
  
  auto list = app.getSubmoduleList();
  BOOST_CHECK( list.size() == 2 );
  
  bool modGroupInstanceToAssignLater_found = false;
  bool modInstanceToAssignLater_found = false;
  for(auto mod : list) {
    if(mod == &(app.modGroupInstanceToAssignLater)) modGroupInstanceToAssignLater_found = true;
    if(mod == &(app.modInstanceToAssignLater)) modInstanceToAssignLater_found = true;
  }
  
  BOOST_CHECK(modGroupInstanceToAssignLater_found);
  BOOST_CHECK(modInstanceToAssignLater_found);
  BOOST_CHECK_EQUAL( app.modGroupInstanceToAssignLater.getSubmoduleList().size(), 42 );
  BOOST_CHECK_EQUAL( app.modInstanceToAssignLater.getSubmoduleList().size(), 14 );
  
}
