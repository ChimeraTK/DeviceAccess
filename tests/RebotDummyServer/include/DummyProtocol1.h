#ifndef DUMMY_PROTOCOL_1_H
#define DUMMY_PROTOCOL_1_H

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

    static const uint8_t BAR = 0;
  };

} //  namespace ChimeraTK

#endif // DUMMY_PROTOCOL_1_H
