/*
 * testApplication.cc
 *
 *  Created on: Nov 15, 2017
 *      Author: Martin Hierholzer
 */

#include <chrono>
#include <future>

#define BOOST_TEST_MODULE testApplication

#include <boost/filesystem.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/thread.hpp>

#include <libxml++/libxml++.h>

#include "Application.h"
#include "ControlSystemModule.h"
#include "Multiplier.h"
#include "Pipe.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Application without name */

struct TestApp : public ctk::Application {
  TestApp(const std::string& name) : ctk::Application(name) {}
  ~TestApp() { shutdown(); }

  using Application::makeConnections; // we call makeConnections() manually in
                                      // the tests to catch exceptions etc.
  void defineConnections() {
    multiplierD.output >> csmod("myVarD");
    csmod["mySubModule"]("myVarSIn") >> pipe.input;
    pipe.output >> csmod["mySubModule"]("myVarSOut");
    csmod("myVarU16") >> multiplierU16.input;
  }

  ctk::ConstMultiplier<double> multiplierD{this, "multiplierD", "Some module", 42};
  ctk::ScalarPipe<std::string> pipe{this, "pipe", "unit", "Some pipe module"};
  ctk::ConstMultiplier<uint16_t, uint16_t, 120> multiplierU16{this, "multiplierU16", "Some other module", 42};
  ctk::ControlSystemModule csmod;
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an
 * app variable */

BOOST_AUTO_TEST_CASE(testApplicationExceptions) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testApplicationExceptions" << std::endl;

