/**
 * EnableXMLGenerator.h
 *
 *  Created on: May 5, 2017
 *      Author: Martin Hierholzer
 *
 *  Include this header file in your main application .cc file, so the xml
 * generator can be compiled using the compiler switch -DGENERATE_XML
 */

#ifndef CHIMERATK_APPLICATION_CORE_H
#define CHIMERATK_APPLICATION_CORE_H

/** Compile-time switch: two executables will be created. One will generate an
 * XML file containing the application's variable list. The other will be the
 * actual control system server running the application. The main function for
 * the actual control system server is coming from the respective
 *  ControlSystemAdapter implementation library, so inly the XML generator's
 * main function is definde here. */
#ifdef GENERATE_XML

int main(int, char **) {
  ChimeraTK::Application::getInstance().generateXML();
  return 0;
}

#endif /* GENERATE_XML */

#endif /* CHIMERATK_APPLICATION_CORE_H */
