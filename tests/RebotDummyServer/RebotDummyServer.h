#ifndef SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_
#define SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_

#include <string>
#include "DummyBackend.h"
#include <boost/asio.hpp>
#include "DummyProtocolImplementor.h"

namespace ip = boost::asio::ip;

namespace ChimeraTK {
  using namespace mtca4u;
  
extern bool volatile sigterm_caught;

/*
 * starts a blocking Rebot server on localhost:port. where port is the
 * portNumber specified during object creation.
 */
class RebotDummyServer {

  // everything is public so all protocol implementors can reach it. They are only called from
  // within the server
 public:
  RebotDummyServer(unsigned int &portNumber, std::string &mapFile, unsigned int protocolVersion);
  void start();
  virtual ~RebotDummyServer();

  // The following stuff is only intended for the protocol implementors and the server itself

  static const int BUFFER_SIZE_IN_WORDS = 256;
  static const int32_t READ_SUCCESS_INDICATION = 1000;
  static const int32_t WRITE_SUCCESS_INDICATION = 1001;
  static const int32_t TOO_MUCH_DATA_REQUESTED = -1010;
  static const int32_t UNKNOWN_INSTRUCTION = -1040;

  static const uint32_t SINGLE_WORD_WRITE = 1;
  static const uint32_t MULTI_WORD_WRITE = 2;
  static const uint32_t MULTI_WORD_READ = 3;
  static const uint32_t HELLO = 4;
  static const uint32_t REBOT_MAGIC_WORD = 0x72626f74; // ascii code 'rbot'
  
  // internal states. Currently there are only two when the connection is open
  static const uint32_t ACCEPT_NEW_COMMAND = 1;
  // a multi word write can be so large that it needs more than one package
  static const uint32_t INSIDE_MULTI_WORD_WRITE = 2;

  // The actual state: ready for new command or not
  uint32_t _state;

  DummyBackend _registerSpace;
  unsigned int _serverPort;
  unsigned int _protocolVersion;
  boost::asio::io_service _io;
  ip::tcp::endpoint _serverEndpoint;
  ip::tcp::acceptor _connectionAcceptor;
  boost::shared_ptr<ip::tcp::socket> _currentClientConnection;
  std::unique_ptr<DummyProtocolImplementor> _protocolImplementor;
  
  void processReceivedPackage(std::vector<uint32_t> &buffer);
  void writeWordToRequestedAddress(std::vector<uint32_t> &buffer);
  void readRegisterAndSendData(std::vector<uint32_t> &buffer);
  void handleAcceptedConnection(boost::shared_ptr<ip::tcp::socket>& );
  // most commands have a singe work as response. Avoid code duplication.
  void sendSingleWord(int32_t response);
};

} /* namespace ChimeraTK */

#endif /* SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_ */
