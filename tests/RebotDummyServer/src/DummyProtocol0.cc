#include "DummyProtocol0.h"
#include "RebotDummyServer.h"

namespace ChimeraTK {

  DummyProtocol0::DummyProtocol0(RebotDummySession& parent) : _parent(parent) {}

  void DummyProtocol0::singleWordWrite(std::vector<uint32_t>& buffer) {
    _parent.writeWordToRequestedAddress(buffer);
    // if  writeWordToRequestedAddress dosent throw, we can safely assume
    // write was a success
    _parent.sendSingleWord(RebotDummySession::WRITE_SUCCESS_INDICATION);
  }

  void DummyProtocol0::multiWordRead(std::vector<uint32_t>& buffer) {
    uint32_t numberOfWordsToRead = buffer.at(2);

    if(numberOfWordsToRead > 361) { // not supported
      _parent.sendSingleWord(RebotDummySession::TOO_MUCH_DATA_REQUESTED);
    }
    else {
      _parent.readRegisterAndSendData(buffer);
    }
  }

  uint32_t DummyProtocol0::multiWordWrite(std::vector<uint32_t>& /*buffer*/) {
    _parent.sendSingleWord(RebotDummySession::UNKNOWN_INSTRUCTION);
    return RebotDummySession::ACCEPT_NEW_COMMAND;
  }

  uint32_t DummyProtocol0::continueMultiWordWrite(std::vector<uint32_t>& /*buffer*/) {
    // we never should end here. Don't do anything
    return RebotDummySession::ACCEPT_NEW_COMMAND;
  }

  void DummyProtocol0::hello(std::vector<uint32_t>& /*buffer*/) {
    _parent.sendSingleWord(RebotDummySession::UNKNOWN_INSTRUCTION);
  }

  void DummyProtocol0::ping(std::vector<uint32_t>& /*buffer*/) {
    _parent.sendSingleWord(RebotDummySession::UNKNOWN_INSTRUCTION);
  }

} // namespace ChimeraTK
