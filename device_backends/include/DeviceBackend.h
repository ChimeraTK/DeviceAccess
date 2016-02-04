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

namespace mtca4u {

  // some forward declarations
  class RegisterAccessor;
  template<typename T> class BufferingRegisterAccessor;
  template<typename T> class RegisterAccessor2Dimpl;

  /** The base class of an IO device.
   */
  class DeviceBackend : public boost::enable_shared_from_this<DeviceBackend> {

    public:

      /** Every virtual class needs a virtual desctructor. */
      virtual ~DeviceBackend();

      /** TODO documentation missing
       */
      virtual void open() = 0;

      /** TODO documentation missing
       */
      virtual void close() = 0;

      /** Read one or more words from the device.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  read, not just one word as in DeviceBackend::readArea! Make sure your buffer
       *  is large enough!
       */
      virtual void read(const std::string &regModule, const std::string &regName,
          int32_t *data, size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** Write one or more words from the device.
       *  @attention In case you leave data size at 0, the full size of the register is
       *  written, not just one word as in DeviceBackend::readArea! Make sure your buffer
       *  is large enough!
       */
      virtual void write(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) = 0;

      /** TODO documentation missing
       */
      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      /** TODO documentation missing
       */
      virtual void write(uint8_t bar, uint32_t address, int32_t const* data,  size_t sizeInBytes) = 0;

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

      virtual std::string readDeviceInfo() = 0;

      /** Return whether a device has been opened or not.
       *  As the variable already exists in the base class we implement this
       * function here to avoid
       *  having to reimplement the same, trivial return function over and over
       * again.
       */
      virtual bool isOpen() = 0;

      /** Return whether a device has been connected or not.
       * A device is considered connected when it is created.
       */
      virtual bool isConnected() = 0;

      /** Get a RegisterAccessor object from the register name, to read and write registers via user-provided
       * buffers and plain pointers.
       */
      virtual boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(
          const std::string &registerName, const std::string &module = std::string()) = 0;

      /** Get register accessor for 2-dimensional registers.
       */
      template<typename UserType>
      boost::shared_ptr< RegisterAccessor2Dimpl<UserType> > getRegisterAccessor2D(
          const std::string &dataRegionName, const std::string &module = std::string());

      /** Returns the register information aka RegisterInfo.
       *  This function was named getRegisterMap because RegisterInfoMap will be renamed.
       */
      virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const = 0;

      /** Get a complete list of RegisterInfo objects (mapfile::RegisterInfo) for one
       * module.
       *  The registers are in alphabetical order.
       */
      virtual std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const = 0;

      /** Get a complete list of RegisterAccessors for one module.
       *  The registers are in alphabetical order.
       */
      virtual std::list< boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &moduleName) = 0;

    protected:

      /** Virtual implementation of the getRegisterAccessor2D() template function. The return value must
       *  be of the type MultiplexedDataAccessor<UserType>*.
       */
      virtual void* getRegisterAccessor2Dimpl(const std::type_info &UserType, const std::string &dataRegionName,
          const std::string &module) = 0;

  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr< RegisterAccessor2Dimpl<UserType> > DeviceBackend::getRegisterAccessor2D(
      const std::string &dataRegionName, const std::string &module)
  {
    void *voidptr = getRegisterAccessor2Dimpl(typeid(UserType), dataRegionName, module);
    RegisterAccessor2Dimpl<UserType> *ptr = static_cast<RegisterAccessor2Dimpl<UserType>*>(voidptr);
    return boost::shared_ptr< RegisterAccessor2Dimpl<UserType> >(ptr);
  }

} // namespace mtca4u

#endif /*MTCA4U_DEVICE_BACKEND_H*/
