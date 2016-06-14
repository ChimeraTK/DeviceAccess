/*
 * demoApp.cc
 *
 *  Created on: Jun 9, 2016
 *      Author: Martin Hierholzer
 */

#include <iostream>

#include <mtca4u/BackendFactory.h>
#include <ControlSystemAdapter-DoocsAdapter/DoocsAdapter.h>

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"

namespace ctk = ChimeraTK;

class MyFirstModule : public ctk::ApplicationModule {
  public:

    SCALAR_ACCESSOR(int, setpoint, ctk::VariableDirection::input, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(int, feedforward, ctk::VariableDirection::output, "MV/m", ctk::UpdateMode::push);

    void mainLoop() {
      feedforward = 100;
      feedforward.write();

      while(true) {
        setpoint.read();
        feedforward = 42*setpoint;
        std::cout << "FirstModule: feedforward = " << feedforward << std::endl;
        usleep(200000);
        feedforward.write();
      }

    }
};


class MySecondModule : public ctk::ApplicationModule {
  public:

    SCALAR_ACCESSOR(int, feedforward, ctk::VariableDirection::input, "MV/m", ctk::UpdateMode::push);
    SCALAR_ACCESSOR(int, readback, ctk::VariableDirection::output, "MV/m", ctk::UpdateMode::push);

    void mainLoop() {

      while(true) {
        feedforward.read();
        std::cout << "SecondModule: feedforward = " << feedforward << std::endl;
        readback = feedforward/21;
        usleep(200000);
        readback.write();
      }

    }
};



class MyApp : public ctk::Application {

  public:

    MyFirstModule firstModule;
    MySecondModule secondModule;

    constexpr static char unit_Field[] = "MV/m";

    void initialise() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      firstModule.feedforward.connectToDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      secondModule.feedforward.connectToDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);

      firstModule.setpoint.publish("MyLocation/setpoint");
      secondModule.readback.publish("MyLocation/readback");
    }

    virtual ~MyApp() {};

};



BEGIN_DOOCS_SERVER("Cosade server", 10)
   // Create static instances for all applications cores. They must not have overlapping
   // process variable names ("location/protery" must be unique).
   static MyApp myApp;
   myApp.setPVManager(doocsAdapter.getDevicePVManager());
   myApp.run();
END_DOOCS_SERVER()
