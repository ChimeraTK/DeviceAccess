#ifndef SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_
#define SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_

#include "DummyBackend.h"
#include "DummyProtocolImplementor.h"

#include <memory>

#include <atomic>
#include <boost/asio.hpp>
#include <string>

namespace ip = boost::asio::ip;

namespace ChimeraTK {
  using namespace ChimeraTK;
}

namespace ChimeraTK {
  using namespace ChimeraTK;

  extern std::atomic<bool> stop_rebot_server;

  /*
   * starts a blocking Rebot server on localhost:port. where port is the
   * portNumber specified during object creation.
   */
  class RebotDummySession : public std::enable_shared_from_this<RebotDummySession> {
    // everything is public so all protocol implementors can reach it. They are
    // only called from within the server
  public:
    RebotDummySession(unsigned int protocolVersion, ip::tcp::socket socket, std::shared_ptr<DummyBackend> regsiterSpace);
    void start();
    virtual ~RebotDummySession();

    // The following stuff is only intended for the protocol implementors and the
    // server itself

    static const int BUFFER_SIZE_IN_WORDS = 256;
    static const int32_t READ_SUCCESS_INDICATION = 1000;
    static const int32_t WRITE_SUCCESS_INDICATION = 1001;
    static const uint32_t PONG = 1005;
    static const int32_t TOO_MUCH_DATA_REQUESTED = -1010;
    static const int32_t UNKNOWN_INSTRUCTION = -1040;

    static const uint32_t SINGLE_WORD_WRITE = 1;
    static const uint32_t MULTI_WORD_WRITE = 2;
    static const uint32_t MULTI_WORD_READ = 3;
    static const uint32_t HELLO = 4;
    static const uint32_t PING = 5;
    static const uint32_t REBOT_MAGIC_WORD = 0x72626f74; // ascii code 'rbot'

    // internal states. Currently there are only two when the connection is open
    static const uint32_t ACCEPT_NEW_COMMAND = 1;
    // a multi word write can be so large that it needs more than one package
    static const uint32_t INSIDE_MULTI_WORD_WRITE = 2;

    // The actual state: ready for new command or not
    uint32_t _state;

    std::atomic<uint32_t> _heartbeatCount;
    std::atomic<uint32_t> _helloCount; // in protocol version 1 we have to send
                                       // hello instead of heartbeat
    std::atomic<bool> _dont_answer;    // flag to cause an error condition
    std::shared_ptr<DummyBackend> _registerSpace;
    std::vector<uint32_t> _dataBuffer;

    unsigned int _serverPort;
    unsigned int _protocolVersion;
    ip::tcp::socket _currentClientConnection;
    std::unique_ptr<DummyProtocolImplementor> _protocolImplementor;

    void processReceivedPackage(std::vector<uint32_t>& buffer);
    void writeWordToRequestedAddress(std::vector<uint32_t>& buffer);
    void readRegisterAndSendData(std::vector<uint32_t>& buffer);

    // most commands have a single word as response. Avoid code duplication.
    void sendSingleWord(int32_t response);
    void acceptHandler(const boost::system::error_code &error);
    void doRead();
    void doWrite();
    void write(std::vector<uint32_t> data);
  };

  class RebotDummyServer {
  public:
    RebotDummyServer(unsigned int portNumber, std::string mapFile, unsigned int protocolVersion);

    void start();
    void stop();
    boost::asio::io_service& service() { return _io; }
    std::shared_ptr<RebotDummySession> session() { return _currentSession.lock(); }

  private:
    void do_accept();
    unsigned int _protocolVersion;
    boost::asio::io_service _io;
    ip::tcp::acceptor _connectionAcceptor;
    std::weak_ptr<RebotDummySession> _currentSession;
    ip::tcp::socket _socket;
    std::shared_ptr<DummyBackend> _registerSpace;
  };

} /* namespace ChimeraTK */

#endif /* SOURCE_DIRECTORY__TESTS_REBOTDUMMYSERVER_REBOTDUMMYSERVER_H_ */
