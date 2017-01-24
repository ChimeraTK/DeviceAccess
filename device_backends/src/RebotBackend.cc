#include "RebotBackend.h"
#include "TcpCtrl.h"
#include "RebotProtocolDefinitions.h"

namespace ChimeraTK {

static const int READ_BLOCK_SIZE = 361;
static const uint32_t HELLO_TOKEN = 0x00000004;
static const uint32_t LENGTH_OF_HELLO_TOKEN_MESSAGE = 3; // 3 words
static const uint32_t MAGIC_WORD = 0x72626f74; // rbot in ascii.. start from
                                               // msb

// Most Significant 16 bits ==  major version
// Least Significant 16 bits == minor version
static const uint32_t CLIENT_PROTOCOL_VERSION = 0x00000001;
static const uint32_t UNKNOWN_INSTRUCTION = -1040; // FIXME: use unsigned

RebotBackend::RebotBackend(std::string boardAddr, int port,
                           std::string mapFileName)
    : NumericAddressedBackend(mapFileName),
      _boardAddr(boardAddr),
      _port(port),
      _tcpObject(boost::make_shared<TcpCtrl>(_boardAddr, _port)),
      _serverProtocolVersion(0), 
      _mutex(){}

RebotBackend::~RebotBackend() {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
  if (isOpen()) {
    _tcpObject->closeConnection();
  }
}

void RebotBackend::open() {
   
   std::lock_guard<std::mutex> lock(_mutex);
   
  _tcpObject->openConnection();
  _serverProtocolVersion = getServerProtocolVersion();
  _opened = true;
}

void RebotBackend::read(uint8_t /*bar*/, uint32_t address, int32_t* data,
                        size_t sizeInBytes) {
  
  std::lock_guard<std::mutex> lock(_mutex);                          
                          
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

  unsigned int totalWordsToFetch = sizeInBytes / 4;

  // TODO: move logic for different versions to different classes
  if (_serverProtocolVersion == 0) {
    int iterationsRequired = totalWordsToFetch / READ_BLOCK_SIZE;
    int leftOverWords = totalWordsToFetch % READ_BLOCK_SIZE;

    // read in till the last multiple of READ_BLOCK_SIZE
    for (int count = 0; count < iterationsRequired; ++count) {
      fetchFromRebotServer(address + (count * READ_BLOCK_SIZE), READ_BLOCK_SIZE,
                           data + (count * READ_BLOCK_SIZE));
    }
    // read remaining from the last multiple of READ_BLOCK_SIZE
    fetchFromRebotServer(address + (iterationsRequired * READ_BLOCK_SIZE),
                         leftOverWords,
                         data + (iterationsRequired * READ_BLOCK_SIZE));

  } else {
    fetchFromRebotServer(address, totalWordsToFetch, data);
  }
}

void RebotBackend::write(uint8_t /*bar*/, uint32_t address, int32_t const* data,
                         size_t sizeInBytes) {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
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
  // TODO: move logic for different versions to different classes
  if (_serverProtocolVersion == 0) {
    int mode = 1;
    unsigned int packetsize = sizeInBytes / 4;
    const unsigned int datasendSize = 3 * sizeof(int);
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

      boost::array<char, 4> receivedData;
      _tcpObject->sendData(datasend);
      _tcpObject->receiveData(receivedData);
    }
  } else {
    int mode = 2; // server supports multi word write
    unsigned int wordsToWrite = sizeInBytes / 4;
    std::vector<uint32_t> writeCommandPacket;
    writeCommandPacket.push_back(mode);
    writeCommandPacket.push_back(address);
    writeCommandPacket.push_back(wordsToWrite);
    for (unsigned int i = 0; i < wordsToWrite; ++i) {
      writeCommandPacket.push_back(data[i]);
    }
    boost::array<char, 4> receivedData;
    _tcpObject->sendData(writeCommandPacket);
    _tcpObject->receiveData(receivedData);
  }
}

void RebotBackend::close() {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
  _opened = false;
  _tcpObject->closeConnection();
}

