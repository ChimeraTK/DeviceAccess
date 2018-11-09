/*!
 * \author Klaus Zenker (HZDR)
 * \date 10.08.2018
 * \page historydoc Server based history modules
 * \section historyintro Server based history
 *  Some control systems offer a variable history but some do not. In this case the \c ServerHistory can be used to
 *  create a history ring buffer on the server. If only a local history is needed consider to use the \c MicroDAQ module.
 *  In order to do so you connect the variable that should have a history on the server
 *  to the \c ScalarServerHistory module or the \c ArrayServerHistory for input scalars or input arrays.
 *  In addition you need to specify the history length. Every time the variable is updated
 *  it will be filled into the history buffer. The buffer length (history length) can not be changed during runtime and
 *  requires a server restart.
 *
 *  So far you need to create a ServerHistory for each variable type.
 *
 *  Output variables created by the \c ScalarServerHistory module are named: \c "module_name"/"variable_name"_hist.
 *  In case of \c ArrayServerHistory the outputs are named: \c "module_name"/"variable_name"_i_hist, where i
 *  is the position in the input array. In consequence an input array of length i will result in i
 *  output history arrays.
 *  In general the variable_name can be set different from the original feeding variable.
 *  The following tags are added to the history output variable:
 *  - CS
 *  - history
 *  - "module_name"
 *
 *  The ServerHistory modules also includes a Logger, which can be used to feed a \c LoggingModule
 *
 *  The following example shows how to integrate use the \c ScalarServerHistory module
 *  messages (TestModule).
 *  \code
 *  sruct TestModule: public ChimeraTK::ApplicationModule{
 *  chimeraTK::ScalarOutput<float> measurement{...};
 *  ...
 *  };
 *  struct myApp : public ChimeraTK::Application{
 *
 *  history::ScalarServerHistory<float> history{this, "ServerHistory", "History for certain scalara float variables"};
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
#include <tuple>

namespace ChimeraTK{
namespace history{

/**
 * \brief Module adding a history for an scalar output variable.
 *
 * A history array with a specified length will be created.
 * The name of the output variable is: module_name/par_name_hist,
 * where module_name and par_name are set when adding an output variable.
 *
 */
template<typename UserType>
struct ScalarServerHistory: public ApplicationModule{
  using ApplicationModule::ApplicationModule;
  using histEntry = std::pair<ScalarPushInput<UserType>,ArrayOutput<UserType> > ;

  std::vector<histEntry> v_history;

  /* List of inputs to the history module used to avoid duplicates */
  std::vector<std::string> inputs;

  logging::Logger logger{this};
  /*
   * This method should be called in defineConnections() of the application.
   * \param in A pointer to the ScalarOutput you want to have a history for.
   * \param module_name The name of the Module that contains in
   * \param par_name The name of the parameter (the created history variables will be named par_name_hist)
   * \param length This is the length of the history arrays.
   */
  void addHistory(ScalarOutput<UserType> *in, const std::string &module_name, const std::string &par_name, const uint &length);
  void mainLoop() override;
};

template<typename UserType>
void ScalarServerHistory<UserType>::addHistory(ScalarOutput<UserType> *in, const std::string &module_name, const std::string &par_name, const uint &length){
  if(find(inputs.begin(), inputs.end(), std::string(module_name + "/" + par_name)) == inputs.end()){
    std::string path = module_name + "/" + par_name + "_hist";
    v_history.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(ScalarPushInput<UserType>{this, par_name + "_hist_in", "", "",
        { "history", module_name }}),
        std::forward_as_tuple(ArrayOutput<UserType>{this, path, "", length, "",
        { "CS", "history", module_name }}));
    *in >> v_history.back().first;
    inputs.push_back(module_name + "/" + par_name);
    logger.sendMessage(std::string("Added history for " + inputs.back() + " with path: " + path),logging::DEBUG);
  } else {
    throw ChimeraTK::logic_error("Cannot add history for variable "+par_name+
      " since history was already added for this variable.");
  }
}

template<typename UserType>
void ScalarServerHistory<UserType>::mainLoop(){
  if(inputs.size() == 0)
    throw ChimeraTK::logic_error("ServerHistory module has no inputs. Did you forget to add some in defineConnections()?");
  auto group = readAnyGroup();
  while(true){
    auto id = group.readAny();
    for(auto &element: v_history){
      if(id == element.first.getId()){
        logger.sendMessage(std::string("Update history for variable:") + element.first.getName(), logging::DEBUG);
        std::rotate(element.second.begin(), element.second.begin()+1, element.second.end());
        *(element.second.end()-1) = element.first;
        element.second.write();
      }
    }
  }
}

