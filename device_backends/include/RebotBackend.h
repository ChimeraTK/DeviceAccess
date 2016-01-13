//#include <mtca4u/DeviceBackend.h>
//#include <mtca4u/devConfigBase.h>
#include "DeviceBackendImpl.h"

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

class TcpCtrl;
namespace mtca4u {

class RebotBackend : public DeviceBackendImpl {

private:
  std::string _boardAddr;
  int _port;
  boost::shared_ptr<TcpCtrl> _tcpObject;

public:
  RebotBackend(std::string boardAddr, int port);
  ~RebotBackend();
  ///The function opens the connection to the device
  virtual void open();
  virtual void close();
  virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);
  virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);
  ///Not implemented
  virtual std::string readDeviceInfo () {return std::string("RebotDevice");}
  ///Not implemented
  virtual void readDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/, size_t /*sizeInBytes*/) {};
  ///Not implemented
  virtual void writeDMA(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/, size_t /*sizeInBytes*/) {};
  static boost::shared_ptr<DeviceBackend> createInstance(std::string host, std::string instance, std::list<std::string> parameters);

private:
  /*!
   * @brief Frame a rebot 'n' word read request and send it over the socket.
   *
   * @param address     The start address to read from. Method expects this
   *                    address to be byte aligned to the register address
   *                    space.
   * @param wordsToRead Number of 32 bit words that should be returned from the
   *                    start address (value at start address inclusive).
   */
  void sendRebotReadRequest(uint32_t const address, uint32_t const wordsToRead);


  /*!
   * @brief Copy contents of a vector to a preallocated memory region
   */
  void transferVectorToDataPtr(std::vector<int32_t> source, int32_t* destination);
};

} //namespace mtca4u
