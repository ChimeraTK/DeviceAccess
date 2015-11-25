/*
 * RebotDummyServer.cc
 *
 *  Created on: Nov 23, 2015
 *      Author: varghese
 */

#include "RebotDummyServer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>



namespace mtca4u {



RebotDummyServer::RebotDummyServer(unsigned int& portNumber,
                                   std::string& mapFile)
    : _registerSpace(mapFile), _serverPort(portNumber) {}

void RebotDummyServer::start() {
  // open a tcp connection on localhost:port and listen for commands
  // reuse tcp control ?
  boost::shared_ptr<boost::asio::io_service> _io_service;
  boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;

  boost::system::error_code ec;
  boost::asio::ip::tcp::endpoint endp(boost::asio::ip::address::from_string("127.0.0.1"), _serverPort);

  int connectionCounter = 0;
  while (1) {
    _socket->connect(endp, ec);
    if (!ec || connectionCounter > 30) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    connectionCounter++;
  }
}

RebotDummyServer::~RebotDummyServer() {
  // TODO Auto-generated destructor stub
}

} /* namespace mtca4u */
