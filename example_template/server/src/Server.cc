
#include "Server.h"
#include "version.h"


void Server::defineConnections() {
  ctk::setDMapFilePath("devices.dmap");


//  ctk::VariableNetworkNode trigger{};
//  //trigger = externalTrigger("MACRO_PULSE_NUMBER", typeid(int), 1, ctk::UpdateMode::push);
//  trigger >> cs["Server"]("triggerNumber");

  dev.connectTo(cs/*, timer.tick*/);
}

