#include "DummyProtocol0.h"
#include "RebotDummyServer.h"

namespace ChimeraTK{
  
  DummyProtocol0::DummyProtocol0(RebotDummyServer & parent)
    : _parent(parent) {
  }

  void DummyProtocol0::singleWordWrite(std::vector<uint32_t>& buffer){
    _parent.writeWordToRequestedAddress(buffer);
    // if  writeWordToRequestedAddress dosent throw, we can safely assume
    // write was a success
    _parent.sendSingleWord(RebotDummyServer::WRITE_SUCCESS_INDICATION);

  }
  
  void DummyProtocol0::multiWordRead(std::vector<uint32_t>& buffer){
    uint32_t numberOfWordsToRead = buffer.at(2);
    
    if (numberOfWordsToRead > 361) { // not supported
      _parent.sendSingleWord(RebotDummyServer::TOO_MUCH_DATA_REQUESTED);
    } else {
      _parent.readRegisterAndSendData(buffer);
    }
  }

  uint32_t DummyProtocol0::multiWordWrite(std::vector<uint32_t>& /*buffer*/){
    _parent.sendSingleWord(RebotDummyServer::UNKNOWN_INSTRUCTION);
    return RebotDummyServer::ACCEPT_NEW_COMMAND;
  }

  uint32_t DummyProtocol0::continueMultiWordWrite(std::vector<uint32_t>& /*buffer*/){
    // we never should end here. Don't do anything
    return RebotDummyServer::ACCEPT_NEW_COMMAND;
  }

  void DummyProtocol0::hello(std::vector<uint32_t>& /*buffer*/){
    _parent.sendSingleWord(RebotDummyServer::UNKNOWN_INSTRUCTION);
  }

  void DummyProtocol0::ping(std::vector<uint32_t>& /*buffer*/){
    std::cout << "Ping was send. Sending unknown instruction" << std::endl;
    _parent.sendSingleWord(RebotDummyServer::UNKNOWN_INSTRUCTION);
  }

  
}//namespace ChimeraTK
