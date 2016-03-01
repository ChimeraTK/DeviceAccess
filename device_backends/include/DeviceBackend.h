#ifndef MTCA4U_DEVICE_BACKEND_H
#define MTCA4U_DEVICE_BACKEND_H

#include <string>
#include <list>
#include <stdint.h>
#include <fcntl.h>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

#include "DeviceBackendException.h"
#include "RegisterInfoMap.h"
#include "DeviceException.h"
#include "VirtualFunctionTemplate.h"

namespace mtca4u {

  // forward declaration to make friends
  class Device;

  // some forward declarations
  class RegisterAccessor;
  template<typename T> class TwoDRegisterAccessorImpl;
  template<typename T> class BufferingRegisterAccessorImpl;

  /** The base class of an IO device.
   */
  class DeviceBackend : public boost::enable_shared_from_this<DeviceBackend> {

    public:

      /** Default constructor */
      DeviceBackend();

      /** Every virtual class needs a virtual desctructor. */
      virtual ~DeviceBackend();

      /** TODO documentation missing
       */
      virtual void open() = 0;

      /** TODO documentation missing
       */
      virtual void close() = 0;

      /** Read one or more words from the named register.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  read! Make sure your buffer is large enough!
       */
      virtual void read(const std::string &regModule, const std::string &regName,
          int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** Write one or more words to the name register.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  written! Make sure your buffer is large enough!
       */
      virtual void write(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** Read one or more words from the given memory address. Not all backends might support
       *  memory addresses, thus a not-implemented exception might be thrown.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  read! Make sure your buffer is large enough!
       */
      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      /** Write one or more words to the given memory address. Not all backends might support
       *  memory addresses, thus a not-implemented exception might be thrown.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  written! Make sure your buffer is large enough!
       */
      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) = 0;

      /** Return a device information string containing hardware details like the firmware version number or the
       *  slot number used by the board. The format and contained information of this string is completely
       *  backend implementation dependent, so the string may only be printed to the user as an informational
       *  output. Do not try to parse this string or extract information from it programmatically.
       */
      virtual std::string readDeviceInfo() = 0;

      /** Return whether a device has been opened or not.
       */
      virtual bool isOpen() = 0;

      /** Return whether a device has been connected or not. A device is considered connected when it is created.
       */
      virtual bool isConnected() = 0;

      /** Get a RegisterAccessor object from the register name, to read and write registers via user-provided
       * buffers and plain pointers.
       */
      virtual boost::shared_ptr<RegisterAccessor> getRegisterAccessor(
          const std::string &registerName, const std::string &module = std::string()) = 0;

      /** Get a BufferingRegisterAccessor object from the register name, to read and write registers transparently
       *  using a std::vector-like interface.
       */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor(
          const std::string &registerName, const std::string &module = std::string()) {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(getBufferingRegisterAccessor_impl, UserType, registerName, module);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE( getBufferingRegisterAccessor_impl,
          boost::shared_ptr< BufferingRegisterAccessorImpl<T> >(const std::string&, const std::string&) );

      /** Get register accessor for 2-dimensional registers.
       */
      template<typename UserType>
      boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > getTwoDRegisterAccessor(
          const std::string &dataRegionName, const std::string &module = std::string()) {
        return CALL_VIRTUAL_FUNCTION_TEMPLATE(getTwoDRegisterAccessor_impl, UserType, dataRegionName, module);
      }
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE( getTwoDRegisterAccessor_impl,
          boost::shared_ptr< TwoDRegisterAccessorImpl<T> >(const std::string&, const std::string&) );

      /** Returns the register information aka RegisterInfo.
       *  This function was named getRegisterMap because RegisterInfoMap will be renamed.
       */
      virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const = 0;

      /** Get a complete list of RegisterInfo objects (mapfile::RegisterInfo) for one
       * module.
       *  The registers are in alphabetical order.
       */
      virtual std::list<RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const = 0;

      /** Get a complete list of RegisterAccessors for one module.
       *  The registers are in alphabetical order.
       */
      virtual std::list< boost::shared_ptr<RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &moduleName) = 0;

      /** \deprecated {
       *  This function is deprecated. Use read() instead!
       *  @todo Add printed warning after release of version 0.2
       *  }
       */
      virtual void readDMA(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      /** \deprecated {
       *  This function is deprecated. Use write() instead!
       *  @todo Add printed warning after release of version 0.2
       *  }
       */
      virtual void writeDMA(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) = 0;

    protected:

      /** Templated default implementation to obtain the BackendBufferingRegisterAccessor */
      template<typename UserType>
      boost::shared_ptr< BufferingRegisterAccessorImpl<UserType> > getBufferingRegisterAccessor_impl(
          const std::string &registerName, const std::string &module);
      DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER( DeviceBackend, getBufferingRegisterAccessor_impl, 2);

      /// for compatibility functions only: replace the current register map with a new one.
      virtual void setRegisterMap(boost::shared_ptr<RegisterInfoMap> registerMap) = 0;

      friend class Device;

  };


} // namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_H*/
