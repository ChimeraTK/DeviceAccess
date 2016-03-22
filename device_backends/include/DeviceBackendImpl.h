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

  protected:
      
      /** the register catalogue containing describing the registers known by this backend */
      RegisterCatalogue _catalogue;

      /** flag if device is opened */
      bool        _opened;
      
      /** flag if device is connected. */
      bool        _connected;

      /** Add plugin-provided decorators to a BufferingRegisterAccessor */
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > decorateBufferingRegisterAccessor(
          const RegisterPath &registerPathName, boost::shared_ptr< NDRegisterAccessor<UserType> > accessor) const {
        if(!_catalogue.hasRegister(registerPathName)) return accessor;
        auto info = _catalogue.getRegister(registerPathName);
        for(auto i = info->plugins_begin(); i != info->plugins_end(); ++i) {
          accessor = i->decorateBufferingRegisterAccessor<UserType>(accessor);
        }
        return accessor;
      }

      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> /*registerMap*/) {}; // LCOV_EXCL_LINE only for compatibility!

  };

}//namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_IMPL_H*/
