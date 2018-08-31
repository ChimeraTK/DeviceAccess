#include "TcpCtrl.h"
#include "Exception.h"

namespace ChimeraTK{

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
      
      // Try connecting to the first working endpoint in the list returned by
      // the resolver.
      // TODO: let boost asio throw instead of handling error through 'ec'.
      ec = connectToResolvedEndPoints(endPointIterator);
      if (ec) {
        throw ChimeraTK::runtime_error("Could not connect to server");
      }
    }
    catch (std::exception &exception) {
      throw ChimeraTK::runtime_error(exception.what());
    }
  }
  
  void TcpCtrl::closeConnection() {
    boost::system::error_code ec;
    _socket->close(ec);
    if (ec) {
      throw ChimeraTK::runtime_error("Error closing socket");
    }
  }
  
  std::vector<int32_t> TcpCtrl::receiveData(uint32_t const &numWordsToRead) {
    try {
      std::vector<int32_t> readData(numWordsToRead);
      boost::asio::read(*_socket, boost::asio::buffer(readData));
      return readData;
    }
    catch (...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error reading from socket");
    }
  }
  
  void TcpCtrl::sendData(const std::vector<char> &data) {
    try {
      boost::asio::write(*_socket, boost::asio::buffer(&data[0], data.size()));
    }
    catch (...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error writing to socket");
    }
  }
  
  void TcpCtrl::sendData(const std::vector<uint32_t> &data) {
    try {
      boost::asio::write(*_socket, boost::asio::buffer(data));
    }
    catch (...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error writing to socket");
    }
  }
  
  std::string TcpCtrl::getAddress() { return _serverAddress; }
  
  void TcpCtrl::setAddress(std::string ipaddr) {
    if (_socket->is_open()) {
      throw ChimeraTK::logic_error("Error setting IP. The socket is open");
    }
    _serverAddress = ipaddr;
  }
  
  int TcpCtrl::getPort() { return _port; }
  
  void TcpCtrl::setPort(int port) {
    if (_socket->is_open()) {
      throw ChimeraTK::logic_error("Error setting port. The socket is open");
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
    try {
      boost::asio::read(*_socket, boost::asio::buffer(receivedArray));
    }
    catch (...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error reading from socket");
    }
  }

}//namespace ChimeraTK
