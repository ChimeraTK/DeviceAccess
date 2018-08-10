/*!
 * \author Klaus Zenker (HZDR)
 * \date 10.08.2018
 * \page historydoc Server based history module
 * \section into Server based history
 *  Some control systems offer a variable history but some do not. In this case the \c ServerHistory can be used to
 *  create a history ring buffer on the server. If only a local history is needed consider to use the \c MicroDAQ module.
 * In order to do so you connect the variable that should have a history on the server
 *  to the \c ServerHistory module. In addition you need to specify the history length. Every time the variable is updated
 *  it will be filled into the history buffer. The buffer length (history length) can not be changed during runtime and
 *  requires a server restart.
 *  Output variables created by the \c ServerHistory module are named: \c "module_name"/"variable_name"_hist, where
 *  the variable_name can be set different from the original feeding variable.
 *  The following tags are added to the history output variable:
 *  - CS
 *  - history
 *  - "module_name"
 *
 *  The \c ServerHistory module also includes a Logger, which can be used to feed a \c LoggingModule
 *  So far the \c ServerHistory is just implemented for float variables, but it could easily be extended using a template approach.
 *
 *  The following example shows how to integrate use the \c ServerHistory module
 *  messages (TestModule).
 *  \code
 *  sruct TestModule: public ChimeraTK::ApplicationModule{
 *  chimeraTK::ScalarOutput<float> measurement{...};
 *  ...
 *  };
 *  struct myApp : public ChimeraTK::Application{
 *
 *  history::ServerHistory history{this, "ServerHistory", "History for certain variables"};
 *
 *  ChimeraTK::ControlSystemModule cs;
 *
 *  LoggingModule log { this, "LoggingModule", "LoggingModule test" };
 *
 *  TestModule test{ this, "test", "" };
 *  ...
 *  };
 *
 *
 *  void myAPP::defineConnctions(){
 *  history.addHistory(&test.measurement, test->getName(), "temperature", 1200)
 *  history.findTag("CS").connectTo(cs["History"]);
 *  // will show up in the control system as History/test/temperature_hist
 *
 *  // logging configuration
 *  cs["Logging"]("targetStream") >> log.targetStream;
 *  cs["Logging"]("logLevel") >> log.logLevel;
 *  cs["Logging"]("logFile") >> log.logFile;
 *  cs["Logging"]("tailLength") >> log.tailLength;
 *  log.addSource(&history.logger)
 *  ...
 *  }
 *
 *  \endcode
 */


#ifndef MODULES_SERVERHISTORY_H_
#define MODULES_SERVERHISTORY_H_

#include "ApplicationCore.h"
#include "Logging.h"

#include <vector>

namespace ctk = ChimeraTK;

namespace history{

struct ServerHistory: public ctk::ApplicationModule{
  using ctk::ApplicationModule::ApplicationModule;
  using histEntry = std::pair<ctk::ScalarPushInput<float>,ctk::ArrayOutput<float> > ;

  //\ToDo: Make this flexible in type -> templated map with different template types
  std::vector<histEntry> v_history;

  /* List of inputs to the history module used to avoid duplicates */
  std::vector<std::string> inputs;

  logging::Logger logger{this};

  void addHistory(ctk::ScalarOutput<float> *in, const std::string &module_name, const std::string &par_name, const uint &length);
  void mainLoop() override;
};

}

#endif /* MODULES_SERVERHISTORY_H_ */
