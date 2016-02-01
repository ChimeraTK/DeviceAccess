#ifndef _MTCA4U_DEVICE_BACKEND_H__
#define _MTCA4U_DEVICE_BACKEND_H__

#include "DeviceBackendException.h"
#include <string>

#include <stdint.h>
#include <fcntl.h>
#include <vector>
#include <typeinfo>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

namespace mtca4u {

  // some forward declarations
  class RegisterAccessor;
  template<typename T> class BufferingRegisterAccessor;

  /** The base class of an IO device.
   */
  class DeviceBackend : public boost::enable_shared_from_this<DeviceBackend> {

    public:
      virtual void open() = 0;
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

      /**
       *
       */
      virtual void read(uint8_t bar, uint32_t address, int32_t* data,  size_t sizeInBytes) = 0;

      /**
       *
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

      /** Every virtual class needs a virtual desctructor. */
      virtual ~DeviceBackend(){}

      /** Get a RegisterAccessor object from the register name, to read and write registers via user-provided
       * buffers and plain pointers.
       */
      boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(
          const std::string &registerName,
          const std::string &module = std::string()) const;

      /** Get a BufferingRegisterAccessor object from the register name, to read and write registers transparently
       *  by using the accessor object like a variable of the type UserType. Conversion to and from the UserType
       *  will be handled by the FixedPointConverter matching the register description in the map.
       *  Note: This function returns an object, not a (shared) pointer to the object. This is necessary to use
       *  operators (e.g. = or []) directly on the object.
       */
      template<typename UserType>
      BufferingRegisterAccessor<UserType> getBufferingRegisterAccessor(
          const std::string &module, const std::string &registerName) const;

      /**
       * returns an accssesor which is used for interpreting  data contained in a
       * memory region. Memory regions that require a custom interpretation would
       * be indicated by specific keywords in the mapfile. For example, a memory
       * region indicated by the keyword
       * AREA_MULTIPLEXED_SEQUENCE_<dataRegionName> indicates that this memory
       * region contains multiplexed data sequences. The intelligence for
       * interpreting this multiplexed data is provided through the custom class -
       * MultiplexedDataAccessor<userType>
       */
      template<typename customClass>
      boost::shared_ptr<customClass> getCustomAccessor(
          const std::string &dataRegionName,
          const std::string &module = std::string()) const;

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
      virtual std::list<mtca4u::RegisterAccessor> getRegisterAccessorsInModule(
          const std::string &moduleName) = 0;

    protected:

      /// helper function for getBufferingRegisterAccessor, needs to be implemented by each backend implementation
      virtual boost::any getBufferingRegisterAccessorImpl(const std::type_info &userType,
          const std::string &module, const std::string &registerName) const;

  };

  template <typename customClass>
  boost::shared_ptr<customClass> DeviceBackend::getCustomAccessor(
      const std::string &dataRegionName, const std::string &module) const {
    return (
        customClass::createInstance(dataRegionName, module, shared_from_this()));
  }

  template<typename UserType>
  BufferingRegisterAccessor<UserType> DeviceBackend::getBufferingRegisterAccessor(
      const std::string &module, const std::string &registerName) const {
      boost::any acc = getBufferingRegisterAccessorImpl(typeid(UserType), module, registerName);
      return boost::any_cast< BufferingRegisterAccessor<UserType> >( acc );
  }

} // namespace mtca4u

#endif /*_MTCA4U_DEVICE_BACKEND_H__*/
