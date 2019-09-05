#pragma once

#include "Timer.h"
#include "RebotProtocolDefinitions.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <mutex>

namespace ChimeraTK {
namespace Rebot {

  struct HeartBeat{
    std::vector<uint32_t> sendToken;
    uint32_t responseLength;
  };

  /// Handles the communication over TCP protocol with RebotDevice-based devices
  class Connection {
  public:
    /// Gets an IP address and port of the device but does not open the
    /// connection
    Connection(const std::string& address, const std::string& port,
               uint32_t connectionTimeout_sec = 5);

    /// Opens a connection to the device.
    void open();

    /// Closes a connection with the device.
    void close();

    /// Receives uint32_t words from the socket.
    std::vector<uint32_t> read(uint32_t numWordsToRead);

    /// Sends a std::vector of bytes to the socket.
    void write(const std::vector<uint32_t>& data);

    /// Get the connection state.
    bool isOpen();

    void configureKeepAlive(HeartBeat c, uint32_t period_inSec);
  private:
    std::string address_;
    const std::string port_;
    boost::asio::io_service ioService_;
    boost::asio::ip::tcp::socket s_;
    boost::asio::deadline_timer disconnectTimer_;
    boost::asio::deadline_timer::duration_type connectionTimeout_;
    Timer keepaliveTimer_{};

    void disconnectionTimerStart();
    void disconnectionTimerCancel(const boost::system::error_code& ec);
    void write_(const std::vector<uint32_t>& d);
    std::vector<uint32_t> read_(uint32_t numWords);
  };
} // namespace Rebot

} // namespace ChimeraTK

