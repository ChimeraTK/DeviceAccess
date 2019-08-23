#include "RebotProtocol1.h"

#include "RebotProtocolDefinitions.h"
#include "Connection.h"

namespace ChimeraTK {
  using namespace Rebot;

  RebotProtocol1::RebotProtocol1(boost::shared_ptr<Connection>& tcpCommunicator)
  : RebotProtocol0(tcpCommunicator), _lastSendTime(std::chrono::steady_clock::now()) {
    // Setting the time stamp to now() is sufficient in precision.
    // We know that the server has just replied to the hello before this class was
    // created.
  }

  void RebotProtocol1::read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) {
    // locking is happening in the backend
    // check for isOpen() is happening in the backend which does the bookkeeping

    RegisterInfo registerInfo(addressInBytes, sizeInBytes);
    // Resolution of timing is sufficient if we set the timestamp here
    // We just send one read request at the beginning.
    _lastSendTime = std::chrono::steady_clock::now();
    fetchFromRebotServer(registerInfo.addressInWords, registerInfo.nWords, data);
  }

  void RebotProtocol1::write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) {
    RegisterInfo registerInfo(addressInBytes, sizeInBytes);
    std::vector<uint32_t> writeCommandPacket;
    writeCommandPacket.push_back(MULTI_WORD_WRITE);
    writeCommandPacket.push_back(registerInfo.addressInWords);
    writeCommandPacket.push_back(registerInfo.nWords);
    for(unsigned int i = 0; i < registerInfo.nWords; ++i) {
      writeCommandPacket.push_back(data[i]);
    }
    // Again we timestamp here. Technically the comminucator might send muptilple
    // packets, but it is sufficient to reemember that we triggered it here.
    _lastSendTime = std::chrono::steady_clock::now();
    _tcpCommunicator->write(writeCommandPacket);
    // FIXME: this returns std::vector<int32_t> of length 1. Do error handling!
    (void)_tcpCommunicator->read(1);
  }

  void RebotProtocol1::sendHeartbeat() {
    _tcpCommunicator->write(std::vector<uint32_t>({HELLO_TOKEN, MAGIC_WORD, CLIENT_PROTOCOL_VERSION}));
    // don't evaluate. The other side is sending an error anyway in this protocol
    // version
    _tcpCommunicator->read(3);
  }

} // namespace ChimeraTK
