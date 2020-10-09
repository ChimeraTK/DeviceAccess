
#define BOOST_TEST_MODULE testVirtualHierarchy
#include <boost/test/included/unit_test.hpp>

#include "Application.h"
#include "ApplicationModule.h"
#include "ModuleGroup.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;

namespace ctk = ChimeraTK;

struct TestModule : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int> input{this, "input", "", {"CS"}};
  ctk::ScalarOutput<int> output{this, "output", "", {"CS"}};

  void mainLoop() override {}
};

struct TestModule2 : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int> input2{this, "input2", "", {"CS"}};
  ctk::ScalarOutput<int> output2{this, "output2", "", {"CS"}};

  void mainLoop() override {}
};

struct TestModule3 : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int> input3{this, "input3", "", {"CS"}};
  ctk::ScalarOutput<int> output3{this, "output3", "", {"CS"}};

  void mainLoop() override {}
};

// Typical connection scenario: One module provides inputs and outputs, and another wants to use them.
// Hierarcy modifiers are used to move the variable group in the "using" module to the same variable with the same name in the virtual space.
// Notice: For simplicity we  do not send initial values and have a circular dependency. This code never reaches the main loops.
// We have to test two different scenarios: The module hierarchy modifyer is placed before and after the module with modifier.
struct TestModuleWithVariableGroups : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  struct /*GroupOneLevelUp*/ : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarOutput<int> var1InGroupOneLevelUp{this, "var1InGroupOneLevelUp", "", ""};
    ctk::ScalarPushInput<int> var2InGroupOneLevelUp{this, "var2InGroupOneLevelUp", "", ""};
  } groupOneLevelUp{this, "InnerModuleWithVariableGroups2", "", ctk::HierarchyModifier::oneLevelUp};

  ctk::ScalarOutput<int> var3InGroupOneLevelUp{this, "var3InGroupOneLevelUp", "", ""};
  ctk::ScalarPushInput<int> var4InGroupOneLevelUp{this, "var4InGroupOneLevelUp", "", ""};

  // directly connect variales in the virtual level of the module group (always one up and hide in both modules)
  struct /*GroupOneUpAndHide*/ : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarOutput<int> varInGroupOneUpAndHide{this, "var1InGroupOneUpAndHide", "", ""};
    ctk::ScalarPushInput<int> var2InGroupOneUpAndHide{this, "var2InGroupOneUpAndHide", "", ""};
  } groupOneUpAndHide{this, "YoullNeverSee", "", ctk::HierarchyModifier::oneUpAndHide};

  void mainLoop() override {}
};

struct TestModuleWithVariableGroups2 : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;

  ctk::ScalarPushInput<int> var1InGroupOneLevelUp{this, "var1InGroupOneLevelUp", "", ""};
  ctk::ScalarOutput<int> var2InGroupOneLevelUp{this, "var2InGroupOneLevelUp", "", ""};

  struct /*GroupOneLevelUp*/ : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int> var3InGroupOneLevelUp{this, "var3InGroupOneLevelUp", "", ""};
    ctk::ScalarOutput<int> var4InGroupOneLevelUp{this, "var4InGroupOneLevelUp", "", ""};
  } groupOneLevelUp{this, "InnerModuleWithVariableGroups1", "", ctk::HierarchyModifier::oneLevelUp};

  struct /*GroupOneUpAndHide*/ : public ctk::VariableGroup {
    using ctk::VariableGroup::VariableGroup;
    ctk::ScalarPushInput<int> varInGroupOneUpAndHide{this, "var1InGroupOneUpAndHide", "", ""};
    ctk::ScalarOutput<int> var2InGroupOneUpAndHide{this, "var2InGroupOneUpAndHide", "", ""};
  } groupOneUpAndHide{this, "IntentionallyNotYoullNeverSee", "", ctk::HierarchyModifier::oneUpAndHide};

  void mainLoop() override {}
};

struct InnerGroup : public ctk::ModuleGroup {
  using ctk::ModuleGroup::ModuleGroup;

  TestModule innerModule{this, "innerModule", "", ctk::HierarchyModifier::none};
  TestModule2 innerModuleOneUpAndHide{this, "innerModuleOneUpAndHide", "", ctk::HierarchyModifier::oneUpAndHide};
  TestModule3 innerModuleMoveToRoot{this, "innerModuleMoveToRoot", "", ctk::HierarchyModifier::moveToRoot};
  TestModule3 innerModuleSameNameAsGroup{this, "innerModuleGroup", "", ctk::HierarchyModifier::oneLevelUp};
  TestModuleWithVariableGroups innerModuleWithVariableGroups{this, "InnerModuleWithVariableGroups1", ""};
  TestModuleWithVariableGroups2 innerModuleWithVariableGroups2{this, "InnerModuleWithVariableGroups2", ""};
};

