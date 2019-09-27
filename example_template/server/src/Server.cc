
#include "Server.h"
#include "version.h"


void Server::defineConnections() {
  ctk::setDMapFilePath("devices.dmap");

  std::cout << "****************************************************************" << std::endl;
  std::cout << "*** Template server version " << AppVersion::major << "." << AppVersion::minor << "."
            << AppVersion::patch << std::endl;

  dev.connectTo(cs/*, timer.tick*/);
  config.connectTo(cs);

  dumpConnectionGraph();
  dumpGraph();
  dumpModuleGraph("module-graph.dot");
}

