#ifndef DUMMY_PROTOCOL_0_H
#define DUMMY_PROTOCOL_0_H

#include "DummyProtocolImplementor.h"

namespace ChimeraTK{
  
class RebotDummyServer;

/// Only put commands which don't exist in all versions, or behave differently
struct DummyProtocol0: public DummyProtocolImplementor{
  DummyProtocol0(RebotDummyServer & parent);

  virtual void singleWordWrite(std::vector<uint32_t>& buffer) override;
  virtual void multiWordRead(std::vector<uint32_t>& buffer) override;
  virtual uint32_t multiWordWrite(std::vector<uint32_t>& buffer) override;
  virtual uint32_t continueMultiWordWrite(std::vector<uint32_t>& buffer) override;

  virtual void hello(std::vector<uint32_t>& buffer) override;
  virtual void ping(std::vector<uint32_t>& buffer) override;

  virtual uint32_t protocolVersion() override {return 0;}
 
  RebotDummyServer & _parent;
};

}//  namespace ChimeraTK

#endif //DUMMY_PROTOCOL_0_H
