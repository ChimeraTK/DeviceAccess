/*
 * RebotDummyServer.h
 *
 *  Created on: Nov 23, 2015
 *      Author: varghese
 */

#ifndef SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_
#define SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_

#include <string>
#include "DummyBackend.h"

namespace mtca4u {



class RebotDummyServer {
private:
  DummyBackend _registerSpace;
  unsigned int _serverPort;

public:
  RebotDummyServer(unsigned int &portNumber, std::string &mapFile);
  void start();
  virtual ~RebotDummyServer();
};

} /* namespace mtca4u */

#endif /* SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_ */