boost::shared_ptr<DeviceBackend> RebotBackend::createInstance(
    std::string /*host*/, std::string /*instance*/,
    std::list<std::string> parameters, std::string mapFileName) {

  if (parameters.size() < 2) { // expecting tmcb ip and port
    throw RebotBackendException(
        "Tmcb ip address and port not found in the parameter list",
        RebotBackendException::EX_INVALID_PARAMETERS);
  }

  std::list<std::string>::iterator it = parameters.begin();

  std::string tmcbIP = *it;
  int portNumber = std::stoi(*(++it));

  if (++it != parameters.end()) {
    // there is a third parameter, it is the map file
    if (mapFileName.empty()) {
      // we use the parameter from the URI
      // \todo FIXME This can be a relative path. In case the URI is coming from
      // a dmap file,
      // and no map file has been defined in the third column, this path is not
      // interpreted relative to the dmap file.
      // Note: you cannot always interpret it relative to the dmap file because
      // the URI can directly come from the Devic::open() function,
      // although a dmap file path has been set. We don't know this here.
      mapFileName = *it;
    } else {
      // We take the entry from the dmap file because it contains the correct
      // path relative to the dmap file
      // (this is case we print a warning)
      std::cout << "Warning: map file name specified in the sdm URI and the "
                   "third column of the dmap file. "
                << "Taking the name from the dmap file ('" << mapFileName
                << "')" << std::endl;
    }
  }

  return boost::shared_ptr<RebotBackend>(
      new RebotBackend(tmcbIP, portNumber, mapFileName));
}

void RebotBackend::fetchFromRebotServer(uint32_t wordAddress,
                                        uint32_t numberOfWords,
                                        int32_t* dataLocation) {
  sendRebotReadRequest(wordAddress, numberOfWords);

  //first check that the response starts with READ_ACK. If it is an error code there might be just
  //one word in the response.
  std::vector<int32_t> responseCode = _tcpObject->receiveData(1);  
  if (responseCode[0] != rebot::READ_ACK){
    std::cout << "response code is " << responseCode[0] << std::endl;
    // FIXME: can we do somwthing more clever here?
    throw RebotBackendException("Reading via ReboT failed",
                                RebotBackendException::EX_SOCKET_READ_FAILED);
  }

  // now that we know that the command worked on the server side we can read the rest of the data
  std::vector<int32_t> readData = _tcpObject->receiveData(numberOfWords);

  transferVectorToDataPtr(readData, dataLocation);
}

void RebotBackend::sendRebotReadRequest(const uint32_t wordAddress,
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
    datasend[j] = (wordAddress >> (8 * (j - 4))) & 0xFF;
  }
  for (int j = 8; j < 12; ++j) {
    datasend[j] = ((wordsToRead) >> (8 * (j - 8))) & 0xFF;
  }

  _tcpObject->sendData(datasend);
}

void RebotBackend::transferVectorToDataPtr(std::vector<int32_t> source,
                                           int32_t* destination) {
  for (auto& i : source) {
    *destination = i;
    ++destination; // this will not change the destination ptr value outside the
                   // scope of this function (signature pass by value)
  }
}

uint32_t RebotBackend::getServerProtocolVersion() {
  // send a negotiation to the server:
  // sendClientProtocolVersion
  std::vector<uint32_t> clientHelloMessage = frameClientHello();
  _tcpObject->sendData(clientHelloMessage);

  // Kludge is needed to work around server bug.
  // We have a bug with the old version were only one word is returned for multiple
  // unrecognized command. Fetching one word for the 3 words send is a workaround.
  auto serverHello = _tcpObject->receiveData(1);

  if (serverHello.at(0) == (int32_t) UNKNOWN_INSTRUCTION) {
    return 0; // initial protocol version 0.0
  }

  auto remainingBytesOfAValidServerHello =
      _tcpObject->receiveData(LENGTH_OF_HELLO_TOKEN_MESSAGE - 1);

  serverHello.insert(serverHello.end(),
                     remainingBytesOfAValidServerHello.begin(),
                     remainingBytesOfAValidServerHello.end());
  return parseRxServerHello(serverHello);
}

std::vector<uint32_t> RebotBackend::frameClientHello() {
  std::vector<uint32_t> clientHello;
  clientHello.push_back(HELLO_TOKEN);
  clientHello.push_back(MAGIC_WORD);
  clientHello.push_back(CLIENT_PROTOCOL_VERSION);
  return std::move(clientHello);
}

uint32_t RebotBackend::parseRxServerHello(
    const std::vector<int32_t>& serverHello) {
  // 3 rd element/word is the version word
  return serverHello.at(2);
}

} // namespace ChimeraTK