  // zero length name forbidden
  try {
    TestApp app("");
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // names with spaces and special characters are forbidden
  try {
    TestApp app("With space");
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }
  try {
    TestApp app("WithExclamationMark!");
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }

  // all allowed characters in the name
  { TestApp app("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz_1234567890"); }

  // repeated characters are allowed
  { TestApp app("AAAAAAA"); }

  // Two apps at the same time are not allowed
  TestApp app1("FirstInstance");
  try {
    TestApp app2("SecondInstance");
    BOOST_FAIL("Exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }
}

/*********************************************************************************************************************/
// Helper function for testXmlGeneration:
// Obtain a value from an XML node
std::string getValueFromNode(const xmlpp::Node* node, const std::string& subnodeName) {
  xmlpp::Node* theChild = nullptr;
  for(const auto& child : node->get_children()) {
    if(child->get_name() == subnodeName) theChild = child;
  }
  BOOST_REQUIRE(theChild != nullptr); // requested child tag is there
  auto subChildList = theChild->get_children();
  if(subChildList.size() == 0) { // special case: no text in the tag -> return empty string
    return "";
  }
  BOOST_REQUIRE_EQUAL(subChildList.size(),
      1); // child tag contains only text (no further sub-tags)
  const xmlpp::TextNode* textNode = dynamic_cast<xmlpp::TextNode*>(subChildList.front());
  BOOST_REQUIRE(textNode != nullptr);
  return textNode->get_content();
}

/*********************************************************************************************************************/
/* test creation of XML file describing the variable tree */

BOOST_AUTO_TEST_CASE(testXmlGeneration) {
  std::cout << "***************************************************************"
               "******************************************************"
            << std::endl;
  std::cout << "==> testXmlGeneration" << std::endl;

  // delete XML file if already existing
  boost::filesystem::remove("TestAppInstance.xml");

  // create app which exports some properties and generate its XML file
  TestApp app("TestAppInstance");
  app.generateXML();

  // validate the XML file
  xmlpp::XsdValidator validator("application.xsd");
  validator.validate("TestAppInstance.xml");

  // parse XML file
  xmlpp::DomParser parser;
  try {
    parser.parse_file("TestAppInstance.xml");
  }
  catch(xmlpp::exception& e) {
    throw std::runtime_error(std::string("ConfigReader: Error opening the config file "
                                         "'TestAppInstance.xml': ") +
        e.what());
  }

  // get root element
  const auto root = parser.get_document()->get_root_node();
  BOOST_CHECK_EQUAL(root->get_name(), "application");

  // parsing loop
  bool found_myVarD{false};
  bool found_myVarSIn{false};
  bool found_myVarSOut{false};
  bool found_myVarU8{false};

  for(const auto& child : root->get_children()) {
    // cast into element, ignore if not an element (e.g. comment)
    auto* element = dynamic_cast<const xmlpp::Element*>(child);
    if(!element) continue;

    if(element->get_name() == "variable") {
      // obtain attributes from the element
      auto xname = element->get_attribute("name");
      BOOST_REQUIRE(xname != nullptr);
      std::string name(xname->get_value());

      // obtain values from sub-elements
      std::string value_type = getValueFromNode(element, "value_type");
      std::string direction = getValueFromNode(element, "direction");
      std::string unit = getValueFromNode(element, "unit");
      std::string description = getValueFromNode(element, "description");
      std::string numberOfElements = getValueFromNode(element, "numberOfElements");

      // check if variables are described correctly
      if(name == "myVarD") {
        found_myVarD = true;
        BOOST_CHECK_EQUAL(value_type, "double");
        BOOST_CHECK_EQUAL(direction, "application_to_control_system");
        BOOST_CHECK_EQUAL(unit, "");
        BOOST_CHECK_EQUAL(description, "Some module");
        BOOST_CHECK_EQUAL(numberOfElements, "1");
      }
      else if(name == "myVarU16") {
        found_myVarU8 = true;
        BOOST_CHECK_EQUAL(value_type, "uint16");
        BOOST_CHECK_EQUAL(direction, "control_system_to_application");
        BOOST_CHECK_EQUAL(unit, "");
        BOOST_CHECK_EQUAL(description, "Some other module");
        BOOST_CHECK_EQUAL(numberOfElements, "120");
      }
      else {
        BOOST_ERROR("Wrong variable name found.");
      }
    }
    else if(element->get_name() == "directory") {
      auto xname = element->get_attribute("name");
      BOOST_REQUIRE(xname != nullptr);
      std::string name(xname->get_value());
      BOOST_CHECK_EQUAL(name, "mySubModule");

      for(const auto& subchild : child->get_children()) {
        auto* element = dynamic_cast<const xmlpp::Element*>(subchild);
        if(!element) continue;
        BOOST_CHECK_EQUAL(element->get_name(), "variable");

        // obtain attributes from the element
        auto xname = element->get_attribute("name");
        BOOST_REQUIRE(xname != nullptr);
        std::string name(xname->get_value());

        // obtain values from sub-elements
        std::string value_type = getValueFromNode(element, "value_type");
        std::string direction = getValueFromNode(element, "direction");
        std::string unit = getValueFromNode(element, "unit");
        std::string description = getValueFromNode(element, "description");
        std::string numberOfElements = getValueFromNode(element, "numberOfElements");

        if(name == "myVarSIn") {
          found_myVarSIn = true;
          BOOST_CHECK_EQUAL(value_type, "string");
          BOOST_CHECK_EQUAL(direction, "control_system_to_application");
          BOOST_CHECK_EQUAL(unit, "unit");
          BOOST_CHECK_EQUAL(description, "Some pipe module");
          BOOST_CHECK_EQUAL(numberOfElements, "1");
        }
        else if(name == "myVarSOut") {
          found_myVarSOut = true;
          BOOST_CHECK_EQUAL(value_type, "string");
          BOOST_CHECK_EQUAL(direction, "application_to_control_system");
          BOOST_CHECK_EQUAL(unit, "unit");
          BOOST_CHECK_EQUAL(description, "Some pipe module");
          BOOST_CHECK_EQUAL(numberOfElements, "1");
        }
        else {
          BOOST_ERROR("Wrong variable name found.");
        }
      }
    }
    else {
      BOOST_ERROR("Wrong tag found.");
    }
  }

  BOOST_CHECK(found_myVarD);
  BOOST_CHECK(found_myVarSIn);
  BOOST_CHECK(found_myVarSOut);
  BOOST_CHECK(found_myVarU8);
}