struct OuterGroup : public ctk::ModuleGroup {
  OuterGroup(EntityOwner* owner, const std::string& name, const std::string& description,
      ctk::HierarchyModifier modifier, ctk::HierarchyModifier innerGroupModifier = ctk::HierarchyModifier::none)
  : ModuleGroup{owner, name, description, modifier}, innerGroup{this, "innerModuleGroup", "", innerGroupModifier} {
    // Here, findTag should give proper exceptions if HierarchyModifiers are used illegally
    auto allAccessors = getOwner()->findTag(".*").getAccessorListRecursive();

    //    for(auto acc : allAccessors) {
    //      std::cout << "      -- Accessor: " << acc.getName() << " of module: " << acc.getOwningModule()->getName()
    //                << std::endl;
    //    }
  }
  virtual ~OuterGroup() {}

  TestModule outerModule{this, "outerModuleInGroup", "", ctk::HierarchyModifier::oneLevelUp};
  InnerGroup innerGroup;
};

struct TestApplication : public ctk::Application {
  TestApplication(ctk::HierarchyModifier outerModuleModifier,
      ctk::HierarchyModifier innerGroupModifier = ctk::HierarchyModifier::none, bool skipConnection = false)
  : Application("testApp"), outerModuleGroup1{this, "outerModuleGroup1", "", ctk::HierarchyModifier::none,
                                innerGroupModifier},
    outerModule{this, "outerModule", "", outerModuleModifier}, _skipConnection{skipConnection} {}
  ~TestApplication() { shutdown(); }

  OuterGroup outerModuleGroup1;
  TestModule outerModule;

  ctk::ControlSystemModule cs;

  void defineConnections() {
    // Tests for getVirtualQualifiedName require that findTag is not used globally,
    // so it can be disabled
    if(!_skipConnection) {
      findTag(".*").connectTo(cs);
    }
    //cs.dump();
  }
  bool _skipConnection;
};

// Check if HierarchyModifiers are properly handled in the call to findTag
// in the constructor of TestApplication
BOOST_AUTO_TEST_CASE(testIllegalModifiers) {
  std::cout << "testIllegalModifiers" << std::endl;

  {
    std::cout << "  Creating TestApplication with outerModuleModifier = none " << std::endl;
    // Should work
    TestApplication app(ctk::HierarchyModifier::none);
    ctk::TestFacility test;
    std::cout << std::endl;
  }

  {
    std::cout << "  Creating TestApplication with outerModuleModifier = oneLevelUp " << std::endl;
    TestApplication app(ctk::HierarchyModifier::oneLevelUp);
    // Should detect illegal usage of oneLevelUp on first level below Application and throw
    BOOST_CHECK_THROW(ctk::TestFacility test, ctk::logic_error);
    std::cout << std::endl;
  }

  // Should detect illegal usage of oneUpAndHide on first level below Application and throw
  // Currently leads to memory access violation, should also throw
  // Bug is described by issue #166.
  //  {
  //    std::cout << "Creating TestApplication with outerModuleModifier = oneUpAndHide " << std::endl;
  //    TestApplication app(ctk::HierarchyModifier::oneUpAndHide);
  //    ctk::TestFacility test;
  //    std::cout << std::endl;
  //  }

  {
    std::cout << "  Creating TestApplication with outerModuleModifier = moveToRoot " << std::endl;
    // Should work
    TestApplication app(ctk::HierarchyModifier::moveToRoot);
    ctk::TestFacility test;
    std::cout << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(testGetVirtualQualifiedName) {
  std::cout << "testGetVirtualQualifiedName" << std::endl;

  {
    TestApplication app(ctk::HierarchyModifier::none);
    ctk::TestFacility test;

    //app.cs.dump();
    BOOST_CHECK_EQUAL(app.outerModule.getVirtualQualifiedName(), "/testApp/outerModule");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.getVirtualQualifiedName(), "/testApp/outerModuleGroup1");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.outerModule.getVirtualQualifiedName(), "/testApp/outerModuleInGroup");
    BOOST_CHECK_EQUAL(
        app.outerModuleGroup1.innerGroup.getVirtualQualifiedName(), "/testApp/outerModuleGroup1/innerModuleGroup");

    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModule.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup/innerModule");
    BOOST_CHECK_EQUAL(
        app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups.groupOneLevelUp.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2");
    BOOST_CHECK_EQUAL(
        app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups.groupOneUpAndHide.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup");
    BOOST_CHECK_EQUAL(
        app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups2.groupOneLevelUp.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1");
    BOOST_CHECK_EQUAL(
        app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups2.groupOneUpAndHide.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup");

    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModuleOneUpAndHide.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModuleMoveToRoot.getVirtualQualifiedName(),
        "/testApp/innerModuleMoveToRoot");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModuleSameNameAsGroup.getVirtualQualifiedName(),
        "/testApp/outerModuleGroup1/innerModuleGroup");
  }

  {
    TestApplication app(ctk::HierarchyModifier::hideThis);
    ctk::TestFacility test;

    //    app.cs.dump();
    BOOST_CHECK_EQUAL(app.outerModule.getVirtualQualifiedName(), "/testApp");
  }

  // Modifiers oneLevelUp and oneUpAndHide need to be catched by getVirtualQualifiedName, if used on
  // the top level og the application. Note: If defineConnections uses findTag on the entire app,
  // the error is catched there, this is avoided by the boolean constructor arguments below
  {
    TestApplication app(ctk::HierarchyModifier::oneLevelUp, ctk::HierarchyModifier::none, true);
    ctk::TestFacility test;
    BOOST_CHECK_THROW(app.outerModule.getVirtualQualifiedName(), ctk::logic_error);
  }
  {
    TestApplication app(ctk::HierarchyModifier::oneUpAndHide, ctk::HierarchyModifier::none, true);
    ctk::TestFacility test;
    BOOST_CHECK_THROW(app.outerModule.getVirtualQualifiedName(), ctk::logic_error);
  }

  {
    TestApplication app(ctk::HierarchyModifier::moveToRoot, ctk::HierarchyModifier::moveToRoot);
    ctk::TestFacility test;

    //    app.cs.dump();
    BOOST_CHECK_EQUAL(app.outerModule.getVirtualQualifiedName(), "/testApp/outerModule");
    auto virtualisedApp = app.findTag(".*");
    BOOST_CHECK_NO_THROW(virtualisedApp["outerModule"]);
    BOOST_CHECK_NO_THROW(virtualisedApp["innerModuleMoveToRoot"]);

    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.getVirtualQualifiedName(), "/testApp/innerModuleGroup");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModule.getVirtualQualifiedName(),
        "/testApp/innerModuleGroup/innerModule");
  }
}

