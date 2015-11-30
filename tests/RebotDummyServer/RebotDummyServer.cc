
#include "RebotDummyServer.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace mtca4u {

RebotDummyServer::RebotDummyServer(unsigned int& portNumber,
                                   std::string& mapFile)
    : _registerSpace(mapFile),
      _serverPort(portNumber),
      _io(),
      _serverEndpoint(ip::tcp::v4(), _serverPort),
      _connectionAcceptor(_io),
      _currentClientConnection() {
  // set the acceptor backlog to 1
  _connectionAcceptor.open(_serverEndpoint.protocol());
  _connectionAcceptor.bind(_serverEndpoint);
  _connectionAcceptor.listen(1);
  std::cout << "listen(1) outside constructor" << std::endl;

}

void RebotDummyServer::start() {

  while (true) { // loop accepts client connections - one at a time
    boost::shared_ptr<ip::tcp::socket> incomingConnection(new ip::tcp::socket(_io));
    _connectionAcceptor.accept(*incomingConnection);
    // http://www.boost.org/doc/libs/1_46_0/doc/html/boost_asio/example/echo/blocking_tcp_echo_server.cpp
    RebotDummyServer::handleAcceptedConnection(incomingConnection);
    //boost::asio::detail::thread t(boost::bind(&RebotDummyServer::handleAcceptedConnection, this, incomingConnection));
  }
}

void RebotDummyServer::processReceivedCommand(std::vector<uint32_t> &buffer) {

  uint32_t requestedAction = buffer.at(0);

  switch (requestedAction) {

    case 1: { // Write one word to a register
      bool status = writeWordToRequestedAddress(buffer);
      sendResponseForWriteCommand(status);
      break;
    }
    case 3: { // multi word read
      uint32_t numberOfWordsToRead = buffer.at(2);

      if (numberOfWordsToRead > 361) { // not supported
        std::vector<int32_t> dataToSend(1);
        dataToSend.at(0) = TOO_MUCH_DATA_REQUESTED;

        boost::asio::write(*_currentClientConnection, boost::asio::buffer(dataToSend));
      } else {
        readRegisterAndSendData(buffer);
      }
      break;
    }
  }
}

bool RebotDummyServer::writeWordToRequestedAddress(std::vector<uint32_t> &buffer) {
  uint32_t registerAddress = buffer.at(1);
  int32_t wordToWrite = buffer.at(2);
  uint8_t bar = 0;
  try {
    _registerSpace.write(bar, registerAddress, &wordToWrite,
                         sizeof(wordToWrite));
    return true;
  }
  catch (...) {
    return false;
  }
}

void RebotDummyServer::readRegisterAndSendData(std::vector<uint32_t> &buffer) {
  uint32_t registerAddress = buffer.at(1);
  uint32_t numberOfWordsToRead = buffer.at(2);

  std::vector<int32_t> dataToSend(numberOfWordsToRead + 1); // +1 to accomodate
                                                            // READ_SUCCESS_INDICATION
  dataToSend.at(0) = READ_SUCCESS_INDICATION;

  uint8_t bar = 0;
  // start putting in the read values from location dataToSend[1]:
  int32_t* startAddressForReadInData = dataToSend.data() + 1;
  _registerSpace.read(bar, registerAddress, startAddressForReadInData,
                      numberOfWordsToRead * sizeof(int32_t));

  // FIXME: Nothing in protocol to indicate read failure.
  boost::asio::write(*_currentClientConnection, boost::asio::buffer(dataToSend));
}

void RebotDummyServer::sendResponseForWriteCommand(bool status) {
  if (status == true) { // WriteSuccessful
    std::vector<int32_t> data(1);
    data.at(0) = WRITE_SUCCESS_INDICATION;
    boost::asio::write(*_currentClientConnection, boost::asio::buffer(data));
  }// else {
    // FIXME: We currently have nothing for write failure
  //}
}

RebotDummyServer::~RebotDummyServer() {
  _connectionAcceptor.close();
  _currentClientConnection->close();
}

void RebotDummyServer::handleAcceptedConnection(
     boost::shared_ptr<ip::tcp::socket>& incomingSocket) {
  _currentClientConnection = incomingSocket;

  while (true) { // This loop handles the accepted connection

    std::vector<uint32_t> dataBuffer(BUFFER_SIZE_IN_WORDS);
    boost::system::error_code errorCode;
    _currentClientConnection->read_some(boost::asio::buffer(dataBuffer), errorCode);
   // FIXME: Will fail if the command is sent in two successive tcp packets

    if (errorCode == boost::asio::error::eof) { // The client has closed the
                                                // connection; move to the
                                                // outer  loop to accept new
                                                // connections
      _currentClientConnection->close();
      std::cout << "connection closed" << std::endl;
      break;
    } else if (errorCode) {
      throw boost::system::system_error(errorCode);
    }

    processReceivedCommand(dataBuffer);
  }
}

} /* namespace mtca4u */

