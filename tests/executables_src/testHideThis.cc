#define BOOST_TEST_MODULE testHideThis
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "ApplicationCore.h"
#include "TestFacility.h"
using namespace ChimeraTK;

struct TestApp : public Application {
  TestApp() : Application("test") {}
  ~TestApp() override { shutdown(); }

  ControlSystemModule cs;

  // hides itself, and then produces A and B as sub-grops. Works.
  struct A : public ApplicationModule {
    using ApplicationModule::ApplicationModule;

    struct : public VariableGroup {
      using VariableGroup::VariableGroup;

      ScalarPushInput<int> in{this, "input", "", ""};
    } self{this, "A", ""};

    // the output of A is the input of B
    struct : public VariableGroup {
      using VariableGroup::VariableGroup;

      ScalarOutput<int> in{this, "input", "", ""};
    } b{this, "B", ""};

    void mainLoop() override {
      while(true){
        b.in = 2*int(self.in);
        b.in.write();
        self.in.read();
      }
    }

  } a{this, "A", "", HierarchyModifier::hideThis};

  // This part was broken: Tried to hide itself like A, but failed. Result was B/B/input and B/output
  // Now is fixed
  struct B : public ApplicationModule {
    using ApplicationModule::ApplicationModule;

    struct : public VariableGroup {
      using VariableGroup::VariableGroup;

      ScalarPushInput<int> in{this, "input", "", ""};
    } self{this, "B", ""};

    // the output of B is one level up (global output)
    ScalarOutput<int> out{this, "output", "", ""};

    void mainLoop() override {
      while (true) {
        out = 3*int(self.in);
        out.write();
        self.in.read();
      }
    }

  } b{this, "B", "", HierarchyModifier::hideThis}; // name it "HiddenB" here and it works

  void defineConnections() override { findTag(".*").connectTo(cs); }
};

BOOST_AUTO_TEST_CASE(testBIsHidden) {
  TestApp t;
  ChimeraTK::TestFacility testFacility;
  testFacility.runApplication();

  t.dumpConnections();
  t.cs.dump();

  testFacility.writeScalar<int>("/A/input", 5);
  testFacility.stepApplication();

  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("B/input"),10); // A multiplies with 2
  // this checks two things:
  // 1. B/output has been moved to root
  // 2. B/B/input has been moved to B/input and connected through the CS with the output of A
  BOOST_CHECK_EQUAL(testFacility.readScalar<int>("output"),30); // B multiplies with 3
}
