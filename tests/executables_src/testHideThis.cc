#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
#include <ChimeraTK/ApplicationCore/TestFacility.h>
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
      b.in = int(self.in);
      b.in.write();
    }

  } a{this, "A", "", HierarchyModifier::hideThis};

  // tries to hide itself like A, but fails. We get B/B/input and B/output
  struct B : public ApplicationModule {
    using ApplicationModule::ApplicationModule;

    struct : public VariableGroup {
      using VariableGroup::VariableGroup;

      ScalarPushInput<int> in{this, "input", "", ""};
    } self{this, "B", ""};

    // the output of B is one level up (global output)
    ScalarOutput<int> out{this, "output", "", ""};

    void mainLoop() override {
      out = int(self.in);
      out.write();
    }

  } b{this, "B", "", HierarchyModifier::hideThis}; // name it "HiddenB" here and it works

  void defineConnections() override { findTag(".*").connectTo(cs); }
};

int main() {
  TestApp t;
  ChimeraTK::TestFacility testFacility;
  testFacility.runApplication();

  t.dumpConnections();
  t.dump();
}
