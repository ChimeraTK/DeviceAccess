#include "RebotBackend.h"
#include "TcpCtrl.h"

namespace mtca4u {

RebotBackend::RebotBackend(std::string boardAddr, int port)
    : _boardAddr(boardAddr),
      _port(port),
      _tcpObject(boost::make_shared<TcpCtrl>(_boardAddr, _port)) {}

RebotBackend::~RebotBackend() {
  if (isOpen()) {
    _tcpObject->closeConnection();
  }
}

void RebotBackend::open() {
  _tcpObject->openConnection();
  _opened = true;
}

void RebotBackend::read(uint8_t /*bar*/, uint32_t address, int32_t* data,
                        size_t sizeInBytes) {
  if (!isOpen()) {
    throw RebotBackendException("Device is closed",
                                RebotBackendException::EX_DEVICE_CLOSED);
  }
  if (sizeInBytes % 4 != 0) {
    throw RebotBackendException("\"size\" argument must be a multiplicity of 4",
                                RebotBackendException::EX_SIZE_INVALID);
  }
  // address == byte address; This should be converted into word address
  if (address % 4 != 0) {
    throw RebotBackendException(
        "Register address is not valid",
        RebotBackendException::EX_INVALID_REGISTER_ADDRESS);
  } else {
    address = address / 4;
  }

  unsigned int wordsToRead = sizeInBytes / 4;

  // address, words to read -> command
  sendRebotReadRequest(address, wordsToRead);

  // read wordsToRead + 1. The extra byte is the read status indicator. The
  // returned status indicator from the server should be handled FIXME
  std::vector<int32_t> readData = _tcpObject->receiveData(wordsToRead + 1);

  // remove the first byte form the read in data; this will be the read status
  // indicator returned from the server
  readData.erase(readData.begin());

  transferVectorToDataPtr(readData, data);
}

void RebotBackend::sendRebotReadRequest(const uint32_t address,
                                        const uint32_t wordsToRead) {
  int mode = 3;

  unsigned int datasendSize = 3 * sizeof(int);
  std::vector<char> datasend(datasendSize);
  datasend[0] = mode;

  // send out an n word read request
  for (int j = 1; j < 4; ++j) {
    datasend[j] = 0;
  }
  for (int j = 4; j < 8; ++j) {
    datasend[j] = (address >> (8 * (j - 4))) & 0xFF;
  }
  for (int j = 8; j < 12; ++j) {
    datasend[j] = ((wordsToRead) >> (8 * (j - 8))) & 0xFF;
  }

  _tcpObject->sendData(datasend);
}

void RebotBackend::transferVectorToDataPtr(std::vector<int32_t> source,
                                           int32_t* destination) {
  for (auto &i : source) {
    *destination = i;
    ++destination; // this will not change the destination ptr value outside the
                   // scope of this function
  }
}
void RebotBackend::write(uint8_t /*bar*/, uint32_t address, int32_t const* data,
                         size_t sizeInBytes) {
  if (!isOpen()) {
    throw RebotBackendException("Device is closed",
                                RebotBackendException::EX_DEVICE_CLOSED);
  }
  if (sizeInBytes % 4 != 0) {
    throw RebotBackendException("\"size\" argument must be a multiplicity of 4",
                                RebotBackendException::EX_SIZE_INVALID);
  }
  // address == byte address; This should be converted into word address
  if (address % 4 != 0) {
    throw RebotBackendException(
        "Register address is not valid",
        RebotBackendException::EX_INVALID_REGISTER_ADDRESS);
  } else {
    address = address / 4;
  }

  int mode = 1;
  unsigned int packetsize = sizeInBytes / 4;
  const unsigned int datasendSize = 3 * sizeof(int);
  boost::array<char, 4> receivedData;
  for (unsigned int i = 0; i < packetsize; ++i) {
    std::vector<char> datasend(datasendSize);
    datasend[0] = mode;
    for (int j = 1; j < 4; ++j) {
      datasend[j] = 0;
    }
    for (int j = 4; j < 8; ++j) {
      datasend[j] = ((address + i) >> (8 * (j - 4))) & 0xFF;
    }
    for (int j = 8; j < 12; ++j) {
      datasend[j] = (data[i] >> (8 * (j - 8))) & 0xFF;
    }

    _tcpObject->sendData(datasend);
    _tcpObject->receiveData(receivedData);
  }
}

void RebotBackend::close() {
  _opened = false;
  _tcpObject->closeConnection();
}

boost::shared_ptr<DeviceBackend> RebotBackend::createInstance(
    std::string /*host*/, std::string /*instance*/,
    std::list<std::string> parameters) {

  if (parameters.size() < 2) { // expecting tmcb ip and port
    throw RebotBackendException(
        "Tmcb ip address and port not found in the parameter list",
        RebotBackendException::EX_INVALID_PARAMETERS);
  }

  std::list<std::string>::iterator it = parameters.begin();

  std::string tmcbIP = *it;
  int portNumber = std::stoi(*(++it));
  return boost::shared_ptr<RebotBackend>(new RebotBackend(tmcbIP, portNumber));
}
} // namespace mtca4u
