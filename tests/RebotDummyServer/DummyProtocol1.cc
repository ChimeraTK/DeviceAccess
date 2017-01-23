#include "DummyProtocol1.h"
#include "RebotDummyServer.h"

namespace ChimeraTK{
  
  void DummyProtocol1::multiWordRead(std::vector<uint32_t>& buffer){
    _parent.readRegisterAndSendData(buffer);
  }

}//namespace ChimeraTK
