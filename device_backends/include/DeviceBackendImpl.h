#ifndef MTCA4U_DEVICE_BACKEND_IMPL_H
#define MTCA4U_DEVICE_BACKEND_IMPL_H

#include <list>

#include "DeviceBackend.h"
#include "DeviceException.h"

namespace mtca4u {

  /**
   *  DeviceBackendImpl implements some basic functionality which should be available for all backends. This is
   *  required to allow proper decorator patterns which should not have this functionality in the decorator itself.
   */
  class DeviceBackendImpl: public DeviceBackend
  {

  public:

      DeviceBackendImpl();
      virtual ~DeviceBackendImpl();

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

      virtual const RegisterCatalogue& getRegisterCatalogue() const {
        return _catalogue;
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
      }// LCOV_EXCL_LINE

  protected:
      
      /** the register catalogue containing describing the registers known by this backend */
      RegisterCatalogue _catalogue;

      bool        _opened;
      bool        _connected;

      /** Templated default implementation to obtain the BackendBufferingRegisterAccessor */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor_impl(
          const std::string &registerName, const std::string &module);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( DeviceBackendImpl, getBufferingRegisterAccessor_impl, 2);

      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> /*registerMap*/) {}; // LCOV_EXCL_LINE only for compatibility!

  };

}//namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_IMPL_H*/
