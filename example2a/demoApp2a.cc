#include <ApplicationCore.h>
#include <PeriodicTrigger.h>

namespace ctk = ChimeraTK;

struct Controller : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;
    ctk::ScalarPollInput<double> sp{this, "sp", "degC", "Description", {"CS"}};
    ctk::ScalarPushInput<double> rb{this, "rb", "degC", "...", {"DEV", "CS"}};
    ctk::ScalarOutput<double> cur{this, "cur", "mA", "...", {"DEV"}};

    void mainLoop() {
      const double gain = 100.0;
      while(true) {
        readAll();     // waits until rb updated, then reads sp

        cur = gain * (sp - rb);
        writeAll();    // writes any outputs
      }
    }
};

struct Automation : public ctk::ApplicationModule {
    using ctk::ApplicationModule::ApplicationModule;
    ctk::ScalarPollInput<double> opSp{this, "opSp", "degC", "...", {"CS"}};
    ctk::ScalarOutput<double> actSp{this, "sp", "degC", "...", {"Controller"}};
    ctk::ScalarPushInput<uint64_t> trigger{this, "trigger", "", "..."};

    void mainLoop() {
      const double maxStep = 0.1;
      while(true) {
        readAll();     // waits until trigger received, then read opSp
        actSp += std::max(std::min(opSp - actSp, maxStep), -maxStep);
        writeAll();
      }
    }
};


struct ExampleApp : public ctk::Application {
    ExampleApp() : Application("exampleApp") {}
    ~ExampleApp() { shutdown(); }

    Controller controller{this, "Controller", "The Controller"};
    Automation automation{this, "Automation", "Slow setpoint ramping algorithm"};

    ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for the controller", 1000};

    ctk::DeviceModule heater{"oven","heater"};
    ctk::ControlSystemModule cs{"Bakery"};

    void defineConnections();
};
static ExampleApp theExampleApp;


void ExampleApp::defineConnections() {
    ChimeraTK::setDMapFilePath("example2.dmap");

    automation.findTag("Controller").connectTo(controller);
    automation.findTag("CS").connectTo(cs);
    timer.tick >> automation.trigger;

    controller.findTag("DEV").connectTo(heater, timer.tick);
    controller.findTag("CS").connectTo(cs);
}
