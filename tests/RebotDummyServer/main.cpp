
#include <iostream>
#include <signal.h>

#include "RebotDummyServer.h"
#include "argumentParser.h"

void setSigtemIndicator(int /*signalNumber*/) {
  // should handle signal numbers according to what is received; Letting this
  // slide for now as we would only receive the term signal, the way we are
  // testing currently
  ChimeraTK::sigterm_caught = true;
}

int main(int, char** argv) {

  // https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = setSigtemIndicator;
  sigaction(SIGTERM, &action, NULL);

  unsigned int portNumber = getPortNumber(argv);
  std::string mapFileLocation = getMapFileLocation(argv);
  unsigned int protocolVersion = getProtocolVersion(argv);
  
  ChimeraTK::RebotDummyServer testServer(portNumber, mapFileLocation, protocolVersion);
  testServer.start();

  return 0;
}
