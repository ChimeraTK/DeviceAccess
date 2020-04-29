
#define BOOST_TEST_MODULE testStatusAggregator
#include <boost/test/included/unit_test.hpp>

#include "StatusAggregator.h"

#include "Application.h"
#include "ModuleGroup.h"
#include "TestFacility.h"

using namespace boost::unit_test_framework;

namespace ctk = ChimeraTK;

struct OuterGroup : public ctk::ModuleGroup {
  using ctk::ModuleGroup::ModuleGroup;
  virtual ~OuterGroup() {}

  ctk::StateMonitor<uint8_t> outerStateMonitor{this, "outerStateMonitor", "", "watch", "status",
      ctk::HierarchyModifier::none, {"OUTER_MON_OUTPUT"}, {"OUTER_MON_PARAMS"}, {"OUTER_MON_INPUT"}};

  struct InnerGroup : public ctk::ModuleGroup {
    using ctk::ModuleGroup::ModuleGroup;

    ctk::StateMonitor<uint8_t> innerStateMonitorNone{this, "innerMinMonitorNone", "", "watch", "status",
        ctk::HierarchyModifier::none, {"INNER_MON_OUTPUT"}, {"INNER_MON_PARAMS"}, {"INNER_MON_INPUT"}};

    ctk::StateMonitor<uint8_t> innerStateMonitorHideThis{this, "innerStateMonitorHideThis", "", "watch", "status",
        ctk::HierarchyModifier::hideThis, {"INNER_MON_OUTPUT"}, {"INNER_MON_PARAMS"}, {"INNER_MON_INPUT"}};

    ctk::StateMonitor<uint8_t> innerStateMonitorOneUp{this, "innerStateMonitorOneUp", "", "watch", "status",
        ctk::HierarchyModifier::oneLevelUp, {"INNER_MON_OUTPUT"}, {"INNER_MON_PARAMS"}, {"INNER_MON_INPUT"}};

  } innerGroup{this, "innerModuleGroup", "", ctk::HierarchyModifier::none};

  ctk::StatusAggregator outerStatusAggregator{this, "outerStatusAggregator", "StatusAggregator of OuterGroup",
      "groupStatus", ctk::HierarchyModifier::none, {"STATUS"}};
};

struct TestApplication : public ctk::Application {
  TestApplication() : Application("testApp") {}
  ~TestApplication() { shutdown(); }

  OuterGroup outerModuleGroup1{this, "outerModuleGroup1", "", ctk::HierarchyModifier::none};
  //OuterGroup outerModuleGroup2{this, "outerModuleGroup2", "", ctk::HierarchyModifier::hideThis};

  ctk::StateMonitor<uint8_t> globalStateMonitor{this, "globalStateMonitor", "", "stateWatch", "stateStatus",
      ctk::HierarchyModifier::none, {"GLOBAL_MON_OUTPUT"}, {"GLOBAL_MON_PARAMS"}, {"GLOBAL_MON_INPUT"}};

  ctk::ControlSystemModule cs;

  ctk::StatusAggregator globalStatusAggregator{this, "globalStatusAggregator", "Global StatusAggregator of testApp",
      "globalStatus", ctk::HierarchyModifier::none, {"STATUS"}};

  void defineConnections() { findTag(".*").connectTo(cs); }
};

BOOST_AUTO_TEST_CASE(testStatusAggregator) {
  std::cout << "testStatusAggregator" << std::endl;

  TestApplication app;

  ctk::TestFacility test;
  test.runApplication();
  //app.cs.dump();
}
