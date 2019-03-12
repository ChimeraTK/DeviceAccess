#ifndef DUMMY_PROTOCOL_0_H
#define DUMMY_PROTOCOL_0_H

#include "DummyProtocolImplementor.h"

namespace ChimeraTK {

  class RebotDummySession;

  /// Only put commands which don't exist in all versions, or behave differently
  struct DummyProtocol0 : public DummyProtocolImplementor {
    DummyProtocol0(RebotDummySession& parent);

    virtual void singleWordWrite(std::vector<uint32_t>& buffer) override;
    virtual void multiWordRead(std::vector<uint32_t>& buffer) override;
    virtual uint32_t multiWordWrite(std::vector<uint32_t>& buffer) override;
    virtual uint32_t continueMultiWordWrite(std::vector<uint32_t>& buffer) override;

    virtual void hello(std::vector<uint32_t>& buffer) override;
    virtual void ping(std::vector<uint32_t>& buffer) override;

    uint32_t protocolVersion() const override { return 0; }

    RebotDummySession& _parent;
  };

} //  namespace ChimeraTK

#endif // DUMMY_PROTOCOL_0_H
