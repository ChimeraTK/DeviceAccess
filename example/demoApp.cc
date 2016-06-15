/*
 * demoApp.cc
 *
 *  Created on: Jun 9, 2016
 *      Author: Martin Hierholzer
 */

#include <iostream>

#include <mtca4u/BackendFactory.h>

#include "ApplicationCore.h"

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

    void initialise() {
      mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");

      firstModule.feedforward.connectToDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      secondModule.feedforward.connectToDevice("Dummy0","/MyModule/Variable", ctk::UpdateMode::poll);
      //firstModule.feedforward.connectTo(secondModule.feedforward);

      firstModule.setpoint.publish("MyLocation/setpoint");
      secondModule.readback.publish("MyLocation/readback");
    }

    virtual ~MyApp() {};

};

MyApp myApp;

