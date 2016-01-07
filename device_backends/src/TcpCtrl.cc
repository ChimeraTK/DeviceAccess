/*
 * TcpCtrl.cpp
 *
 *  Created on: May 27, 2015
 *      Author: adworz
 */

#include "TcpCtrl.h"
using namespace mtca4u;
using namespace boost::asio::ip;

TcpCtrl::TcpCtrl(std::string ipaddr, int port)
    : _ipAddress(ipaddr), _port(port) {
  _io_service = boost::make_shared<boost::asio::io_service>();
  _socket = boost::make_shared<tcp::socket>(*_io_service);
}

TcpCtrl::~TcpCtrl() {}

void TcpCtrl::openConnection() {

  boost::asio::ip::tcp::resolver resolver(*_io_service);
  boost::asio::ip::tcp::resolver::query query(_ipAddress, std::to_string(_port));
  boost::asio::ip::tcp::resolver::iterator endPointIterator = resolver.resolve(query);

  boost::system::error_code ec;
  int connectionCounter = 0;
  while (1) {
    // Try connecting to the first working endpoint in the list returned by the
    // resolver and retry 30 times(FIXME: why 30) if all endpoints in the list
    // fails to connect.
    ec = connectToResolvedEndPoints(endPointIterator);
    if (!ec || connectionCounter > 30) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    connectionCounter++;
  }

  if (ec) {
    throw RebotBackendException("Error connecting endpoint",
                                RebotBackendException::EX_CONNECTION_FAILED);
  }
}

void TcpCtrl::closeConnection() {
  boost::system::error_code ec;
  _socket->close(ec);
  if (ec) {
    throw RebotBackendException("Error closing socket",
                                RebotBackendException::EX_CLOSE_SOCKET_FAILED);
  }
}

void TcpCtrl::receiveData(boost::array<char, 4> &receivedArray) {
  boost::system::error_code ec;
  _socket->read_some(boost::asio::buffer(receivedArray), ec);
  if (ec) {
    throw RebotBackendException("Error reading from socket",
                                RebotBackendException::EX_SOCKET_READ_FAILED);
  }
}

void TcpCtrl::sendData(const std::vector<char> &data) {
  boost::system::error_code ec;
  _socket->write_some(boost::asio::buffer(&data[0], data.size()), ec);
  if (ec) {
    throw RebotBackendException("Error writing to socket",
                                RebotBackendException::EX_SOCKET_WRITE_FAILED);
  }
}

std::string TcpCtrl::getAddress() { return _ipAddress; }

void TcpCtrl::setAddress(std::string ipaddr) {
  if (_socket->is_open()) {
    throw RebotBackendException("Error setting IP. The socket is open",
                                RebotBackendException::EX_SET_IP_FAILED);
  }
  _ipAddress = ipaddr;
}

int TcpCtrl::getPort() { return _port; }

void TcpCtrl::setPort(int port) {
  if (_socket->is_open()) {
    throw RebotBackendException("Error setting port. The socket is open",
                                RebotBackendException::EX_SET_PORT_FAILED);
  }
  _port = port;
}

boost::system::error_code TcpCtrl::connectToResolvedEndPoints(
    boost::asio::ip::tcp::resolver::iterator endpointIterator) {
  boost::system::error_code ec;
  for (; endpointIterator != tcp::resolver::iterator(); ++endpointIterator) {
    _socket->close();
    _socket->connect(*endpointIterator, ec);
    if (ec == boost::system::errc::success) { // conncetion successful
      return ec;
    }
  }
  // Reaching here implies We failed to connect to every endpoint in the list.
  // Indicate connection error in this case
  ec = boost::asio::error::not_found;
  return ec;
}
