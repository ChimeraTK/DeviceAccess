#include "RebotProtocol0.h"
#include "TcpCtrl.h"
#include "RebotProtocolDefinitions.h"
#include "RebotBackendException.h"
#include <iostream>

namespace ChimeraTK{
  using namespace rebot;

  RebotProtocol0::RebotProtocol0(boost::shared_ptr<TcpCtrl> & tcpCommunicator)
    :  _tcpCommunicator(tcpCommunicator)
  {}

  RebotProtocol0::RegisterInfo::RegisterInfo(uint32_t addressInBytes, uint32_t sizeInBytes){
    if (sizeInBytes % 4 != 0) {
      throw RebotBackendException("\"size\" argument must be a multiplicity of 4",
                                  RebotBackendException::EX_SIZE_INVALID);
    }
    // address == byte address; This should be converted into word address
    if (addressInBytes % 4 != 0) {
      throw RebotBackendException(
                                  "Register address is not valid",
                                  RebotBackendException::EX_INVALID_REGISTER_ADDRESS);
    }

    addressInWords = addressInBytes/4;
    nWords = sizeInBytes/4;
  }
  
  void RebotProtocol0::read(uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) {
    //locking is happening in the backend
    //check for isOpen() is happening in the backend which does the bookkeeping

    RegisterInfo registerInfo(addressInBytes, sizeInBytes);
    
    // read implementation for protocol 0 : we are limited in the read size and
    // have to do multiple calls to fetchFromRebotServer
    
    int iterationsRequired = registerInfo.nWords / READ_BLOCK_SIZE;
    int leftOverWords = registerInfo.nWords % READ_BLOCK_SIZE;

    // read in till the last multiple of READ_BLOCK_SIZE
    for (int count = 0; count < iterationsRequired; ++count) {
      fetchFromRebotServer(registerInfo.addressInWords+ (count * READ_BLOCK_SIZE), READ_BLOCK_SIZE,
                           data + (count * READ_BLOCK_SIZE));
    }
    // read remaining from the last multiple of READ_BLOCK_SIZE
    fetchFromRebotServer(registerInfo.addressInWords + (iterationsRequired * READ_BLOCK_SIZE),
                         leftOverWords,
                         data + (iterationsRequired * READ_BLOCK_SIZE));

  }

  void RebotProtocol0::write(uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) {
  
  RegisterInfo registerInfo(addressInBytes, sizeInBytes);

  //Implementation for protocol version 0: Only single word write possible
  const unsigned int datasendSize = 3 * sizeof(int);
  for (unsigned int i = 0; i < registerInfo.nWords; ++i) {
    std::vector<char> datasend(datasendSize);
    //FIXME: Do proper host to network conversion, no manual, platform dependent byte fiddling
    datasend[0] = SINGLE_WORD_WRITE; // set the mode
    for (int j = 1; j < 4; ++j) {
      datasend[j] = 0;
    }
    for (int j = 4; j < 8; ++j) {
      datasend[j] = ((registerInfo.addressInWords + i) >> (8 * (j - 4))) & 0xFF;
    }
    for (int j = 8; j < 12; ++j) {
      datasend[j] = (data[i] >> (8 * (j - 8))) & 0xFF;
    }
    
    boost::array<char, 4> receivedData;
    _tcpCommunicator->sendData(datasend);
    _tcpCommunicator->receiveData(receivedData);
    //FIXME: Do error handling of the response
  }
}

void RebotProtocol0::fetchFromRebotServer(uint32_t wordAddress,
                                          uint32_t numberOfWords,
                                          int32_t* dataLocation) {
  sendRebotReadRequest(wordAddress, numberOfWords);

  //first check that the response starts with READ_ACK. If it is an error code there might be just
  //one word in the response.
  std::vector<int32_t> responseCode = _tcpCommunicator->receiveData(1);  
  if (responseCode[0] != rebot::READ_ACK){
    std::cout << "response code is " << responseCode[0] << std::endl;
    // FIXME: can we do somwthing more clever here?
    throw RebotBackendException("Reading via ReboT failed",
                                RebotBackendException::EX_SOCKET_READ_FAILED);
  }

  // now that we know that the command worked on the server side we can read the rest of the data
  std::vector<int32_t> readData = _tcpCommunicator->receiveData(numberOfWords);

  transferVectorToDataPtr(readData, dataLocation);
}

void RebotProtocol0::sendRebotReadRequest(const uint32_t wordAddress, const uint32_t wordsToRead) {

  unsigned int datasendSize = 3 * sizeof(int);
  std::vector<char> datasend(datasendSize);
  //FIXME: Do proper host to network conversion, no manual, platform dependent byte fiddling
  // send out an n word read request
  datasend[0] = MULTI_WORD_READ;
  for (int j = 1; j < 4; ++j) {
    datasend[j] = 0;
  }
  for (int j = 4; j < 8; ++j) {
    datasend[j] = (wordAddress >> (8 * (j - 4))) & 0xFF;
  }
  for (int j = 8; j < 12; ++j) {
    datasend[j] = ((wordsToRead) >> (8 * (j - 8))) & 0xFF;
  }

  _tcpCommunicator->sendData(datasend);
}

void RebotProtocol0::transferVectorToDataPtr(std::vector<int32_t> source, int32_t* destination) {
  // FIXME: just use memcopy
  for (auto& i : source) {
    *destination = i;
    ++destination; // this will not change the destination ptr value outside the
                   // scope of this function (signature pass by value)
  }
}

void RebotProtocol0::sendHeartbeat(){
  //just do nothing in v0
}
  
} // namespace ChimeraTK
