/*
 * TcpCtrl.cc
 *
 *  Created on: May 27, 2015
 *      Author: adworz
 */

#include "TcpCtrl.h"

using namespace mtca4u;
namespace boost_ip = boost::asio::ip;
typedef boost::asio::ip::tcp::resolver  resolver_t;
typedef boost::asio::ip::tcp::resolver::query  query_t;
typedef boost_ip::tcp::resolver::iterator  iterator_t;

TcpCtrl::TcpCtrl(std::string address, int port)
: _serverAddress(address), _port(port) {
  _io_service = boost::make_shared<boost::asio::io_service>();
  _socket = boost::make_shared<boost_ip::tcp::socket>(*_io_service);
}

TcpCtrl::~TcpCtrl() {}

void TcpCtrl::openConnection() {
  try {
    // Use boost resolver for DNS name resolution when server address is a
    // hostname
    resolver_t dnsResolver(*_io_service);
    query_t query(_serverAddress, std::to_string(_port));
    iterator_t endPointIterator = dnsResolver.resolve(query);

    boost::system::error_code ec;
    int connectionCounter = 0;

    while (1) {
      // Try connecting to the first working endpoint in the list returned by
      // the resolver and retry 30 times if all endpoints in the list
      // fails to connect (FIXME: why 30? why retry anyway).
      ec = connectToResolvedEndPoints(endPointIterator);
      if (!ec || connectionCounter > 30) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      connectionCounter++;
    }
    // we are unsuccessful after 30 retries
    if (ec) {
      throw RebotBackendException("Could not connect to server",
          RebotBackendException::EX_CONNECTION_FAILED);
    }
  }
  catch (std::exception &exception) {
    throw RebotBackendException(exception.what(),
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

std::vector<int32_t> TcpCtrl::receiveData(uint32_t const &numWordsToRead) {
  boost::system::error_code ec;
  std::vector<int32_t> readData(numWordsToRead);
  _socket->read_some(boost::asio::buffer(readData), ec);
  if (ec) {
    throw RebotBackendException("Error reading from socket",
        RebotBackendException::EX_SOCKET_READ_FAILED);
  } else {
    return readData;
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

std::string TcpCtrl::getAddress() { return _serverAddress; }

void TcpCtrl::setAddress(std::string ipaddr) {
  if (_socket->is_open()) {
    throw RebotBackendException("Error setting IP. The socket is open",
        RebotBackendException::EX_SET_IP_FAILED);
  }
  _serverAddress = ipaddr;
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
  for (; endpointIterator != resolver_t::iterator(); ++endpointIterator) {
    _socket->close();
    _socket->connect(*endpointIterator, ec);
    if (ec == boost::system::errc::success) { // return if connection to server
      // successful
      return ec;
    }
  }
  // Reaching here implies We failed to connect to every endpoint in the list.
  // Indicate connection error in this case
  ec = boost::asio::error::not_found;
  return ec;
}

// FIXME: remove this later; Do not want to do a cleanup at this point
void TcpCtrl::receiveData(boost::array<char, 4> &receivedArray) {
  boost::system::error_code ec;
  _socket->read_some(boost::asio::buffer(receivedArray), ec);
  if (ec) {
    throw RebotBackendException("Error reading from socket",
        RebotBackendException::EX_SOCKET_READ_FAILED);
  }
}