BOOST_AUTO_TEST_CASE(testGetNetworkNodesOnVirtualHierarchy) {
  std::cout << "testGetNetworkNodesOnVirtualHierarchy" << std::endl;

  TestApplication app(ctk::HierarchyModifier::none);
  ctk::TestFacility test;

  app.cs.dump();
  //app.outerModuleGroup1.virtualise().dump();

  auto virtualisedApplication = app.findTag(".*");
  //virtualisedApplication.dump();

  // Need to trip away "/appName/" in the submodule() calls
  size_t firstModuleOffsetInPath = ("/" + app.getName() + "/").size();

  auto pathToInnerModuleOneUpAndHide =
      app.outerModuleGroup1.innerGroup.innerModuleOneUpAndHide.getVirtualQualifiedName();

  // Get submodule by the virtual path
  ctk::Module& module = virtualisedApplication.submodule(
      {pathToInnerModuleOneUpAndHide.begin() + firstModuleOffsetInPath, pathToInnerModuleOneUpAndHide.end()});
  auto node2 = module("input2");
  BOOST_CHECK_EQUAL(node2.getName(), "input2");

  // As a reference, navigate to the module using operator []
  auto node2Ref = virtualisedApplication["outerModuleGroup1"]("input2");
  BOOST_CHECK(node2 == node2Ref);

  // Repeat test for other modules: Module moved to root
  auto pathToInnerModuleMoveToRoot = app.outerModuleGroup1.innerGroup.innerModuleMoveToRoot.getVirtualQualifiedName();

  ctk::Module& innerModuleMoveToRoot = virtualisedApplication.submodule(
      {pathToInnerModuleMoveToRoot.begin() + firstModuleOffsetInPath, pathToInnerModuleMoveToRoot.end()});
  auto node3 = innerModuleMoveToRoot("input3");

  auto node3Ref = virtualisedApplication["innerModuleMoveToRoot"]("input3");
  BOOST_CHECK(node3 == node3Ref);

  // Repeat test for other modules: Module with same name as its group and modifier oneLevelUp
  auto pathToInnerModuleSameNameAsGroup =
      app.outerModuleGroup1.innerGroup.innerModuleSameNameAsGroup.getVirtualQualifiedName();

  ctk::Module& innerModuleSameNameAsGroup = virtualisedApplication.submodule(
      {pathToInnerModuleSameNameAsGroup.begin() + firstModuleOffsetInPath, pathToInnerModuleSameNameAsGroup.end()});
  node3 = innerModuleSameNameAsGroup("input3");

  node3Ref = virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]("input3");
  BOOST_CHECK(node3 == node3Ref);

  auto pathToGroupWithOneLevelUp =
      app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups.groupOneLevelUp.getVirtualQualifiedName();
  ctk::Module& groupWithOneLevelUp = virtualisedApplication.submodule(
      {pathToGroupWithOneLevelUp.begin() + firstModuleOffsetInPath, pathToGroupWithOneLevelUp.end()});
  auto node4 = groupWithOneLevelUp("var1InGroupOneLevelUp");

  auto node4Ref = virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]["InnerModuleWithVariableGroups2"](
      "var1InGroupOneLevelUp");
  BOOST_CHECK(node4 == node4Ref);

  auto pathToGroupWithOneUpAndHide =
      app.outerModuleGroup1.innerGroup.innerModuleWithVariableGroups.groupOneUpAndHide.getVirtualQualifiedName();
  ctk::Module& groupWithOneUpAndHide = virtualisedApplication.submodule(
      {pathToGroupWithOneUpAndHide.begin() + firstModuleOffsetInPath, pathToGroupWithOneUpAndHide.end()});
  auto node6 = groupWithOneUpAndHide("var1InGroupOneUpAndHide");

  auto node6Ref = virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]("var1InGroupOneUpAndHide");
  BOOST_CHECK(node6 == node6Ref);
}

