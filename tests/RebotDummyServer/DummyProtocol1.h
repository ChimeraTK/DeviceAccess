#ifndef DUMMY_PROTOCOL_1_H
#define DUMMY_PROTOCOL_1_H

#include "DummyProtocol0.h"

namespace ChimeraTK{
  
class RebotDummyServer;

/// Only put commands which don't exist in all versions, or behave differently
struct DummyProtocol1: public DummyProtocol0{
  using DummyProtocol0::DummyProtocol0; // use the parent constructor with the parent argument
  
  /// The multi word read is not limited in the size any more
  virtual void multiWordRead(std::vector<uint32_t>& buffer);

  /// First protocol version that implements hello
  virtual void hello(std::vector<uint32_t>& buffer);

  virtual uint32_t protocolVersion(){return 1;}

};

}//  namespace ChimeraTK

#endif //DUMMY_PROTOCOL_1_H
