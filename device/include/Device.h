#ifndef MTCA4U_DEVICE_H
#define MTCA4U_DEVICE_H

/**
 *      @file           Device.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @brief          Template that connect functionality of libdev and libmap
 *libraries.
 *                      This file support only map file parsing.
 *
 */
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>

#include "DeviceException.h"
#include "BackendFactory.h"
#include "DeviceBackend.h"
#include "TwoDRegisterAccessor.h"

// Note: for backwards compatibility there is RegisterAccessor.h and MultiplexedDataAccessor.h included at the end
// of this file. @todo add warning when relying on these includes after relase of version 0.6

namespace mtca4u {

  // Just declare the class, no need to include the header because
  // it is template code and the header is included by the calling code.
  template<typename UserType>
  class BufferingRegisterAccessor;

  // Also just declare this class, since it is only used as template arguments within this header file.
  // The corresponding include defining the class is included where needed (and for comaptibility at the
  // end of this file).
  class RegisterAccessor;

  /**
   *      @class  Device
   *      @brief  Class allows to read/write registers from device
   *
   *      TODO update documentation
   *      Allows to read/write registers from device by passing the name of
   *      the register instead of offset from the beginning of address space.
   *      Type of the object used to control access to device must be passed
   *      as a template parameter and must be an type defined in libdev class.
   *
   *      The device can open and close a device for you. If you let the Device
   *      open the device you will not be able to get a handle to this device
   *      directly, you can only close it with the Device. Should you create
   *      RegisterAccessor objects, which contain shared pointers to this
   *      device, the device will stay opened and functional even if the
   *      Device object which created the RegisterAccessor goes out of scope.
   *      In this case you cannot close the device. It will finally be closed
   *      when the the last RegisterAccessor pointing to it goes out of scope.
   *      The same holds if you open another device with the same Device: You
   *      lose direct access to the previous device, which stays open as long
   *      as there are RegisterAccessors pointing to it.
   */
  class Device {
    public:

      /** Open a device by the given alias name from the DMAP file.
       */
      virtual void open(std::string const & aliasName);

      /** Re-open the device after previously closeing it by calling close().
       */
      virtual void open();

      /** Close the device. The connection with the alias name is kept so the device can be re-opened using the
       *  open() function without argument.
       */
      virtual void close();

      /** TODO add missing documentation
       */
      virtual void readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const;

      /** TODO add missing documentation
       */
      virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);

      /** TODO add missing documentation
       */
      virtual void readArea(uint32_t regOffset, int32_t *data, size_t size, uint8_t bar) const;

      /** TODO add missing documentation
       */
      virtual void writeArea(uint32_t regOffset, int32_t const *data, size_t size, uint8_t bar);

      /** TODO add missing documentation
       */
      virtual std::string readDeviceInfo() const;

