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
#include <boost/asio.hpp>

namespace ip = boost::asio::ip;

namespace mtca4u {


/*
 * starts a blocking Rebot server on localhost:port. where port is the
 * portNumber specified during object creation.
 */
class RebotDummyServer {

public:
  RebotDummyServer(unsigned int &portNumber, std::string &mapFile);
  void start();
  virtual ~RebotDummyServer();

private:

  static const int MAX_SEGMENT_LENGTH_IN_BYTES = 1024;
  static const int32_t READ_SUCCESS_INDICATION = 1000;
  static const int32_t WRITE_SUCCESS_INDICATION = 1001;
  static const int32_t TOO_MUCH_DATA_REQUESTED = -1010;
  static const int32_t UNKNOWN_INSTRUCTION = -1040;



  DummyBackend _registerSpace;
  unsigned int _serverPort;
  boost::asio::io_service _io;
  ip::tcp::endpoint _serverEndpoint;
  ip::tcp::acceptor _connectionAcceptor;
  ip::tcp::socket _incomingConnection;

  void processReceivedCommand(char* buffer);
  bool writeWordToRequestedAddress(uint32_t* buffer);
  void readRegisterAndSendData(uint32_t* buffer);
  void sendResponseForWriteCommand(bool status);
};

} /* namespace mtca4u */

#endif /* SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_ */
