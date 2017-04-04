/**
 * ApplicationCore.h
 *
 *  Created on: Jun 14, 2016
 *      Author: Martin Hierholzer
 *
 *  This is the main header file for the ApplicationCore library. It brings all includes and functionality needed
 *  for writing an application.
 */

#include <mtca4u/Utilities.h>   // for mtca4u::setDMapFilePath(), which is used by all applications

#include "Application.h"
#include "ScalarAccessor.h"
#include "ArrayAccessor.h"
#include "ApplicationModule.h"
#include "DeviceModule.h"
#include "ControlSystemModule.h"
#include "VariableGroup.h"
#include "VirtualModule.h"

#ifndef CHIMERATK_APPLICATION_CORE_H
#define CHIMERATK_APPLICATION_CORE_H

/** Compile-time switch: two executables will be created. One will generate an XML file containing the
 *  application's variable list. The other will be the actual control system server running the
 *  application. The main function for the actual control system server is coming from the respective
 *  ControlSystemAdapter implementation library, so inly the XML generator's main function is definde
 *  here. */
#ifdef GENERATE_XML

  int main(int, char **) {
    ChimeraTK::Application::getInstance().generateXML();
    return 0;
  }

#endif /* GENERATE_XML */


#endif /* CHIMERATK_APPLICATION_CORE_H */
