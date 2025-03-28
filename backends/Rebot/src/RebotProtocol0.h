// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "RebotProtocolImplementor.h"

#include <boost/shared_ptr.hpp>

#include <vector>

namespace ChimeraTK {
  namespace Rebot {
    class Connection;
  }

  struct RebotProtocol0 : RebotProtocolImplementor {
    explicit RebotProtocol0(boost::shared_ptr<Rebot::Connection>& tcpCommunicator);
    virtual ~RebotProtocol0() {};

    virtual void read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) override;
    virtual void write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) override;
    virtual void sendHeartbeat() override;

    struct RegisterInfo {
      uint32_t addressInWords;
      uint32_t nWords;
      // the constructor checks that the address and size are multiples of 4 and
      // throws if this is not the case
      RegisterInfo(uint32_t addressInBytes, uint32_t sizeInBytes);
    };

    //  protected:
    boost::shared_ptr<Rebot::Connection> _tcpCommunicator;
    void fetchFromRebotServer(uint32_t wordAddress, uint32_t numberOfWords, int32_t* dataLocation) const;
    void sendRebotReadRequest(const uint32_t wordAddress, const uint32_t wordsToRead) const;
    static void transferVectorToDataPtr(const std::vector<uint32_t>& source, int32_t* destination);
  };

} // namespace ChimeraTK
