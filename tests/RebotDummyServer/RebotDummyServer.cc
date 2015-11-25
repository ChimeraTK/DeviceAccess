/*
 * RebotDummyServer.cc
 *
 *  Created on: Nov 23, 2015
 *      Author: varghese
 */

#include "RebotDummyServer.h"

#include <iostream>
#include <thread>


namespace mtca4u {

RebotDummyServer::RebotDummyServer(unsigned int& portNumber,
                                   std::string& mapFile)
    : _registerSpace(mapFile),
      _serverPort(portNumber),
      _io(),
      _serverEndpoint(ip::tcp::v4(), _serverPort),
      _connectionAcceptor(_io, _serverEndpoint),
      _incomingConnection(_io) {}

void RebotDummyServer::start() {

  while (true) { // loop accepts client connections - one at a time
    _connectionAcceptor.accept(_incomingConnection);
    while (true) { // This loop handles the accepted connection
      char dataBuffer[MAX_SEGMENT_LENGTH_IN_BYTES];
      boost::system::error_code status;
      _incomingConnection.read_some(boost::asio::buffer(dataBuffer), status);
      if (status == boost::asio::error::eof) { // The client has closed the
                                               // connection; move to the outer
                                               // loop to accept new connections
        break;
      } else if (status) {
        throw boost::system::system_error(status);
      }
      processReceivedCommand(dataBuffer);
    }
  }
}

void RebotDummyServer::processReceivedCommand(char* buffer) {
  uint32_t dataBuffer[MAX_SEGMENT_LENGTH_IN_BYTES / 4];
  memcpy(dataBuffer, buffer, sizeof(dataBuffer));

  uint32_t requestedAction = dataBuffer[0];

  switch (requestedAction) {
    case 1: { // Write one word to a register
      bool status = writeWordToRequestedAddress(dataBuffer);
      sendResponseForWriteCommand(status);
      break;
    }
    case 3: { break; }
  }
}

bool RebotDummyServer::writeWordToRequestedAddress(uint32_t* buffer) {
  uint32_t registerAddress = buffer[1];
  int32_t wordToWrite = buffer[2];
  try {
    uint8_t bar = 0;
    _registerSpace.write(bar, registerAddress, &wordToWrite,
                         sizeof(wordToWrite));
    return true;
  }
  catch (...) {
    return false;
  }
}

void RebotDummyServer::sendResponseForWriteCommand(bool status) {
  int32_t data;
  if (status == true) { // WriteSuccessful
    data = WRITE_SUCCESS_INDICATION;
    boost::asio::write(sock, boost::asio::buffer(data, length));
  } else {
    // FIXME: We currently have nothing for write failure
  }
}

RebotDummyServer::~RebotDummyServer() {
  // TODO Auto-generated destructor stub
}

} /* namespace mtca4u */
