#include "RebotBackend.h"
#include "RebotProtocol1.h"
#include "RebotProtocolDefinitions.h"
#include "Connection.h"
#include "testableRebotSleep.h"
#include <boost/bind.hpp>
#include <sstream>

namespace ChimeraTK {

  RebotBackend::RebotBackend(std::string boardAddr, std::string port, std::string mapFileName)
  : NumericAddressedBackend(mapFileName), _boardAddr(boardAddr), _port(port),
    _threadInformerMutex(boost::make_shared<ThreadInformerMutex>()),
    _tcpCommunicator(boost::make_shared<Rebot::Connection>(_boardAddr, _port)), _protocolImplementor(),
    _lastSendTime(testable_rebot_sleep::now()), _connectionTimeout(Rebot::DEFAULT_CONNECTION_TIMEOUT),
    _heartbeatThread(std::bind(&RebotBackend::heartbeatLoop, this, _threadInformerMutex)) {}

  RebotBackend::~RebotBackend() {
    { // extra scope for the lock guard
      std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

      // make sure the thread does not access any hardware when it gets the lock
      _threadInformerMutex->quitThread = true;

      if(isOpen()) {
        _tcpCommunicator->close();
      }
    } // end of the lock guard scope. We have to release the lock before waiting
      // for the thread to join
    _heartbeatThread.interrupt();
    _heartbeatThread.join();
  }

  void RebotBackend::open() {
    std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

    _tcpCommunicator->open();
    auto serverVersion = getServerProtocolVersion();
    if(serverVersion == 0) {
      _protocolImplementor.reset(new RebotProtocol0(_tcpCommunicator));
    }
    else if(serverVersion == 1) {
      _protocolImplementor.reset(new RebotProtocol1(_tcpCommunicator));
    }
    else {
      _tcpCommunicator->close();
      std::stringstream errorMessage;
      errorMessage << "Server protocol version " << serverVersion << " not supported!";
      throw ChimeraTK::runtime_error(errorMessage.str());
    }

    _opened = true;
  }

  void RebotBackend::read(uint8_t /*bar*/, uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) {
    std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

    if(!isOpen()) {
      throw ChimeraTK::logic_error("Device is closed");
    }

    _lastSendTime = testable_rebot_sleep::now();
    _protocolImplementor->read(addressInBytes, data, sizeInBytes);
  }

  void RebotBackend::write(uint8_t /*bar*/, uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) {
    std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

    if(!isOpen()) {
      throw ChimeraTK::logic_error("Device is closed");
    }

    _lastSendTime = testable_rebot_sleep::now();
    _protocolImplementor->write(addressInBytes, data, sizeInBytes);
  }

  void RebotBackend::close() {
    std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

    _opened = false;
    _tcpCommunicator->close();
    _protocolImplementor.reset(0);
  }

  boost::shared_ptr<DeviceBackend> RebotBackend::createInstance(std::string /*address*/,
      std::map<std::string, std::string>
          parameters) {
    if(parameters["ip"].size() == 0) {
      throw ChimeraTK::logic_error("TMCB IP address not found in the parameter list");
    }
    if(parameters["port"].size() == 0) {
      throw ChimeraTK::logic_error("TMCB port number not found in the parameter list");
    }

    std::string tmcbIP = parameters["ip"];
    std::string portNumber = parameters["port"];
    std::string mapFileName = parameters["map"];

    return boost::shared_ptr<RebotBackend>(new RebotBackend(tmcbIP, portNumber, mapFileName));
  }

  uint32_t RebotBackend::getServerProtocolVersion() {
    // send a negotiation to the server:
    // sendClientProtocolVersion
    _lastSendTime = testable_rebot_sleep::now();
    std::vector<uint32_t> clientHelloMessage = frameClientHello();
    _tcpCommunicator->write(clientHelloMessage);

    // Kludge is needed to work around server bug.
    // We have a bug with the old version were only one word is returned for
    // multiple unrecognized command. Fetching one word for the 3 words send is a
    // workaround.
    auto serverHello = _tcpCommunicator->read(1);

    if(serverHello.at(0) == static_cast<uint32_t>(Rebot::UNKNOWN_INSTRUCTION)) {
      return 0; // initial protocol version 0.0
    }

    auto remainingBytesOfAValidServerHello = _tcpCommunicator->read(Rebot::LENGTH_OF_HELLO_TOKEN_MESSAGE - 1);

    serverHello.insert(
        serverHello.end(), remainingBytesOfAValidServerHello.begin(), remainingBytesOfAValidServerHello.end());
    return parseRxServerHello(serverHello);
  }

  std::vector<uint32_t> RebotBackend::frameClientHello() {
    std::vector<uint32_t> clientHello;
    clientHello.push_back(Rebot::HELLO_TOKEN);
    clientHello.push_back(Rebot::MAGIC_WORD);
    clientHello.push_back(Rebot::CLIENT_PROTOCOL_VERSION);
    return clientHello;
  }

  uint32_t RebotBackend::parseRxServerHello(const std::vector<uint32_t>& serverHello) {
    // 3 rd element/word is the version word
    return serverHello.at(2);
  }

  void RebotBackend::heartbeatLoop(boost::shared_ptr<ThreadInformerMutex> threadInformerMutex) {
    while(true) {
      try {
        // only send a heartbeat if the connection was inactive for half of the
        // timeout period
        if((testable_rebot_sleep::now() - _lastSendTime) > boost::chrono::milliseconds(_connectionTimeout / 2)) {
          // scope for the lock guard is in the if statement
          std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);
          // To handle the race condition that the thread woke up while the
          // desructor was holding the lock and closes the socket: Check the flag
          // and quit if set
          if(threadInformerMutex->quitThread) {
            break;
          }
          // always update the last send time. Otherwise the sleep will be
          // ineffective for a closed connection and go to 100 & CPU load
          _lastSendTime = testable_rebot_sleep::now();
          if(_protocolImplementor) {
            _protocolImplementor->sendHeartbeat();
          }
        }
        // sleep without holding the lock. Sleep for half of the connection
        // timeout (plus 1 ms)
        testable_rebot_sleep::sleep_until(_lastSendTime + boost::chrono::milliseconds(_connectionTimeout / 2 + 1));
      }
      catch(ChimeraTK::runtime_error& e) {
        std::cerr << "RebotBackend: Sending heartbeat failed. Caught exception: " << e.what() << std::endl;
        std::cerr << "Closing connection." << std::endl;
        close();
      }
    } // while true
  }

} // namespace ChimeraTK
