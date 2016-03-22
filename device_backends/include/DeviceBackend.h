#ifndef MTCA4U_DEVICE_BACKEND_H
#define MTCA4U_DEVICE_BACKEND_H

#include <string>
#include <list>
#include <stdint.h>
#include <fcntl.h>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

#include "ForwardDeclarations.h"
#include "DeviceBackendException.h"
#include "RegisterInfoMap.h"
#include "RegisterCatalogue.h"
#include "DeviceException.h"
#include "VirtualFunctionTemplate.h"

namespace mtca4u {

  /** The base class for backends providing IO functionality for the Device class. Note that most backends should
   *  actually be based on the DeviceBackendImpl class (unless it is a decorator backend).
   *  The actual IO is always performed through register accessors, which is obtained through the
   *  getBufferingRegisterAccessor() member function. */
  class DeviceBackend : public boost::enable_shared_from_this<DeviceBackend> {

    public:

      /** Every virtual class needs a virtual desctructor. */
      virtual ~DeviceBackend();

      /** Open the device */
      virtual void open() = 0;

      /** Close the device */
      virtual void close() = 0;

      /** Return whether a device has been opened or not. */
      virtual bool isOpen() = 0;

      /** Return whether a device has been connected or not. A device is considered connected when it is created. */
      virtual bool isConnected() = 0;

      /** Return the register catalogue with detailed information on all registers. */
      virtual const RegisterCatalogue& getRegisterCatalogue() const = 0;

      /** Get a NDRegisterAccessor object from the register name. */
      template<typename UserType>
      boost::shared_ptr< NDRegisterAccessor<UserType> > getBufferingRegisterAccessor(
          const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE( getBufferingRegisterAccessor_impl,
          boost::shared_ptr< NDRegisterAccessor<T> >(const RegisterPath&, size_t, size_t, bool) );

      /** Return a device information string containing hardware details like the firmware version number or the
       *  slot number used by the board. The format and contained information of this string is completely
       *  backend implementation dependent, so the string may only be printed to the user as an informational
       *  output. Do not try to parse this string or extract information from it programmatically. */
      virtual std::string readDeviceInfo() = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtine warning after release of version 0.9
       *  }
       */
      virtual void read(const std::string &regModule, const std::string &regName,
          int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtine warning after release of version 0.9
       *  }
       */
      virtual void write(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtime warning after release of version 0.9
       *  }
       */
      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtime warning after release of version 0.9
       *  }
       */
      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Remove after release of version 0.9
       *  }
       */
      void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes);

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Remove after release of version 0.9
       *  }
       */
      void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes);

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtime warning after release of version 0.9
       *  }
       */
      virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const = 0;

      /** DEPRECATED
       *
       *  \deprecated {
       *  This function is deprecated. Do not use the backend directly, always use a Device.
       *  @todo Add runtime warning after release of version 0.9
       *  }
       */
      virtual std::list<RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const = 0;

    protected:

      /// for compatibility functions only: replace the current register map with a new one.
      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> registerMap) = 0;

      friend class Device;

  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< NDRegisterAccessor<UserType> > DeviceBackend::getBufferingRegisterAccessor(
      const RegisterPath &registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) {
    return CALL_VIRTUAL_FUNCTION_TEMPLATE(getBufferingRegisterAccessor_impl, UserType, registerPathName, numberOfWords,
        wordOffsetInRegister, enforceRawAccess);
  }

} // namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_H*/
