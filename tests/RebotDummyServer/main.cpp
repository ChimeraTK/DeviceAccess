
#include <iostream>
#include <signal.h>

#include "RebotDummyServer.h"
#include "argumentParser.h"

void setSigtemIndicator(int signalNumber =0){
  // should handle signal numbers according to what is received; Letting this
  // slide for now as we would only receive the term signal, the way we are
  // testing currently
  std::cout << "caught signal " << signalNumber<< std::endl;
  mtca4u::sigterm_caught = true;
}


int main(int, char** argv) {

  //https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = setSigtemIndicator;
  sigaction(SIGTERM, &action, NULL);

  unsigned int portNumber = getPortNumber(argv);
  std::string mapFileLocation = getMapFileLocation(argv);

  mtca4u::RebotDummyServer testServer(portNumber, mapFileLocation);
  testServer.start();

  std::cout << "RebotDummyServer exits gracefully." << std::endl;

  return 0;
}

