#ifndef SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_
#define SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_

#include <string>
#include "DummyBackend.h"
#include <boost/asio.hpp>

namespace ip = boost::asio::ip;

namespace mtca4u {

extern bool sigterm_caught;

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

  static const int BUFFER_SIZE_IN_WORDS = 256;
  static const int32_t READ_SUCCESS_INDICATION = 1000;
  static const int32_t WRITE_SUCCESS_INDICATION = 1001;
  static const int32_t TOO_MUCH_DATA_REQUESTED = -1010;
  static const int32_t UNKNOWN_INSTRUCTION = -1040;



  DummyBackend _registerSpace;
  unsigned int _serverPort;
  boost::asio::io_service _io;
  ip::tcp::endpoint _serverEndpoint;
  ip::tcp::acceptor _connectionAcceptor;
  boost::shared_ptr<ip::tcp::socket> _currentClientConnection;

  void processReceivedCommand(std::vector<uint32_t> &buffer);
  bool writeWordToRequestedAddress(std::vector<uint32_t> &buffer);
  void readRegisterAndSendData(std::vector<uint32_t> &buffer);
  void sendResponseForWriteCommand(bool status);
  void handleAcceptedConnection(boost::shared_ptr<ip::tcp::socket>& );
};

} /* namespace mtca4u */

#endif /* SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_ */
