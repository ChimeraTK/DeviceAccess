
#include "RebotDummyServer.h"
#include "DummyProtocol1.h" // the latest version includes all predecessors in the include
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <stdexcept>

namespace ChimeraTK {

  std::atomic<bool> stop_rebot_server(false);

  RebotDummySession::RebotDummySession(unsigned int protocolVersion, ip::tcp::socket socket, std::shared_ptr<DummyBackend> registerSpace)
  : _state(ACCEPT_NEW_COMMAND), _heartbeatCount(0), _helloCount(0), _dont_answer(false), _registerSpace(registerSpace),
        _protocolVersion(protocolVersion), _currentClientConnection(std::move(socket))  {
    if(protocolVersion == 0) {
      _protocolImplementor.reset(new DummyProtocol0(*this));
    }
    else if(protocolVersion == 1) {
      _protocolImplementor.reset(new DummyProtocol1(*this));
    }
    else {
      throw std::invalid_argument("RebotDummyServer: unknown protocol version");
    }
  }

  /********************************************************************************************************************/

  void RebotDummySession::start() {
    doRead();
  }

  void RebotDummySession::doRead() {
    auto self(shared_from_this());
    std::shared_ptr<std::vector<uint32_t>> buffer;
    buffer.reset(new std::vector<uint32_t>(RebotDummySession::BUFFER_SIZE_IN_WORDS));

    _currentClientConnection.async_read_some(
      boost::asio::buffer(*buffer),
      [this, self, buffer](boost::system::error_code ec, std::size_t) {
        if (ec) {
          _currentClientConnection.close();
        } else {
          processReceivedPackage(*buffer);
        }
      }
    );
  }

  void RebotDummySession::doWrite() {
      if (_dataBuffer.empty()) {
        doRead();
        return;
      }

      auto self(shared_from_this());
      _currentClientConnection.async_write_some(
        boost::asio::buffer(_dataBuffer),
        [this, self](boost::system::error_code ec, std::size_t) {
          if (not ec) {
            _dataBuffer.clear();
          } else {
              std::cerr << ec << std::endl;
              _currentClientConnection.close();
          }
          doRead();
        }
      );
  }

  /********************************************************************************************************************/

  void RebotDummySession::processReceivedPackage(std::vector<uint32_t>& buffer) {
    if(_state == INSIDE_MULTI_WORD_WRITE) {
      _state = _protocolImplementor->continueMultiWordWrite(buffer);
      doWrite();
      return;
    }

    // cause an error condition: just don't answer
    if(_dont_answer) {
      doRead();
      return;
    }

    uint32_t requestedAction = buffer.at(0);
    switch(requestedAction) {
    case SINGLE_WORD_WRITE:
        _protocolImplementor->singleWordWrite(buffer);
        break;
    case MULTI_WORD_WRITE:
        _state = _protocolImplementor->multiWordWrite(buffer);
        break;
    case MULTI_WORD_READ:
        _protocolImplementor->multiWordRead(buffer);
        break;
    case HELLO:
        ++_helloCount;
        _protocolImplementor->hello(buffer);
        break;
    case PING:
        ++_heartbeatCount;
        _protocolImplementor->ping(buffer);
        break;
    default:
        std::cout << "Instruction unknown in all protocol versions " << requestedAction << std::endl;
        sendSingleWord(UNKNOWN_INSTRUCTION);
    }

    doWrite();
  }

  /********************************************************************************************************************/

  void RebotDummySession::writeWordToRequestedAddress(std::vector<uint32_t>& buffer) {
    uint32_t registerAddress = buffer.at(1); // This is the word offset; since
    // dummy device deals with byte
    // addresses convert to bytes FIXME
    registerAddress = registerAddress * 4;
    int32_t wordToWrite = buffer.at(2);
    uint8_t bar = 0;
    _registerSpace->write(bar, registerAddress, &wordToWrite, sizeof(wordToWrite));
  }

  /********************************************************************************************************************/

  void RebotDummySession::readRegisterAndSendData(std::vector<uint32_t>& buffer) {
    uint32_t registerAddress = buffer.at(1); // This is a word offset. convert to bytes before use. FIXME
    registerAddress = registerAddress * 4;
    uint32_t numberOfWordsToRead = buffer.at(2);

    // send data in two packets instead of one; this is done for test coverage.
    // Let READ_SUCCESS_INDICATION go in the first write and data in the second.
    sendSingleWord(READ_SUCCESS_INDICATION);

    std::vector<uint32_t> dataToSend(numberOfWordsToRead);
    uint8_t bar = 0;
    // start putting in the read values from location dataToSend[1]:
    int32_t* startAddressForReadInData = reinterpret_cast<int32_t*>(dataToSend.data());
    _registerSpace->read(bar, registerAddress, startAddressForReadInData, numberOfWordsToRead * sizeof(int32_t));

    // FIXME: Nothing in protocol to indicate read failure.
    write(dataToSend);
  }

  void RebotDummySession::write(std::vector<uint32_t> dataToSend) {
    _dataBuffer.insert(_dataBuffer.end(), dataToSend.begin(), dataToSend.end());
  }

  /********************************************************************************************************************/

  void RebotDummySession::sendSingleWord(int32_t response) {
    _dataBuffer.push_back(response);
  }

  /********************************************************************************************************************/

  RebotDummySession::~RebotDummySession() {
    // if the terminate signal is caught while waiting for a connection
    // there is no client connection, so we have to check the pointer here
    _currentClientConnection.close();
  }

  /********************************************************************************************************************/

  RebotDummyServer::RebotDummyServer(unsigned int portNumber, std::string mapFile, unsigned int protocolVersion)
      : _protocolVersion(protocolVersion), _io(),
        _connectionAcceptor(_io, ip::tcp::endpoint(ip::tcp::v4(), portNumber)), _socket(_io),
        _registerSpace(std::make_shared<DummyBackend>(mapFile)) {
    // The first address of the register space is set to a reference value. This
    // would be used to test the rebot client.
    uint32_t registerAddress = 0x04;
    int32_t wordToWrite = 0xDEADBEEF; // Change this to someting standardized
                                      // later (eg FW version ..)
    uint8_t bar = 0;
    _registerSpace->open();
    _registerSpace->write(bar, registerAddress, &wordToWrite, sizeof(wordToWrite));
  }

  void RebotDummyServer::do_accept() {
    _connectionAcceptor.async_accept(
      _socket,
      [this](boost::system::error_code ec) {
        if (not ec) {
          if (_currentSession.expired()) {
            auto newSession = std::make_shared<RebotDummySession>(_protocolVersion, std::move(_socket), _registerSpace);
            _currentSession = newSession;
            newSession->start();
          } else {
            auto code = boost::system::errc::make_error_code(boost::system::errc::connection_refused);
            _socket.close(code);
          }
        }
        do_accept();
      }
    );
  }

  void RebotDummyServer::start() {
    do_accept();
    _io.run();
  }

  void RebotDummyServer::stop() {
    _io.stop();
  }

} /* namespace ChimeraTK */
