#ifndef MTCA4U_DEVICE_BACKEND_IMPL_H
#define MTCA4U_DEVICE_BACKEND_IMPL_H

#include <list>

#include "DeviceBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  /** DeviceBackend implements the "opened" functionality which before was in DeviceBackend.
   *  It is to be a base class for all the other implementations. Like this debBase
   *  becomes purely virtual, i.e. a real interface.
   */
  class DeviceBackendImpl: public DeviceBackend
  {

  public:

      DeviceBackendImpl() : _opened(false), _connected(true) {}
      virtual ~DeviceBackendImpl(){}

      virtual bool isOpen(){ return _opened; }

      virtual bool isConnected(){ return _connected; }

      virtual void read(uint8_t /*bar*/, uint32_t /*address*/, int32_t* /*data*/,  size_t /*sizeInBytes*/) {
        // implementing this read function is not mandatory, so we throw a not-implemented exception by default
        throw DeviceException("Reading by memory address is not supported by this backend.",DeviceException::NOT_IMPLEMENTED);
      }

      virtual void write(uint8_t /*bar*/, uint32_t /*address*/, int32_t const* /*data*/,  size_t /*sizeInBytes*/) {
        // implementing this read function is not mandatory, so we throw a not-implemented exception by default
        throw DeviceException("Writing by memory address is not supported by this backend.",DeviceException::NOT_IMPLEMENTED);
      }

      virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const  {
        // implementing this read function is not mandatory, so we throw a not-implemented exception by default
        throw DeviceException("Obtaining a register map is not supported by this backend.",DeviceException::NOT_IMPLEMENTED);
      }

      /** \deprecated {
       *  This function is deprecated. Use read() instead!
       *  @todo Add printed warning after release of version 0.2
       *  }
       */
      virtual void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) {
        read(bar, address, data,  sizeInBytes);
      }

      /** \deprecated {
       *  This function is deprecated. Use write() instead!
       *  @todo Add printed warning after release of version 0.2
       *  }
       */
      virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) {
        write(bar, address, data,  sizeInBytes);
      }

  protected:

      bool        _opened;
      bool        _connected;

      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> /*registerMap*/) {};


  };

}//namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_IMPL_H*/
