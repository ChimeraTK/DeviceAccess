/*
 * TcpCtrl.cpp
 *
 *  Created on: May 27, 2015
 *      Author: adworz
 */

#include "TcpCtrl.h"
using boost::asio::ip::tcp;
using namespace mtca4u;

TcpCtrl::TcpCtrl(std::string ipaddr, int port) : _ipAddress(ipaddr), _port(port) {
  _io_service = boost::make_shared<boost::asio::io_service>();
  _socket = boost::make_shared<tcp::socket>(*_io_service);
}

TcpCtrl::~TcpCtrl() {

}

void TcpCtrl::openConnection() {
	
  boost::system::error_code ec;
  tcp::endpoint endp(boost::asio::ip::address::from_string(_ipAddress), _port);
  int connectionCounter=0;
  while(1) {
    _socket->connect(endp,ec);
    if(!ec || connectionCounter>30) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    connectionCounter++;
  }

  if (ec) {
    throw RebotBackendException("Error connecting endpoint", RebotBackendException::EX_CONNECTION_FAILED);
  }

}

void TcpCtrl::closeConnection() {
  boost::system::error_code ec;
  _socket->close(ec);
  if (ec) {
    throw RebotBackendException("Error closing socket", RebotBackendException::EX_CLOSESOCK);	
  }
}


void TcpCtrl::receiveData(boost::array<char, 4> &receivedArray) {
  boost::system::error_code ec;
  _socket->read_some(boost::asio::buffer(receivedArray), ec);
  if (ec) {
    throw RebotBackendException("Error reading from socket", RebotBackendException::EX_RDSOCK);
  }
}

void TcpCtrl::sendData(const std::vector<char> &data) {
  boost::system::error_code ec;
  _socket->write_some(boost::asio::buffer(&data[0],data.size()),ec);	
  if(ec) {
    throw RebotBackendException("Error writing to socket", RebotBackendException::EX_WRSOCK);
  }
}

std::string TcpCtrl::getAddress() {
  return _ipAddress;
}

void TcpCtrl::setAddress(std::string ipaddr) {
  if (_socket->is_open()) {
    throw RebotBackendException("Error setting IP. The socket is open", RebotBackendException::EX_SETIP);
  }
  _ipAddress = ipaddr;
}

int TcpCtrl::getPort() {
  return _port;
}

void TcpCtrl::setPort(int port) {
  if (_socket->is_open()) {
    throw RebotBackendException("Error setting port. The socket is open", RebotBackendException::EX_SETPORT);
  }
  _port = port;
}

