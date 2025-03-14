// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
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

namespace ChimeraTK::Rebot {

  /// Handles the communication over TCP protocol with RebotDevice-based devices
  class Connection {
   public:
    /// Gets an IP address and port of the device but does not open the
    /// connection
    Connection(std::string address, std::string port, uint32_t connectionTimeout_sec);

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

   private:
    std::string address_;
    const std::string port_;
    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::socket s_;
    boost::asio::deadline_timer disconnectTimer_;
    boost::asio::deadline_timer::duration_type connectionTimeout_;

    void disconnectionTimerStart();
    void disconnectionTimerCancel(const boost::system::error_code& ec);
  };
} // namespace ChimeraTK::Rebot
