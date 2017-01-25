#include "RebotBackend.h"
#include "TcpCtrl.h"
#include "RebotProtocolDefinitions.h"
#include "RebotProtocol1.h"
#include <sstream>

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
      _tcpCommunicator(boost::make_shared<TcpCtrl>(_boardAddr, _port)),
      _mutex(),
      _protocolImplementor(){}

RebotBackend::~RebotBackend() {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
  if (isOpen()) {
    _tcpCommunicator->closeConnection();
  }
}

void RebotBackend::open() {
   
   std::lock_guard<std::mutex> lock(_mutex);
   
  _tcpCommunicator->openConnection();
  auto serverVersion = getServerProtocolVersion();
  if (serverVersion == 0){
    _protocolImplementor.reset(new RebotProtocol0(_tcpCommunicator));
  }else if (serverVersion == 1){
    _protocolImplementor.reset(new RebotProtocol1(_tcpCommunicator));    
  }else{
    _tcpCommunicator->closeConnection();
    std::stringstream errorMessage;
    errorMessage << "Server protocol version " << serverVersion << " not supported!";
    throw RebotBackendException(errorMessage.str(), RebotBackendException::EX_CONNECTION_FAILED);
  }
    
  _opened = true;
}

void RebotBackend::read(uint8_t /*bar*/, uint32_t addressInBytes, int32_t* data,
                        size_t sizeInBytes) {
  
  std::lock_guard<std::mutex> lock(_mutex);                          
                          
  if (!isOpen()) {
    throw RebotBackendException("Device is closed",
                                RebotBackendException::EX_DEVICE_CLOSED);
  }

  _protocolImplementor->read(addressInBytes, data, sizeInBytes);
}

void RebotBackend::write(uint8_t /*bar*/, uint32_t addressInBytes, int32_t const* data,
                         size_t sizeInBytes) {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
  if (!isOpen()) {
    throw RebotBackendException("Device is closed",
                                RebotBackendException::EX_DEVICE_CLOSED);
  }

  _protocolImplementor->write(addressInBytes, data, sizeInBytes);
}

void RebotBackend::close() {
  
  std::lock_guard<std::mutex> lock(_mutex);
  
  _opened = false;
  _tcpCommunicator->closeConnection();
  _protocolImplementor.reset(0);
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

uint32_t RebotBackend::getServerProtocolVersion() {
  // send a negotiation to the server:
  // sendClientProtocolVersion
  std::vector<uint32_t> clientHelloMessage = frameClientHello();
  _tcpCommunicator->sendData(clientHelloMessage);

  // Kludge is needed to work around server bug.
  // We have a bug with the old version were only one word is returned for multiple
  // unrecognized command. Fetching one word for the 3 words send is a workaround.
  auto serverHello = _tcpCommunicator->receiveData(1);

  if (serverHello.at(0) == (int32_t) UNKNOWN_INSTRUCTION) {
    return 0; // initial protocol version 0.0
  }

  auto remainingBytesOfAValidServerHello =
      _tcpCommunicator->receiveData(LENGTH_OF_HELLO_TOKEN_MESSAGE - 1);

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
