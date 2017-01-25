#ifndef CHIMERATK_REBOT_BACKEND_H
#define CHIMERATK_REBOT_BACKEND_H

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <mutex>
#include <thread>
#include <boost/thread.hpp>
#include "NumericAddressedBackend.h"


namespace ChimeraTK {
  using namespace mtca4u;

  class TcpCtrl;
  class RebotProtocolImplementor;

  // A helper class that contains a mutex and a flag.
  // The idea is to put it into a shared pointer and hand it to a thread which is sleeping a long
  // time. You can then detach the thread, tell it to finish and continue with the constructor
  // without having to wait for the tread to wake up and finish before you can join it.
  // The thread locks the mutex and checks if it should finish when it wakes up, which it
  // can do because the mutex and flag still exist thanks to the shared pointer.
  struct ThreadInformerMutex{
    std::mutex mutex;
    bool quitThread;
    ThreadInformerMutex() : quitThread(false){}
  };

  class RebotBackend : public NumericAddressedBackend {

    protected:
      std::string _boardAddr;
      int _port;
      boost::shared_ptr<TcpCtrl> _tcpCommunicator;
      boost::shared_ptr<ThreadInformerMutex> _threadInformerMutex;
      std::unique_ptr<RebotProtocolImplementor> _protocolImplementor;
                           
    public:
      RebotBackend(std::string boardAddr, int port, std::string mapFileName="");
      ~RebotBackend();
      /// The function opens the connection to the device
      void open() override;
      void close() override;
      void read(uint8_t bar, uint32_t addressInBytes, int32_t* data, size_t sizeInBytes) override;
      void write(uint8_t bar, uint32_t addressInBytes, int32_t const* data, size_t sizeInBytes) override;
      std::string readDeviceInfo() override { return std::string("RebotDevice"); }
      static boost::shared_ptr<DeviceBackend> createInstance(
          std::string host, std::string instance,
          std::list<std::string> parameters, std::string mapFileName);

   protected:
      // This is not in the protocol implementor. Only the result of the hello tells us
      // which implementor to instantiate.
      uint32_t getServerProtocolVersion();
      std::vector<uint32_t> frameClientHello();
      uint32_t parseRxServerHello(const std::vector<int32_t>& serverHello);

      void heartbeatLoop(boost::shared_ptr<ThreadInformerMutex> threadInformerMutex);
      boost::thread _heartbeatThread;
  };

} // namespace ChimeraTK

// backward compartibility definition
namespace mtca4u{
  using namespace ChimeraTK;
}

#endif /*CHIMERATK_REBOT_BACKEND_H*/
