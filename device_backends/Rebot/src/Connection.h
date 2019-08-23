#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace ChimeraTK {
namespace  Rebot {

  /// Handles the communication over TCP protocol with RebotDevice-based devices
  class Connection {
   public:
    /// Gets an IP address and port of the device but does not open the connection
    Connection(std::string ipaddr, std::string port);
    ~Connection();
    /// Opens a connection to the device.
    void open();
    /// Closes a connection with the device.
    void close();
    /// Receives uint32_t words from the socket.
    std::vector<uint32_t> read(uint32_t const& numWordsToRead);
    /// Sends a std::vector of bytes to the socket.
    void write(const std::vector<uint32_t>& data);

  private:
    std::string _serverAddress;
    std::string _port;
    boost::shared_ptr<boost::asio::io_service> _io_service;
    boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;

    /*!
     * @brief connect to the first good endpoint from a list of endpoints.
     * The endpoint list is accessed through the provided Iterator
     *
     * The code in this method is based on  boost::asio::connect. This convenience
     * function is not available in the version of the boost library (1.46) used
     * currently.
     */
    boost::system::error_code connectToResolvedEndPoints(boost::asio::ip::tcp::resolver::iterator endpointIterator);
  };
} // Rebot

} // namespace ChimeraTK

