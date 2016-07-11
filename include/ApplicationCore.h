/**
 * ApplicationCore.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 *
 *  This is the main header file for the ApplicationCore library. It brings all includes and functionality needed
 *  for writing an application.
 */

#include "Application.h"
#include "ScalarAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"

#ifndef CHIMERATK_APPLICATION_CORE_H
#define CHIMERATK_APPLICATION_CORE_H

/** Compile-time switch: two executables will be created. One will generate an XML file containing the
 *  application's variable list. The other will be the actual control system server running the
 *  application. */
#ifndef GENERATE_XML

/** @todo TODO This works for DOOCS only. We need a common interface for all control system adapter
 *  implementations to generate server executables! */
#include <ControlSystemAdapter-DoocsAdapter/DoocsAdapter.h>

  BEGIN_DOOCS_SERVER("Control System Adapter", 10)
     // set the DOOCS server name to the application name
     object_name = ChimeraTK::Application::getInstance().getName().c_str();
     // Create static instances for all applications cores. They must not have overlapping
     // process variable names ("location/protery" must be unique).
     ChimeraTK::Application::getInstance().setPVManager(doocsAdapter.getDevicePVManager());
     ChimeraTK::Application::getInstance().run();
  END_DOOCS_SERVER()

#else

  int main(int, char **) {
    ChimeraTK::Application::getInstance().generateXML();
    return 0;
  }

#endif /* GENERATE_XML */


#endif /* CHIMERATK_APPLICATION_CORE_H */