// helper class to avoid code duplications. Not a generic network node test but very special for
// nodes with 1 app feeder, 1 app consumer and 1 cs consumer
void testNetworkNode(ctk::VariableNetworkNode node, std::string feederName, std::string consumerName) {
  std::cout << "checking network of " << node.getName() << std::endl;
  auto& network = node.getOwner();
  BOOST_CHECK_EQUAL(network.getFeedingNode().getQualifiedName(), feederName);
  BOOST_CHECK_EQUAL(network.getConsumingNodes().size(), 2);
  int appConsumerCounter{0};
  for(auto& consumer : network.getConsumingNodes()) {
    if(consumer.getType() == ctk::NodeType::Application) {
      BOOST_CHECK_EQUAL(consumer.getQualifiedName(), consumerName);
      ++appConsumerCounter;
    }
  }
  BOOST_CHECK_EQUAL(appConsumerCounter, 1);
}

BOOST_AUTO_TEST_CASE(testNetworks) {
  std::cout << "testNetworks" << std::endl;

  // check that all variables that should be connected with the modifed hierarchies actually are tin the same network

  TestApplication app(ctk::HierarchyModifier::none);
  ctk::TestFacility test;

  auto virtualisedApplication = app.findTag(".*");
  //app.dumpConnections();

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]("var1InGroupOneUpAndHide"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/YoullNeverSee/"
      "var1InGroupOneUpAndHide",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/IntentionallyNotYoullNeverSee/"
      "var1InGroupOneUpAndHide");

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]("var2InGroupOneUpAndHide"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/IntentionallyNotYoullNeverSee/"
      "var2InGroupOneUpAndHide",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/YoullNeverSee/"
      "var2InGroupOneUpAndHide");

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]["InnerModuleWithVariableGroups2"](
                      "var1InGroupOneLevelUp"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/InnerModuleWithVariableGroups2/"
      "var1InGroupOneLevelUp",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/var1InGroupOneLevelUp");

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]["InnerModuleWithVariableGroups2"](
                      "var2InGroupOneLevelUp"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/var2InGroupOneLevelUp",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/InnerModuleWithVariableGroups2/"
      "var2InGroupOneLevelUp");

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]["InnerModuleWithVariableGroups1"](
                      "var3InGroupOneLevelUp"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/var3InGroupOneLevelUp",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/InnerModuleWithVariableGroups1/"
      "var3InGroupOneLevelUp");

  testNetworkNode(virtualisedApplication["outerModuleGroup1"]["innerModuleGroup"]["InnerModuleWithVariableGroups1"](
                      "var4InGroupOneLevelUp"),
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups2/InnerModuleWithVariableGroups1/"
      "var4InGroupOneLevelUp",
      "/testApp/outerModuleGroup1/innerModuleGroup/InnerModuleWithVariableGroups1/var4InGroupOneLevelUp");
}
