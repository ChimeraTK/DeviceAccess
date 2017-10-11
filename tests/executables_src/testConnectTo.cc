/*
 * testConnectTo.cc
 *
 *  Created on: Oct 10, 2017
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testConnectTo

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>
#include <boost/thread.hpp>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "VariableGroup.h"
#include "ModuleGroup.h"
#include "VirtualModule.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Build first hierarchy */

struct FirstHierarchy : ctk::ModuleGroup { using ctk::ModuleGroup::ModuleGroup;

  struct TestModule : ctk::ApplicationModule { using ctk::ApplicationModule::ApplicationModule;

    struct VarGroup : ctk::VariableGroup { using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<int> varA{this, "varA", "MV/m", "Desc"};
      ctk::ScalarPushInput<int> varB{this, "varB", "MV/m", "Desc"};
      ctk::ScalarOutput<int> varC{this, "varC", "MV/m", "Desc"};
    } varGroup{this, "VarGroup", "A group"};

    ctk::ScalarPushInput<int> varA{this, "varA", "MV/m", "Desc"};
    ctk::ScalarOutput<int> varX{this, "varX", "MV/m", "Desc"};

    void mainLoop() {}
  } testModule{this, "TestModule", ""};

  struct SecondModule : ctk::ApplicationModule {
    SecondModule(EntityOwner *owner, const std::string &name, const std::string &description)
    : ctk::ApplicationModule(owner, name, description)
    {
      for(size_t i=0; i<22; ++i) myVec.emplace_back(this, "Var"+std::to_string(i), "Unit", "Foo");
    }
    SecondModule() { throw; } // work around for gcc bug: constructor must be present but is unused

    std::vector<ctk::ScalarPushInput<int>> myVec;

    void mainLoop() {}

  } secondModule{this, "SecondModule", ""};
};

/*********************************************************************************************************************/
/* Build second hierarchy */

struct SecondHierarchy : ctk::ModuleGroup { using ctk::ModuleGroup::ModuleGroup;

  struct TestModule : ctk::ApplicationModule { using ctk::ApplicationModule::ApplicationModule;

    struct VarGroup : ctk::VariableGroup { using ctk::VariableGroup::VariableGroup;
      ctk::ScalarOutput<int> varA{this, "varA", "MV/m", "Desc"};
      ctk::ScalarPushInput<int> varC{this, "varC", "MV/m", "Desc"};
      ctk::ScalarPushInput<int> varD{this, "varD", "MV/m", "Desc"};
    } varGroup{this, "VarGroup", "A group"};

    ctk::ScalarPushInput<int> extraVar{this, "extraVar", "MV/m", "Desc"};
    ctk::ScalarOutput<int> varA{this, "varA", "MV/m", "Desc"};

    struct EliminatedGroup : ctk::VariableGroup { using ctk::VariableGroup::VariableGroup;
      ctk::ScalarPushInput<int> varX{this, "varX", "MV/m", "Desc"};
      struct VarGroup : ctk::VariableGroup { using ctk::VariableGroup::VariableGroup;
        ctk::ScalarOutput<int> varB{this, "varB", "MV/m", "Desc"};
      } varGroup{this, "VarGroup", "This group shall be merged with testModule.varGroup in connectTo()"};
    } eliminatedGroup{this, "eliminatedGroup", "A group whose hierarchy gets eliminated", true};

    void mainLoop() {}
  } testModule{this, "TestModule", ""};

  struct SecondModule : ctk::ApplicationModule {
    SecondModule(EntityOwner *owner, const std::string &name, const std::string &description)
    : ctk::ApplicationModule(owner, name, description)
    {
      for(size_t i=0; i<22; ++i) myVec.emplace_back(this, "Var"+std::to_string(i), "Unit", "Foo");
    }
    SecondModule() { throw; } // work around for gcc bug: constructor must be present but is unused

    struct ExtraGroup : ctk::VariableGroup { using ctk::VariableGroup::VariableGroup;
      ctk::ScalarOutput<int> varA{this, "varA", "MV/m", "Desc"};
    } extraGroup{this, "ExtraGroup", "A group"};

    std::vector<ctk::ScalarOutput<int>> myVec;

    void mainLoop() {}

  } secondModule{this, "SecondModule", ""};
};

/*********************************************************************************************************************/
/* dummy application */

struct TestApplication : public ctk::Application {
    TestApplication() : Application("test suite") {  ChimeraTK::ExperimentalFeatures::enable(); }
    ~TestApplication() { shutdown(); }
    
    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests
    
    FirstHierarchy first{this, "first", "The test module"};
    SecondHierarchy second{this, "second", "The test module"};
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE( testConnectTo ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testConnectTo" << std::endl;
  
  TestApplication app;
 
  app.first.findTag(".*").dump();
  app.second.findTag(".*").dump();
  
  app.first.connectTo(app.second);
  
  app.initialise();
  app.run();
  
  /// @todo Test if connections are made properly!


}
