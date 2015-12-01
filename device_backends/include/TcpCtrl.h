/*
 * TcpCtrl.h
 *
 *  Created on: May 27, 2015
 *      Author: adagio
 */

#ifndef TCPCTRL_H_
#define TCPCTRL_H_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <RebotBackendException.h>

using boost::asio::ip::tcp;

///Handles the communication over TCP protocol with RebotDevice-based devices
class TcpCtrl {
private:
  std::string _ipAddress;
  int _port;
  boost::shared_ptr<boost::asio::io_service> _io_service;
  boost::shared_ptr<tcp::socket> _socket;

public:

  ///Gets an IP address and port of the device but does not open the connection
  TcpCtrl(std::string ipaddr, int port);
  ~TcpCtrl();
  ///Opens a connection to the device.
  void openConnection(); 
  ///Closes a connection with the device.
  void closeConnection();
  ///Receives 4 bytes from the socket.
  void receiveData(boost::array<char, 4> &receiveArray); 
  ///Sends a std::vector of bytes to the socket.
  void sendData(const std::vector<char> &data); 
  ///Returns an IP address associated with an object of the class.
  std::string getAddress();
  ///Sets an IP address in an object. Can be done when connection is closed.
  void setAddress(std::string ipaddr);
  ///Returns a port associated with an object of the class.
  int getPort();
  ///Sets port in an object. Can be done when connection is closed.
  void setPort(int port);


};

#endif /* TCPCTRL_H_ */
