// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Connection.h"

#include "Exception.h"

#include <utility>
#include <boost/exception/diagnostic_information.hpp> 
namespace ChimeraTK::Rebot {

  using Error = boost::system::error_code;

  Connection::Connection(std::string address, std::string port, uint32_t connectionTimeout_sec)
  : address_(std::move(address)), port_(std::move(port)), s_(ioService_), disconnectTimer_(ioService_),
    connectionTimeout_(boost::posix_time::seconds(connectionTimeout_sec)) {}

  void Connection::open() {
     try{
      disconnectionTimerStart();
      boost::asio::ip::tcp::resolver r(ioService_);
      boost::asio::async_connect(
          s_, r.resolve({address_, port_}), [&](const Error ec, auto) { disconnectionTimerCancel(ec); });

      ioService_.reset();
      ioService_.run();
    }
    catch (const boost::exception&) {
        // error handling
        throw ChimeraTK::runtime_error("RebotBackend exception: Host unreachable:");
    }
  }

  std::vector<uint32_t> Connection::read(uint32_t numWords) {
    std::vector<uint32_t> d(numWords);
    disconnectionTimerStart();
    boost::asio::async_read(s_, boost::asio::buffer(d), [&](Error ec, std::size_t) { disconnectionTimerCancel(ec); });

    ioService_.reset();
    ioService_.run();
    return d;
  }

  void Connection::write(const std::vector<uint32_t>& d) {
    disconnectionTimerStart();
    boost::asio::async_write(s_, boost::asio::buffer(d), [&](Error ec, std::size_t) { disconnectionTimerCancel(ec); });

    ioService_.reset();
    ioService_.run();
  }

  void Connection::close() {
    if(s_.is_open()) {
      s_.cancel();
      s_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
      s_.close();
    }
  }

  bool Connection::isOpen() {
    return s_.is_open();
  }

  void Connection::disconnectionTimerStart() {
    disconnectTimer_.expires_from_now(connectionTimeout_);
    disconnectTimer_.async_wait([&](const Error& ec) {
      if(ec == boost::asio::error::operation_aborted) {
        // you have this error code when the timer was cancelled before expiry
        return;
      }
      close();
    });
  }

  void Connection::disconnectionTimerCancel(const Error& ec) {
    disconnectTimer_.cancel();
    if(ec) {
      close();
      throw ChimeraTK::runtime_error("Rebot connection timed out");
    }
  }

} // namespace ChimeraTK::Rebot
