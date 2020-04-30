
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

struct InnerGroup : public ctk::ModuleGroup {
  using ctk::ModuleGroup::ModuleGroup;

  TestModule innerModule{this, "innerModule", "", ctk::HierarchyModifier::none};
  TestModule2 innerModuleOneUpAndHide{this, "innerModuleOneUpAndHide", "", ctk::HierarchyModifier::oneUpAndHide};
  TestModule3 innerModuleMoveToRoot{this, "innerModuleMoveToRoot", "", ctk::HierarchyModifier::moveToRoot};
  TestModule3 innerModuleSameNameAsGroup{this, "innerModuleGroup", "", ctk::HierarchyModifier::oneLevelUp};
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
      ctk::HierarchyModifier innerGroupModifier = ctk::HierarchyModifier::none)
  : Application("testApp"), outerModuleGroup1{this, "outerModuleGroup1", "", ctk::HierarchyModifier::none,
                                innerGroupModifier},
    outerModule{this, "outerModule", "", outerModuleModifier} {}
  ~TestApplication() { shutdown(); }

  OuterGroup outerModuleGroup1;
  TestModule outerModule;

  ctk::ControlSystemModule cs;

  void defineConnections() {
    findTag(".*").connectTo(cs);
    //cs.dump();
  }
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
    BOOST_CHECK_THROW(ctk::TestFacility test, ctk::logic_error)
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

  {
    TestApplication app(ctk::HierarchyModifier::moveToRoot, ctk::HierarchyModifier::moveToRoot);
    ctk::TestFacility test;

    //    app.cs.dump();
    BOOST_CHECK_EQUAL(app.outerModule.getVirtualQualifiedName(), "/testApp/outerModule");
    auto virtualisedApp = app.findTag(".*");
    BOOST_CHECK_NO_THROW(/*[[maybe_unused]]*/ ctk::Module& outerModuleRef = virtualisedApp["outerModule"];)
    BOOST_CHECK_NO_THROW(
        /*[[maybe_unused]]*/ ctk::Module& innerModuleMoveToRootRef = virtualisedApp["innerModuleMoveToRoot"];)

    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.getVirtualQualifiedName(), "/testApp/innerModuleGroup");
    BOOST_CHECK_EQUAL(app.outerModuleGroup1.innerGroup.innerModule.getVirtualQualifiedName(),
        "/testApp/innerModuleGroup/innerModule");
  }
}

BOOST_AUTO_TEST_CASE(testGetNetworkNodesOnVirtualHierarchy) {
  std::cout << "testGetNetworkNodesOnVirtualHierarchy" << std::endl;

  TestApplication app(ctk::HierarchyModifier::none);
  ctk::TestFacility test;

  //app.cs.dump();

  auto virtualisedApplication = app.findTag(".*");

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
}
