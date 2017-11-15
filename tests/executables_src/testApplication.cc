/*
 * testApplication.cc
 *
 *  Created on: Nov 15, 2017
 *      Author: Martin Hierholzer
 */

#include <future>
#include <chrono>

#define BOOST_TEST_MODULE testApplication

#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>
#include <boost/thread.hpp>

#include "Application.h"
#include "Multiplier.h"

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

/*********************************************************************************************************************/
/* Application without name */

struct UnnamedApp : public ctk::Application {
    UnnamedApp() : ctk::Application("") { ChimeraTK::ExperimentalFeatures::enable(); }
    ~UnnamedApp() { shutdown(); }
    
    using Application::makeConnections;     // we call makeConnections() manually in the tests to catch exceptions etc.
    void defineConnections() {}             // the setup is done in the tests
    
    ctk::ConstMultiplier<double> multiplier{this, "multiplier", "Some module", 42};
};

/*********************************************************************************************************************/
/* test trigger by app variable when connecting a polled device register to an app variable */

BOOST_AUTO_TEST_CASE( testApplicationExceptions ) {
  std::cout << "*********************************************************************************************************************" << std::endl;
  std::cout << "==> testApplicationExceptions" << std::endl;
  
  try {
    UnnamedApp app;
    BOOST_FAIL("Exception expected.");
  }
  catch(ctk::ApplicationExceptionWithID<ctk::ApplicationExceptionID::illegalParameter>) {
  }

}

