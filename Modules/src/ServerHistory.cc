#include "ServerHistory.h"
#include <tuple>

using namespace history;

void ServerHistory::mainLoop(){
  if(inputs.size() == 0)
    throw ChimeraTK::logic_error("ServerHistory module has no inputs. Did you forget to add some in defineConnections()?");
//  logger.sendMessage(std::string("Starting main loop " + inputs.back()),logging::DEBUG);
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

void ServerHistory::addHistory(ctk::ScalarOutput<float> *in, const std::string &module_name, const std::string &par_name, const uint &length){
  if(find(inputs.begin(), inputs.end(), std::string(module_name + "/" + par_name)) == inputs.end()){
    v_history.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(ctk::ScalarPushInput<float>{this, par_name + "_hist_in", "", "",
        { "history", module_name }}),
        std::forward_as_tuple(ctk::ArrayOutput<float>{this, module_name + "/" + par_name + "_hist", "", length, "",
        { "CS", "history", module_name }}));
    *in >> v_history.back().first;
    inputs.push_back(module_name + "/" + par_name);
    logger.sendMessage(std::string("Added history for " + inputs.back()),logging::DEBUG);
  } else {
    throw ChimeraTK::logic_error("Cannot add history for variable "+par_name+
      " since history was already added for this variable.");
  }
}
