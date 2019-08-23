#include "Connection.h"
#include "Exception.h"

namespace ChimeraTK {
namespace Rebot {

  namespace boost_ip = boost::asio::ip;
  typedef boost::asio::ip::tcp::resolver resolver_t;
  typedef boost::asio::ip::tcp::resolver::query query_t;
  typedef boost_ip::tcp::resolver::iterator iterator_t;

  Connection::Connection(std::string address, std::string port) : _serverAddress(address), _port(port) {
    _io_service = boost::make_shared<boost::asio::io_service>();
    _socket = boost::make_shared<boost_ip::tcp::socket>(*_io_service);
  }

  Connection::~Connection() {}

  void Connection::open() {
    try {
      // Use boost resolver for DNS name resolution when server address is a
      // hostname
      resolver_t dnsResolver(*_io_service);
      query_t query(_serverAddress, _port);
      iterator_t endPointIterator = dnsResolver.resolve(query);

      boost::system::error_code ec;

      // Try connecting to the first working endpoint in the list returned by
      // the resolver.
      // TODO: let boost asio throw instead of handling error through 'ec'.
      ec = connectToResolvedEndPoints(endPointIterator);
      if(ec) {
        throw ChimeraTK::runtime_error("Could not connect to server");
      }
    }
    catch(std::exception& exception) {
      throw ChimeraTK::runtime_error(exception.what());
    }
  }

  void Connection::close() {
    boost::system::error_code ec;
    _socket->close(ec);
    if(ec) {
      throw ChimeraTK::runtime_error("Error closing socket");
    }
  }

  std::vector<uint32_t> Connection::read(uint32_t const& numWordsToRead) {
    try {
      std::vector<uint32_t> readData(numWordsToRead);
      boost::asio::read(*_socket, boost::asio::buffer(readData));
      return readData;
    }
    catch(...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error reading from socket");
    }
  }

  void Connection::write(const std::vector<uint32_t>& data) {
    try {
      boost::asio::write(*_socket, boost::asio::buffer(data));
    }
    catch(...) {
      // TODO: find out how to extract info from the boost excception and wrap it
      // inside RebotBackendException
      throw ChimeraTK::runtime_error("Error writing to socket");
    }
  }

  boost::system::error_code Connection::connectToResolvedEndPoints(
      boost::asio::ip::tcp::resolver::iterator endpointIterator) {
    boost::system::error_code ec;
    for(; endpointIterator != resolver_t::iterator(); ++endpointIterator) {
      _socket->close();
      _socket->connect(*endpointIterator, ec);
      if(ec == boost::system::errc::success) { // return if connection to server
        // successful
        return ec;
      }
    }
    // Reaching here implies We failed to connect to every endpoint in the list.
    // Indicate connection error in this case
    ec = boost::asio::error::not_found;
    return ec;
  }

} // Rebot
} // namespace ChimeraTK
