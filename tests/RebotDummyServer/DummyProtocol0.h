#ifndef DUMMY_PROTOCOL_0_H
#define DUMMY_PROTOCOL_0_H

#include "DummyProtocolImplementor.h"

namespace ChimeraTK{
  
class RebotDummyServer;

/// Only put commands which don't exist in all versions, or behave differently
struct DummyProtocol0: public DummyProtocolImplementor{
  DummyProtocol0(RebotDummyServer & parent);

  virtual void singleWordWrite(std::vector<uint32_t>& buffer);
  virtual void multiWordRead(std::vector<uint32_t>& buffer);
  
  RebotDummyServer & _parent;
};

}//  namespace ChimeraTK

#endif //DUMMY_PROTOCOL_0_H
