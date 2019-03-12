#include "DummyProtocol1.h"
#include "RebotDummyServer.h"
#include <boost/asio.hpp>

namespace ChimeraTK {

  DummyProtocol1::DummyProtocol1(RebotDummySession& parent)
  : DummyProtocol0(parent), _nextAddressInWords(0), _nWordsLeft(0) {}

  void DummyProtocol1::multiWordRead(std::vector<uint32_t>& buffer) { _parent.readRegisterAndSendData(buffer); }

  void DummyProtocol1::hello(std::vector<uint32_t>& /*buffer*/) {
    // currently there is no check that the buffer is correct
    // from protocol 2 magic work will be checked, and maybe client version

    _parent.write({
        RebotDummySession::HELLO, RebotDummySession::REBOT_MAGIC_WORD, protocolVersion()});
  }

  uint32_t DummyProtocol1::multiWordWrite(std::vector<uint32_t>& buffer) {
    uint32_t addressInWords = buffer.at(1);
    uint32_t addressInBytes = addressInWords * 4;
    uint32_t nWordsTotal = buffer.at(2);

    if((buffer.size() - 3) < nWordsTotal) {
      uint32_t nWordsInThisBuffer = buffer.size() - 3;
      _nWordsLeft = nWordsTotal - nWordsInThisBuffer;
      _nextAddressInWords = addressInWords + nWordsInThisBuffer;

      _parent._registerSpace->write(
          BAR, addressInBytes, reinterpret_cast<int32_t*>(&(buffer.at(3))), 4 * nWordsInThisBuffer);
      return RebotDummySession::INSIDE_MULTI_WORD_WRITE;
    }
    else { // all words in this buffer

      _parent._registerSpace->write(BAR, addressInBytes, reinterpret_cast<int32_t*>(&(buffer.at(3))), 4 * nWordsTotal);

      _parent.sendSingleWord(RebotDummySession::WRITE_SUCCESS_INDICATION);
      return RebotDummySession::ACCEPT_NEW_COMMAND;
    }
  }

  uint32_t DummyProtocol1::continueMultiWordWrite(std::vector<uint32_t>& buffer) {
    if(buffer.size() < _nWordsLeft) {
      uint32_t nWordsInThisBuffer = buffer.size();

      _parent._registerSpace->write(
          BAR, _nextAddressInWords * 4, reinterpret_cast<int32_t*>(buffer.data()), 4 * nWordsInThisBuffer);

      _nWordsLeft -= nWordsInThisBuffer;
      _nextAddressInWords += nWordsInThisBuffer;

      return RebotDummySession::INSIDE_MULTI_WORD_WRITE;
    }
    else { // all words in this buffer

      _parent._registerSpace->write(
          BAR, _nextAddressInWords * 4, reinterpret_cast<int32_t*>(buffer.data()), 4 * _nWordsLeft);

      _nextAddressInWords = 0;
      _nWordsLeft = 0;

      _parent.sendSingleWord(RebotDummySession::WRITE_SUCCESS_INDICATION);
      return RebotDummySession::ACCEPT_NEW_COMMAND;
    }
  }

} // namespace ChimeraTK