      /** Read one or more words from the device. It calls DeviceBackend::readArea, not
       * DeviceBackend::readRaw.
       *  @attention In case you leave data size at 0, the full size of the
       * register
       * is read, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       *  The original readRaw function without module name, kept for backward
       * compatibility.
       *  The signature was changed and not extended to keep the functionality of
       * the default parameters for
       *  dataSize and addRegOffset, as the module name will always be needed in
       * the
       * future.
       */
      virtual void readReg(const std::string &regName, int32_t *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** Read one or more words from the device. It calls DeviceBackend::readArea, not
       * DeviceBackend::readRaw.
       *  @attention In case you leave data size at 0, the full size of the
       * register
       * is read, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       */
      virtual void readReg(const std::string &regName, const std::string &regModule,
          int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      /** Write one or more words from the device. It calls DeviceBackend::writeArea,
       * not
       * DeviceBackend::writeRaw.
       *  @attention In case you leave data size at 0, the full size of the
       * register
       * is written, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       *  The original writeRaw function without module name, kept for backward
       * compatibility.
       *  The signature was changed and not extendet to keep the functionality of
       * the default parameters for
       *  dataSize and addRegOffset, as the module name will always be needed in
       * the
       * future.
       */
      virtual void writeReg(const std::string &regName, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** Write one or more words from the device. It calls DeviceBackend::writeArea,
       * not
       * DeviceBackend::writeRaw.
       *  @attention In case you leave data size at 0, the full size of the
       * register
       * is written, not just one
       *  word as in DeviceBackend::readArea! Make sure your buffer is large enough!
       */
      virtual void writeReg(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** Get a RegisterAccessor object from the register name, to read and write registers via user-provided
       * buffers and plain pointers.
       */
      boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(const std::string &registerName,
          const std::string &module = std::string()) const;

      /** Get a BufferingRegisterAccessor object for the given register, to read and write registers transparently
       *  by using the accessor object like a variable of the type UserType. Conversion to and from the UserType
       *  will be handled by the FixedPointConverter matching the register description in the map.
       *  Note: This function returns an object, not a (shared) pointer to the object. This is necessary to use
       *  operators (e.g. = or []) directly on the object.
       */
      template<typename UserType>
      BufferingRegisterAccessor<UserType> getBufferingRegisterAccessor(
          const std::string &module, const std::string &registerName) const;

      /** Get a RegisterAccessor2D object for the given register. This allows to read and write transparently
       *  2-dimensional registers. The register accessor is similar to the 1-dimensional BufferingRegisterAccessor.
       */
      template<typename UserType>
      TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(
          const std::string &module, const std::string &registerName) const;

      /** Return the register catalogue with detailed information on all registers. */
      const RegisterCatalogue& getRegisterCatalogue() const;

      /** Get a complete list of RegisterInfo objects (mapfile::RegisterInfo) for one
       * module.
       *  The registers are in alphabetical order.
       *
       *  \deprecated
       *  This function is deprecated.
       */
      std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const;

      /** Get a complete list of RegisterAccessors for one module.
       *  The registers are in alphabetical order.
       *
       *  \deprecated
       *  This function is deprecated.
       */
      std::list<  boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &moduleName) const;

      /** Returns the register information aka RegisterInfo.
       *  This function was named getRegisterMap because RegisterInfoMap will be renamed.
       *
       *  \deprecated
       *  This function is deprecated.
       */
      boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

      /** Destructor
       */
      virtual ~Device();

      /** \deprecated
       *  This function is deprecated. Open by alias name instead.
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void open(boost::shared_ptr<DeviceBackend> deviceBackend, boost::shared_ptr<mtca4u::RegisterInfoMap> &registerMap);

      /** \deprecated
       *  This function is deprecated. Open by alias name instead.
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void open(boost::shared_ptr<DeviceBackend> deviceBackend);

      /** \deprecated
       *  This function is deprecated. Use getRegisterAccessor2D() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      template<typename customClass>
      boost::shared_ptr<customClass> getCustomAccessor(
          const std::string &dataRegionName,
          const std::string &module = std::string()) const;

      /** \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void readDMA(uint32_t regOffset, int32_t *data, size_t size,
          uint8_t bar) const;
      /** \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
          uint8_t bar);

      /** \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void readDMA(const std::string &regName, int32_t *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void readDMA(const std::string &regName, const std::string &regModule,
          int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      /** \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void writeDMA(const std::string &regName, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void writeDMA(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** A typedef for backward compatibility.
       *  @deprecated Don't use this in new code. It will be removed in a future release.
       *  Use mtca4u::RegisterAccessor instead (which is not nested inside this class).
       */
      typedef mtca4u::RegisterAccessor RegisterAccessor;

    protected:

      boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

      void checkPointersAreNotNull() const;
  };

  template <typename customClass>
  boost::shared_ptr<customClass> Device::getCustomAccessor(
      const std::string &dataRegionName, const std::string &module) const {
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::getCustomAccessor() detected.                          **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use Device::getTwoDRegisterAccessor() instead!                                              **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    return customClass::createInstance(dataRegionName,module,_deviceBackendPointer,boost::shared_ptr<RegisterInfoMap>());
  }

  template<typename UserType>
  BufferingRegisterAccessor<UserType> Device::getBufferingRegisterAccessor(
      const std::string &module, const std::string &registerName) const {
    return BufferingRegisterAccessor<UserType>(
        _deviceBackendPointer->getBufferingRegisterAccessor<UserType>(module, registerName) );
  }

  template<typename UserType>
  TwoDRegisterAccessor<UserType> Device::getTwoDRegisterAccessor(
      const std::string &module, const std::string &registerName) const {
    return TwoDRegisterAccessor<UserType>(_deviceBackendPointer->getTwoDRegisterAccessor<UserType>(registerName, module));
  }

} // namespace mtca4u

#endif /* MTCA4U_DEVICE_H */

//
// Include the register accessor header for backwards compatibility, as it used to be part of the Device class.
// This include must be at the end of this file as it uses the Device class.
//
#include "RegisterAccessor.h"
#include "MultiplexedDataAccessor.h"