/**
 * \brief Module adding a history for an ArrayOutput variable.
 *
 * For each element in the array a separate history array with a specified length will be created.
 * The name of the output variables is: module_name/par_name_i_hist,
 * where module_name and par_name are set when adding an output variable and i is the element index
 * of the input array.
 *
 */
template<typename UserType>
struct ArrayServerHistory: public ApplicationModule{
  using ApplicationModule::ApplicationModule;
  using histEntry = std::pair<ArrayPushInput<UserType>,std::vector<ArrayOutput<UserType> > > ;

  std::vector<histEntry> v_history;

  /* List of inputs to the history module used to avoid duplicates */
  std::vector<std::string> inputs;

  logging::Logger logger{this};

  /*
   * Since this method is to be called in defineConnections() of the application the array length is not yet available
   * from the input Array and needs to be set in addition.
   * \param in A pointer to the Array you want to have a history for.
   * \param arrLength The length of the array in
   * \param module_name The name of the Module that contains in
   * \param par_name The name of the parameter (the created history variables will be named par_name_i_hist
   * where i is the position in the array i)
   * \param length This is the length of the history arrays.
   */
  void addHistory(ArrayOutput<UserType> *in, const size_t arrLength, const std::string &module_name, const std::string &par_name, const uint &length);
  void mainLoop() override;
};

template<typename UserType>
void ArrayServerHistory<UserType>::addHistory(ArrayOutput<UserType> *in, const size_t arrLength, const std::string &module_name, const std::string &par_name, const uint &length){
  if(in == nullptr)
    throw runtime_error("Nullpointer passed to ArrrayServerHistory module");
  if(find(inputs.begin(), inputs.end(), std::string(module_name + "/" + par_name)) == inputs.end()){
    v_history.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(ArrayPushInput<UserType>{this, par_name + "_hist_in", "", arrLength, "",
        { "history", module_name }}),
        std::forward_as_tuple(std::vector<ArrayOutput<UserType> >{}));
    (*in) >> v_history.back().first;
    for(size_t i =0; i < arrLength; i++){
      v_history.back().second.emplace_back(ArrayOutput<UserType>{this, module_name + "/" + par_name + "_" + std::to_string(i) + "_hist", "", length, "",
        { "CS", "history", module_name }});
    }
    inputs.push_back(module_name + "/" + par_name);
    logger.sendMessage(std::string("Added history for " + inputs.back()),logging::DEBUG);
  } else {
    throw ChimeraTK::logic_error("Cannot add history for variable "+par_name+
      " since history was already added for this variable.");
  }
}

template<typename UserType>
void ArrayServerHistory<UserType>::mainLoop(){
  if(inputs.size() == 0)
    throw ChimeraTK::logic_error("ServerHistory module has no inputs. Did you forget to add some in defineConnections()?");
  auto group = readAnyGroup();
  while(true){
    auto id = group.readAny();
    for(auto &element: v_history){
      if(id == element.first.getId()){
        logger.sendMessage(std::string("Update history for variable:") + element.first.getName(), logging::DEBUG);
        for(size_t i = 0; i < element.first.getNElements(); i++){
          std::rotate(element.second.at(i).begin(), element.second.at(i).begin()+1, element.second.at(i).end());
          *(element.second.at(i).end()-1) = element.first[i];
          element.second.at(i).write();
        }
      }
    }
  }
}

struct ServerHistory : public ModuleGroup{
  using ModuleGroup::ModuleGroup;

  ScalarServerHistory<float> v_float;
  ScalarServerHistory<double> v_double;

  bool has_float{false};
  bool has_double{false};

  template<typename UserType>
  void addHistory(ScalarOutput<UserType> *in, const std::string &module_name, const std::string &par_name, const uint &length);

  /**
   * Use this method in defineConnections() of your application after all variables to the history.
   */
  void connctAll(ChimeraTK::ControlSystemModule *cs, std::string subfolder);

};

template<typename UserType>
void ServerHistory::addHistory(ScalarOutput<UserType> *in, const std::string &module_name, const std::string &par_name, const uint &length){
  throw ChimeraTK::runtime_error("ServerHistory: Type not supported.");
}

template<>
void ServerHistory::addHistory(ScalarOutput<float> *in, const std::string &module_name, const std::string &par_name, const uint &length){
  if(!has_float){
    v_float = ScalarServerHistory<float>{this, "ScalarServerHistory", "History of floats"};
    has_float = true;
  }
  v_float.addHistory(in, module_name, par_name, length);
}

template<>
void ServerHistory::addHistory(ScalarOutput<double> *in, const std::string &module_name, const std::string &par_name, const uint &length){
  if(!has_double){
    v_double = ScalarServerHistory<double>{this, "ScalarServerHistory", "History of doubles"};
    has_double = true;
  }
  v_double.addHistory(in, module_name, par_name, length);
}
}
}
#endif /* MODULES_SERVERHISTORY_H_ */
