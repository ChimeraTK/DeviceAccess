// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "DummyProtocol0.h"

namespace ChimeraTK {

  class RebotDummySession;

  /// Only put commands which don't exist in all versions, or behave differently
  struct DummyProtocol1 : public DummyProtocol0 {
    DummyProtocol1(RebotDummySession& parent);

    /// The multi word read is not limited in the size any more
    void multiWordRead(std::vector<uint32_t>& buffer) override;

    /// First protocol version that implements hello
    uint32_t multiWordWrite(std::vector<uint32_t>& buffer) override;
    uint32_t continueMultiWordWrite(std::vector<uint32_t>& buffer) override;

    /// First protocol version that implements hello
    void hello(std::vector<uint32_t>& buffer) override;

    uint32_t protocolVersion() const override { return 1; }

    // part of the multi word write across many packets
    uint32_t _nextAddressInWords;
    uint32_t _nWordsLeft;

    static const uint64_t BAR = 0;
  };

} //  namespace ChimeraTK
