#include <ApplicationCore.h>
#include <PeriodicTrigger.h>

namespace ctk = ChimeraTK;

struct Controller : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<double> sp{
      this, "temperatureSetpoint", "degC", "Description", {"CS"}};
  ctk::ScalarPushInput<double> rb{
      this, "temperatureReadback", "degC", "...", {"DEV", "CS"}};
  ctk::ScalarOutput<double> cur{this, "heatingCurrent", "mA", "...", {"DEV"}};

  void mainLoop() {
    const double gain = 100.0;
    while (true) {
      readAll(); // waits until rb updated, then reads sp

      cur = gain * (sp - rb);
      writeAll(); // writes any outputs
    }
  }
};

struct ExampleApp : public ctk::Application {
  ExampleApp() : Application("exampleApp") {}
  ~ExampleApp() { shutdown(); }

  Controller controller{this, "Controller", "The Controller"};

  ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for the controller",
                             1000};

  ctk::DeviceModule heater{"oven", "heater"};
  ctk::ControlSystemModule cs{"Bakery"};

  void defineConnections();
};
static ExampleApp theExampleApp;

void ExampleApp::defineConnections() {
  ChimeraTK::setDMapFilePath("example2.dmap");

  controller.findTag("DEV").connectTo(heater, timer.tick);
  controller.findTag("CS").connectTo(cs);
}
