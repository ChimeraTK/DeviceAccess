
#include "RebotDummyServer.h"
#include "DummyProtocol1.h" // the latest version includes all predecessors in the include
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
 
namespace ChimeraTK {

bool volatile sigterm_caught = false;

  RebotDummyServer::RebotDummyServer(unsigned int& portNumber, std::string& mapFile,
                                     unsigned int protocolVersion)
    :  _state(ACCEPT_NEW_COMMAND),
       _registerSpace(mapFile),
      _serverPort(portNumber),
      _protocolVersion(protocolVersion),
      _io(),
      _serverEndpoint(ip::tcp::v4(), _serverPort),
      _connectionAcceptor(_io, _serverEndpoint),
      _currentClientConnection() {

    if (protocolVersion == 0){
      _protocolImplementor.reset(new DummyProtocol0(*this));
    }else if (protocolVersion == 1){
      _protocolImplementor.reset(new DummyProtocol1(*this));
    }else{
      throw std::invalid_argument("RebotDummyServer: unknown protocol version");
    }
      
  // set the acceptor backlog to 1
  /*  _connectionAcceptor.open(_serverEndpoint.protocol());
    _connectionAcceptor.bind(_serverEndpoint);*/
  _connectionAcceptor.listen(1);

  // The first address of the register space is set to a reference value. This
  // would be used to test the rebot client.
  uint32_t registerAddress = 0x04;
  int32_t wordToWrite = 0xDEADBEEF; // Change this to someting standardized
                                    // later (eg FW version ..)
  uint8_t bar = 0;
  _registerSpace.open();
  _registerSpace.write(bar, registerAddress, &wordToWrite, sizeof(wordToWrite));
}

void RebotDummyServer::start() {

  while (sigterm_caught ==
         false) { // loop accepts client connections - one at a time

    boost::shared_ptr<ip::tcp::socket> incomingConnection(
        new ip::tcp::socket(_io));
    try {
      _connectionAcceptor.accept(*incomingConnection);
    }
    catch (boost::system::system_error& e) {
      if (sigterm_caught) {
        break; // exit gracefully
      } else {
        throw; // something else went wrong, rethrow the exception
      }
    }

    // http://www.boost.org/doc/libs/1_46_0/doc/html/boost_asio/example/echo/blocking_tcp_echo_server.cpp
    RebotDummyServer::handleAcceptedConnection(incomingConnection);
    // boost::asio::detail::thread
    // t(boost::bind(&RebotDummyServer::handleAcceptedConnection, this,
    // incomingConnection));
  }
}

void RebotDummyServer::processReceivedPackage(std::vector<uint32_t>& buffer) {

  // FIXME: this whole method is a hack. The whole data handling part of the server
  // needs a  redesign
  if (_state == INSIDE_MULTI_WORD_WRITE){
    _state = _protocolImplementor->continueMultiWordWrite(buffer);
  }else{// has to be ACCEPT_NEW_COMMAND

    uint32_t requestedAction = buffer.at(0);
    switch (requestedAction) {
      
      case SINGLE_WORD_WRITE: 
        _protocolImplementor->singleWordWrite(buffer);
        break;
      case  MULTI_WORD_WRITE:
        _state = _protocolImplementor->multiWordWrite(buffer);
        break;
      case  MULTI_WORD_READ:
        _protocolImplementor->multiWordRead(buffer);
        break;
      case  HELLO:
        _protocolImplementor->hello(buffer);
        break;
      case  PING:
        _protocolImplementor->ping(buffer);
        break;
      default:
        std::cout << "Instruction unknown in all protocol versions " << requestedAction << std::endl;
        sendSingleWord(UNKNOWN_INSTRUCTION);
    }
  }
}

void RebotDummyServer::writeWordToRequestedAddress(
    std::vector<uint32_t>& buffer) {
  uint32_t registerAddress = buffer.at(1); // This is the word offset; since
                                           // dummy device deals with byte
                                           // addresses convert to bytes FIXME
  registerAddress = registerAddress * 4;
  int32_t wordToWrite = buffer.at(2);
  uint8_t bar = 0;
  _registerSpace.write(bar, registerAddress, &wordToWrite, sizeof(wordToWrite));
}

void RebotDummyServer::readRegisterAndSendData(std::vector<uint32_t>& buffer) {
  uint32_t registerAddress =
      buffer.at(1); // This is a word offset. convert to bytes before use. FIXME
  registerAddress = registerAddress * 4;
  uint32_t numberOfWordsToRead = buffer.at(2);

  // send data in two packets instead of one; this is done for test coverage.
  // Let READ_SUCCESS_INDICATION go in the first write and data in the second.
  sendSingleWord(READ_SUCCESS_INDICATION);

  std::vector<int32_t> dataToSend(numberOfWordsToRead);
  uint8_t bar = 0;
  // start putting in the read values from location dataToSend[1]:
  int32_t* startAddressForReadInData = dataToSend.data();
  _registerSpace.read(bar, registerAddress, startAddressForReadInData,
                      numberOfWordsToRead * sizeof(int32_t));

  // FIXME: Nothing in protocol to indicate read failure.
  boost::asio::write(*_currentClientConnection,
                     boost::asio::buffer(dataToSend));
}
  
void RebotDummyServer::sendSingleWord(int32_t response) {
  std::vector<int32_t> data{ response };
  boost::asio::write(*_currentClientConnection, boost::asio::buffer(data));
}

RebotDummyServer::~RebotDummyServer() {
  _connectionAcceptor.close();

  // if the terminate signal is caught while waiting for a connection
  // there is no client connection, so we have to check the pointer here
  if (_currentClientConnection) {
    _currentClientConnection->close();
  }
}

void RebotDummyServer::handleAcceptedConnection(
    boost::shared_ptr<ip::tcp::socket>& incomingSocket) {
  boost::asio::ip::tcp::no_delay option(true);
  incomingSocket->set_option(option);
  _currentClientConnection = incomingSocket;

  while (sigterm_caught == false) { // This loop handles the accepted connection

    std::vector<uint32_t> dataBuffer(BUFFER_SIZE_IN_WORDS);
    boost::system::error_code errorCode;
    _currentClientConnection->read_some(boost::asio::buffer(dataBuffer),
                                        errorCode);
    // FIXME: Will fail if the command is sent in two successive tcp packets

    if (errorCode == boost::asio::error::eof) { // The client has closed the
                                                // connection; move to the
                                                // outer  loop to accept new
                                                // connections
      _currentClientConnection->close();
      //std::cout << "connection closed" << std::endl;
      break;
    } else if (errorCode && sigterm_caught) { // reading was interrupted by a
                                              // terminate signal;
      // move to the outer loop which will also quit
      // and end the server gracefully
      _currentClientConnection->close();
      std::cout << "Terminate signal detected while reading. Connection "
                   "closed, will exit now." << std::endl;
      break;
    } else if (errorCode) {
      throw boost::system::system_error(errorCode);
    }

    processReceivedPackage(dataBuffer);
  }
}

} /* namespace ChimeraTK */
