
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
  TestApplication(ctk::HierarchyModifier outerModuleModifier)
  : Application("testApp"), outerModule{this, "outerModule", "", outerModuleModifier} {}
  ~TestApplication() { shutdown(); }

  OuterGroup outerModuleGroup1{this, "outerModuleGroup", "", ctk::HierarchyModifier::none};

  TestModule outerModule;

  ctk::ControlSystemModule cs;

  void defineConnections() {
    findTag(".*").connectTo(cs);
    cs.dump();
  }
};

BOOST_AUTO_TEST_CASE(testIllegalModifiers) {
  // Just test if the app comes up without without
  std::cout << "testIllegalModifiers" << std::endl;

  {
    std::cout << "Creating TestApplication with outerModuleModifier = none " << std::endl;
    // Should work
    TestApplication app(ctk::HierarchyModifier::none);
    ctk::TestFacility test;
    std::cout << std::endl;
  }

  {
    std::cout << "Creating TestApplication with outerModuleModifier = oneLevelUp " << std::endl;
    TestApplication app(ctk::HierarchyModifier::oneLevelUp);
    // Should detect usage of oneLevelUp and throw
    BOOST_CHECK_THROW(ctk::TestFacility test, ctk::logic_error);
    std::cout << std::endl;
  }

  // Currently leads to memory access violation, should also throw
//  {
//    std::cout << "Creating TestApplication with outerModuleModifier = oneUpAndHide " << std::endl;
//    TestApplication app(ctk::HierarchyModifier::oneUpAndHide);
//    ctk::TestFacility test;
//    std::cout << std::endl;
//  }

    {
      std::cout << "Creating TestApplication with outerModuleModifier = moveToRoot " << std::endl;
      // Should work
      TestApplication app(ctk::HierarchyModifier::moveToRoot);
      ctk::TestFacility test;
      std::cout << std::endl;
    }
}
