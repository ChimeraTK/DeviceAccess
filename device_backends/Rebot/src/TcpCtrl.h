#ifndef CHIMERATK_TCPCTRL_H
#define CHIMERATK_TCPCTRL_H

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

  /// Handles the communication over TCP protocol with RebotDevice-based devices
  class TcpCtrl {
   private:
    std::string _serverAddress;
    int _port;
    boost::shared_ptr<boost::asio::io_service> _io_service;
    boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;

   public:
    /// Gets an IP address and port of the device but does not open the connection
    TcpCtrl(std::string ipaddr, int port);
    ~TcpCtrl();
    /// Opens a connection to the device.
    void openConnection();
    /// Closes a connection with the device.
    void closeConnection();
    /// Receives int32_t words from the socket.
    std::vector<int32_t> receiveData(uint32_t const& numWordsToRead);
    void receiveData(boost::array<char, 4>& receivedArray);
    /// Sends a std::vector of bytes to the socket.
    void sendData(const std::vector<char>& data);
    void sendData(const std::vector<uint32_t>& data);
    /// Returns an IP address associated with an object of the class.
    std::string getAddress();
    /// Sets an IP address in an object. Can be done when connection is closed.
    void setAddress(std::string ipaddr);
    /// Returns a port associated with an object of the class.
    int getPort();
    /// Sets port in an object. Can be done when connection is closed.
    void setPort(int port);

   private:
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

} // namespace ChimeraTK

#endif /* CHIMERATK_TCPCTRL_H */
