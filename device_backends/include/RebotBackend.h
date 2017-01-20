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
#include "NumericAddressedBackend.h"
#include "NotImplementedException.h"


namespace ChimeraTK {
  using namespace mtca4u;

  class TcpCtrl;

 class RebotBackend : public NumericAddressedBackend {

    private:
      std::string _boardAddr;
      int _port;
      boost::shared_ptr<TcpCtrl> _tcpObject;
      uint32_t _serverProtocolVersion;
      std::mutex _mutex;
    public:
      RebotBackend(std::string boardAddr, int port, std::string mapFileName="");
      ~RebotBackend();
      /// The function opens the connection to the device
      virtual void open();
      virtual void close();
      virtual void read(uint8_t bar, uint32_t addressInBytes, int32_t* data,
          size_t sizeInBytes);
      virtual void write(uint8_t bar, uint32_t addressInBytes, int32_t const* data,
          size_t sizeInBytes);
      virtual std::string readDeviceInfo() { return std::string("RebotDevice"); }
      // Don't include deprecated, throwing functions in the coverage report:
      // LCOV_EXCL_START 
      /// Deprecated and not implemented. Always throws NotImplmentedException
      virtual void readDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/,
			   size_t /*sizeInBytes*/) {
	throw NotImplementedException("Deprecated function RebotBackend::readDMA is not implemented!");
      };
      /// Deprecated and not implemented. Always throws NotImplmentedException
      virtual void writeDMA(uint8_t /*bar*/, uint32_t /*address*/,
          int32_t const* /*data*/, size_t /*sizeInBytes*/) {
	throw NotImplementedException("Deprecated function RebotBackend::writeDMA is not implemented!");
      };
      // LCOV_EXCL_STOP      
      static boost::shared_ptr<DeviceBackend> createInstance(
          std::string host, std::string instance,
          std::list<std::string> parameters, std::string mapFileName);
    private:
      /*!
       * @brief Frame a rebot 'n' word read request and send it over the socket.
       *
       * @param wordAddress The start address to read from. Method expects this
       *                    address to be byte aligned to the register address
       *                    space.
       * @param wordsToRead Number of 32 bit words that should be returned from the
       *                    start address (value at start address inclusive).
       */
      void sendRebotReadRequest(uint32_t const wordAddress,
          uint32_t const wordsToRead);

      /*!
       * @brief Copy contents of a vector to a preallocated memory region.
       *
       * @param source      int32_t vector with data to be copied
       * @param destination Pointer to allocated memory region with sufficient space
       *                    to accommodate all elements of source.
       */
      void transferVectorToDataPtr(std::vector<int32_t> source,
          int32_t* destination);

      /*!
       * @brief read and put contents in dataLocation
       *
       * @param wordAddress   Address Of the register to read from (assumes provided
       *                      address is word aligned).
       * @param numberOfWords Number of words to return.
       * @param dataLocation  Location to copy the words into. Assumes has space to
       *                      accommodate the returned values.
       */
      void fetchFromRebotServer(uint32_t wordAddress, uint32_t numberOfWords,
          int32_t* dataLocation);

      uint32_t getServerProtocolVersion();
      std::vector<uint32_t> frameClientHello();
      uint32_t parseRxServerHello(const std::vector<int32_t>& serverHello);
  };

} // namespace ChimeraTK

// backward compartibility definition
namespace mtca4u{
  using namespace ChimeraTK;
}

#endif /*CHIMERATK_REBOT_BACKEND_H*/
