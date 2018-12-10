#include <ApplicationCore.h>
#include <PeriodicTrigger.h>

namespace ctk = ChimeraTK;

struct ExampleApp : public ctk::Application {
    ExampleApp() : Application("exampleApp") {}
    ~ExampleApp() { shutdown(); }

    ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for the controller", 1000};

    ctk::DeviceModule dev{"oven"};
    ctk::ControlSystemModule cs{"Bakery"};

    void defineConnections();
};
static ExampleApp theExampleApp;


void ExampleApp::defineConnections() {
    ChimeraTK::setDMapFilePath("example2.dmap");
    dev.connectTo(cs, timer.tick);
}
