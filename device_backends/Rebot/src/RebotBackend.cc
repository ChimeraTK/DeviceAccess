#include "RebotBackend.h"
#include "RebotProtocol1.h"
#include "RebotProtocolDefinitions.h"
#include "Connection.h"
#include "testableRebotSleep.h"
#include <boost/bind.hpp>
#include <sstream>

namespace ChimeraTK {

std::unique_ptr<RebotProtocolImplementor> getProtocolImplementor( boost::shared_ptr<Rebot::Connection>& c);
uint32_t getProtocolVersion(boost::shared_ptr<Rebot::Connection>& c);
uint32_t parseRxServerHello(const std::vector<uint32_t>& serverHello);

std::unique_ptr<RebotProtocolImplementor> getProtocolImplementor( boost::shared_ptr<Rebot::Connection>& c) {
  auto serverVersion = getProtocolVersion(c);
  if (serverVersion == 0) {
    return std::make_unique<RebotProtocol0>(c);
  } else if (serverVersion == 1) {
    return std::make_unique<RebotProtocol1>(c);
  } else {
    c->close();
    std::stringstream errorMessage;
    errorMessage << "Server protocol version " << serverVersion
                 << " not supported!";
    throw ChimeraTK::runtime_error(errorMessage.str());
  }
}

uint32_t getProtocolVersion(boost::shared_ptr<Rebot::Connection>& c) {
  // send a negotiation to the server:
  // sendClientProtocolVersion
  std::vector<uint32_t> clientHelloMessage;
  clientHelloMessage.push_back(Rebot::HELLO_TOKEN);
  clientHelloMessage.push_back(Rebot::MAGIC_WORD);
  clientHelloMessage.push_back(Rebot::CLIENT_PROTOCOL_VERSION);

  c->write(clientHelloMessage);

  // Kludge is needed to work around server bug.
  // We have a bug with the old version were only one word is returned for
  // multiple unrecognized command. Fetching one word for the 3 words send is a
  // workaround.
  auto serverHello = c->read(1);

  if (serverHello.at(0) == static_cast<uint32_t>(Rebot::UNKNOWN_INSTRUCTION)) {
    return 0; // initial protocol version 0.0
  }

  auto remainingBytesOfAValidServerHello = c->read(Rebot::LENGTH_OF_HELLO_TOKEN_MESSAGE - 1);

  serverHello.insert(serverHello.end(),
                     remainingBytesOfAValidServerHello.begin(),
                     remainingBytesOfAValidServerHello.end());
  return parseRxServerHello(serverHello);
}

uint32_t parseRxServerHello(const std::vector<uint32_t>& serverHello) {
  // 3 rd element/word is the version word
  return serverHello.at(2);
}

  RebotBackend::RebotBackend(std::string boardAddr, std::string port, std::string mapFileName)
  : NumericAddressedBackend(mapFileName), _boardAddr(boardAddr), _port(port),
    _threadInformerMutex(boost::make_shared<ThreadInformerMutex>()),
    _connection(boost::make_shared<Rebot::Connection>(_boardAddr, _port)), _protocolImplementor(),
    _lastSendTime(testable_rebot_sleep::now()), _connectionTimeout(Rebot::DEFAULT_CONNECTION_TIMEOUT),
    _heartbeatThread(std::bind(&RebotBackend::heartbeatLoop, this, _threadInformerMutex)) {}

  RebotBackend::~RebotBackend() {
    { // extra scope for the lock guard
      std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

      // make sure the thread does not access any hardware when it gets the lock
      _threadInformerMutex->quitThread = true;

      if(isOpen()) {
        _connection->close();
      }
    } // end of the lock guard scope. We have to release the lock before waiting
      // for the thread to join
    _heartbeatThread.interrupt();
    _heartbeatThread.join();
  }

  void RebotBackend::open() {
    std::lock_guard<std::mutex> lock(_threadInformerMutex->mutex);

    _connection->open();

    _lastSendTime = testable_rebot_sleep::now();
    _protocolImplementor = getProtocolImplementor(_connection);

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
    _connection->close();
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
