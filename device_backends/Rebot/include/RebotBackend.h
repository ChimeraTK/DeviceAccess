// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedBackend.h"

#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace ChimeraTK {
  namespace Rebot {
    class Connection;
  } // namespace Rebot

  struct RebotProtocolImplementor;

  // A helper class that contains a mutex and a flag.
  // The idea is to put it into a shared pointer and hand it to a thread which is
  // sleeping a long time. You can then detach the thread, tell it to finish and
  // continue with the constructor without having to wait for the tread to wake up
  // and finish before you can join it. The thread locks the mutex and checks if
  // it should finish when it wakes up, which it can do because the mutex and flag
  // still exist thanks to the shared pointer.
  struct ThreadInformerMutex {
    std::mutex mutex;
    bool quitThread{false};
  };

  class RebotBackend : public NumericAddressedBackend {
   protected:
    std::string _boardAddr;
    std::string _port;

    boost::shared_ptr<ThreadInformerMutex> _threadInformerMutex;
    // Only access the following membergs when holding the mutex. They are
    // also accessed by the heartbeat thread
    boost::shared_ptr<Rebot::Connection> _connection;
    std::unique_ptr<RebotProtocolImplementor> _protocolImplementor;
    /// The time when the last command (read/write/heartbeat) was send
    boost::chrono::steady_clock::time_point _lastSendTime;
    unsigned int _connectionTimeout;

   public:
    RebotBackend(std::string boardAddr, std::string port, const std::string& mapFileName = "",
        uint32_t connectionTimeout_sec = DEFAULT_CONNECTION_TIMEOUT_sec);
    ~RebotBackend() override;
    /// The function opens the connection to the device
    void open() override;
    void closeImpl() override;
    void read(uint8_t bar, uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) override;
    void write(uint8_t bar, uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) override;
    std::string readDeviceInfo() override { return {"RebotDevice"}; }

    static boost::shared_ptr<DeviceBackend> createInstance(
        std::string address, std::map<std::string, std::string> parameters);

    size_t minimumTransferAlignment([[maybe_unused]] uint64_t bar) const override { return 4; }

   protected:
    void heartbeatLoop(const boost::shared_ptr<ThreadInformerMutex>& threadInformerMutex);
    boost::thread _heartbeatThread;

    const static uint32_t DEFAULT_CONNECTION_TIMEOUT_sec{5};
  };

} // namespace ChimeraTK
