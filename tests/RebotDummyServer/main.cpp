// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "argumentParser.h"
#include "RebotDummyServer.h"

#include <iostream>

int main(int, char** argv) {
  unsigned int portNumber = getPortNumber(argv);
  std::string mapFileLocation = getMapFileLocation(argv);
  unsigned int protocolVersion = getProtocolVersion(argv);

  ChimeraTK::RebotDummyServer testServer(portNumber, mapFileLocation, protocolVersion);
  boost::asio::signal_set signals(testServer.service(), SIGINT, SIGTERM);
  signals.async_wait([&testServer](const boost::system::error_code& error, int /* signal */) {
    if(not error) {
      testServer.stop();
    }
  });

  std::cout << "Rebot dummy server started" << std::endl;
  std::cout << "PORT " << testServer.port() << std::endl;
  testServer.start();
  std::cout << "Rebot dummy server stopped" << std::endl;

  return 0;
}
