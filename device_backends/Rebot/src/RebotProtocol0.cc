#include "RebotProtocol0.h"
#include "Exception.h"
#include "RebotProtocolDefinitions.h"
#include "Connection.h"
#include <iostream>

namespace ChimeraTK {
  using namespace Rebot;

  RebotProtocol0::RebotProtocol0(boost::shared_ptr<Connection>& tcpCommunicator) : _tcpCommunicator(tcpCommunicator) {}

  RebotProtocol0::RegisterInfo::RegisterInfo(uint32_t addressInBytes, uint32_t sizeInBytes) {
    if(sizeInBytes % 4 != 0) {
      throw ChimeraTK::logic_error("\"size\" argument must be a multiplicity of 4");
    }
    // address == byte address; This should be converted into word address
    if(addressInBytes % 4 != 0) {
      throw ChimeraTK::logic_error("Register address is not valid");
    }

    addressInWords = addressInBytes / 4;
    nWords = sizeInBytes / 4;
  }

  void RebotProtocol0::read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) {
    // locking is happening in the backend
    // check for isOpen() is happening in the backend which does the bookkeeping

    RegisterInfo registerInfo(addressInBytes, sizeInBytes);

    // read implementation for protocol 0 : we are limited in the read size and
    // have to do multiple calls to fetchFromRebotServer

    int iterationsRequired = registerInfo.nWords / READ_BLOCK_SIZE;
    int leftOverWords = registerInfo.nWords % READ_BLOCK_SIZE;

    // read in till the last multiple of READ_BLOCK_SIZE
    for(int count = 0; count < iterationsRequired; ++count) {
      fetchFromRebotServer(
          registerInfo.addressInWords + (count * READ_BLOCK_SIZE), READ_BLOCK_SIZE, data + (count * READ_BLOCK_SIZE));
    }
    // read remaining from the last multiple of READ_BLOCK_SIZE
    fetchFromRebotServer(registerInfo.addressInWords + (iterationsRequired * READ_BLOCK_SIZE), leftOverWords,
        data + (iterationsRequired * READ_BLOCK_SIZE));
  }

  void RebotProtocol0::write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) {
    RegisterInfo registerInfo(addressInBytes, sizeInBytes);

    // Implementation for protocol version 0: Only single word write possible
    std::vector<uint32_t> packet(3);
    for(unsigned int i = 0; i < registerInfo.nWords; ++i) {
        packet.at(0) = SINGLE_WORD_WRITE;
        packet.at(1) = registerInfo.addressInWords++;
        packet.at(2) = static_cast<uint32_t>(data[i]);
        _tcpCommunicator->write(packet);
        _tcpCommunicator->read(1); // response is ignored for now
    }
  }


  void RebotProtocol0::fetchFromRebotServer(uint32_t wordAddress, uint32_t numberOfWords, int32_t* dataLocation) {
    sendRebotReadRequest(wordAddress, numberOfWords);

    // first check that the response starts with READ_ACK. If it is an error code
    // there might be just one word in the response.
    std::vector<uint32_t> responseCode = _tcpCommunicator->read(1);
    if(responseCode[0] != Rebot::READ_ACK) {
      // FIXME: can we do somwthing more clever here?
      throw ChimeraTK::runtime_error("Reading via ReboT failed. Response code: " + std::to_string(responseCode[0]));
    }

    // now that we know that the command worked on the server side we can read the
    // rest of the data
    std::vector<uint32_t> readData = _tcpCommunicator->read(numberOfWords);

    transferVectorToDataPtr(readData, dataLocation);
  }

  void RebotProtocol0::sendRebotReadRequest(const uint32_t wordAddress, const uint32_t wordsToRead) {
    unsigned int datasendSize = 3 * sizeof(int);
    std::vector<char> datasend(datasendSize);
    std::vector<uint32_t> packet(3);
    packet.at(0) = MULTI_WORD_READ;
    packet.at(1) = wordAddress;
    packet.at(2) = wordsToRead;
    _tcpCommunicator->write(packet);
  }

  void RebotProtocol0::transferVectorToDataPtr(std::vector<uint32_t> source, int32_t* destination) {
    // FIXME: just use memcopy
    for(auto& i : source) {
      *destination = i;
      ++destination; // this will not change the destination ptr value outside the
                     // scope of this function (signature pass by value)
    }
  }

  void RebotProtocol0::sendHeartbeat() {
    // just do nothing in v0
  }

} // namespace ChimeraTK
