/*
 * RebotDummyServer.cpp
 *
 *  Created on: Nov 20, 2015
 *      Author: varghese
 */

#include <iostream>

#include "RebotDummyServer.h"
#include "argumentParser.h"


int main(int, char** argv) {
  unsigned int portNumber = getPortNumber(argv);
  std::string mapFileLocation = getMapFileLocation(argv);

  mtca4u::RebotDummyServer testServer(portNumber, mapFileLocation);
  testServer.start();

  return 0;
}
