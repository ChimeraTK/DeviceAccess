
#define BOOST_TEST_MODULE testVirtualModule
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

struct OuterGroup : public ctk::ModuleGroup {
  OuterGroup(
      EntityOwner* owner, const std::string& name, const std::string& description, ctk::HierarchyModifier modifier)
  : ModuleGroup(owner, name, description, modifier) {
    auto allAccessors = getOwner()->findTag(".*").getAccessorListRecursive();

    for(auto acc : allAccessors) {
      std::cout << "      -- Accessor: " << acc.getName() << " of module: " << acc.getOwningModule()->getName()
                << std::endl;
    }
  }
  virtual ~OuterGroup() {}

  TestModule outerModule{this, "outerModuleInGroup", "", ctk::HierarchyModifier::oneLevelUp};

  struct InnerGroup : public ctk::ModuleGroup {
    using ctk::ModuleGroup::ModuleGroup;

    TestModule innerModule{this, "innerModule", "", ctk::HierarchyModifier::hideThis};

  } innerGroup{this, "innerModuleGroup", ""};
};

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testApp") {}
  ~TestApplication() { shutdown(); }

  OuterGroup outerModuleGroup1{this, "outerModuleGroup", "", ctk::HierarchyModifier::oneUpAndHide};

  TestModule outerModule{this, "outerModule", "", ctk::HierarchyModifier::none};

  ctk::ControlSystemModule cs;

  void defineConnections() { findTag(".*").connectTo(cs); }
};

BOOST_AUTO_TEST_CASE(testStatusAggregator) {
  std::cout << "testStatusAggregator" << std::endl;

  TestApplication app;

  ctk::TestFacility test;
  test.runApplication();
  app.cs.dump();
}
