#include "DummyProtocol1.h"
#include "RebotDummyServer.h"
#include <boost/asio.hpp>

namespace ChimeraTK{
  
  void DummyProtocol1::multiWordRead(std::vector<uint32_t>& buffer){
    _parent.readRegisterAndSendData(buffer);
  }

  void DummyProtocol1::hello(std::vector<uint32_t>& /*buffer*/){
    // currently there is no check that the buffer is correct
    // from protocol 2 magic work will be checked, and maybe client version
    std::vector<uint32_t> outputBuffer={RebotDummyServer::HELLO,
                                      RebotDummyServer::REBOT_MAGIC_WORD,
                                      protocolVersion()};
    boost::asio::write(*(_parent._currentClientConnection), boost::asio::buffer(outputBuffer));
  }

}//namespace ChimeraTK
