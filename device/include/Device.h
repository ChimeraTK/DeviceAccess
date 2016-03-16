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

#include "ForwardDeclarations.h"
#include "DeviceException.h"
#include "BackendFactory.h"
#include "DeviceBackend.h"
#include "TwoDRegisterAccessor.h"
#include "BufferingRegisterAccessor.h"

// Note: for backwards compatibility there is RegisterAccessor.h and MultiplexedDataAccessor.h included at the end
// of this file.

namespace mtca4u {

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

      /** Destructor */
      virtual ~Device();

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

      /** Get a BufferingRegisterAccessor object for the given register.
       *
       *  The BufferingRegisterAccessor allows to read and write registers transparently by using the accessor object
       *  like a variable of the type UserType. If needed, the conversion to and from the UserType will be handled by
       *  the FixedPointConverter matching the register description in the map.
       *
       *  The optional arguments numberOfWords and wordOffsetInRegister allow to limit the accessor to a part of the
       *  register.
       *
       *  The last optional argument enforceRawAccess allows to enforce the raw access by disabling any possible
       *  conversion from the original hardware data type into the given UserType. Obtaining the accessor with a
       *  UserType unequal to the raw data type will fail and throw a DeviceException with the id EX_WRONG_PARAMETER.
       *
       *  Note: This function returns an object, not a (shared) pointer to the object. This is necessary to use
       *  operators (e.g. = or []) directly on the object. */
      template<typename UserType>
      BufferingRegisterAccessor<UserType> getBufferingRegisterAccessor(const RegisterPath &registerPathName,
          size_t numberOfWords=0, size_t wordOffsetInRegister=0, bool enforceRawAccess=false) const;

      /** Get a RegisterAccessor2D object for the given register. This allows to read and write transparently
       *  2-dimensional registers. The register accessor is similar to the 1-dimensional BufferingRegisterAccessor. */
      template<typename UserType>
      TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(const RegisterPath &registerPathName) const;

      /** Return the register catalogue with detailed information on all registers. */
      const RegisterCatalogue& getRegisterCatalogue() const;

      /** Return a device information string. Format depends on the backend, use for display only. */
      virtual std::string readDeviceInfo() const;

      /** \brief <b>Inefficient convenience function</b> to read a single-word register without obtaining an accessor.
       *
       *  \warning This function is inefficient as it creates and discards a register accessor in each call of the
       *  function. For a better performance, use register accessors instead (see Device::getBufferingRegisterAccessor).
       *
       *  If a multi-word register is read, only the first element is returned. This function only works for 1D
       *  registers. */
      template<typename UserType>
      UserType read(const RegisterPath &registerPathName, bool enforceRawAccess=false) const;

      /** \brief <b>Inefficient convenience function</b> to read a multi-word register without obtaining an accessor.
       *
       *  \warning This function is inefficient as it creates and discards a register accessor in each call of the
       *  function. For a better performance, use register accessors instead.
       *
       *  The function only works for 1D registers. For 2D registers, see Device::getTwoDRegisterAccessor. The argument
       *  numberOfWords specifies how many elements are read. The returned vector will have a length of numberOfWords or
       *  shorter. numberOfWords must not exceed the size of the register. The optional argument wordOffsetInRegister
       *  allows to skip the first wordOffsetInRegister elements of the register. */
      template<typename UserType>
      std::vector<UserType> read(const RegisterPath &registerPathName, size_t numberOfWords,
          size_t wordOffsetInRegister=0, bool enforceRawAccess=false) const;

      /** \brief <b>Inefficient convenience function</b> to write a single-word register without obtaining an accessor.
       *
       *  \warning This function is inefficient as it creates and discards a register accessor in each call of the
       *  function. For a better performance, use register accessors instead.
       *
       *  If a multi-word register is read, only the first element is written. This function only works for 1D
       *  registers.*/
      template<typename UserType>
      void write(const RegisterPath &registerPathName, UserType value, bool enforceRawAccess=false);

      /** \brief <b>Inefficient convenience function</b> to write a multi-word register without obtaining an accessor.
       *
       *  \warning This function is inefficient as it creates and discards a register accessor in each call of the
       *  function. For a better performance, use register accessors instead.
       *
       *  The function only works for 1D registers. For 2D registers, see Device::getTwoDRegisterAccessor. The optional
       *  argument wordOffsetInRegister allows to skip the first wordOffsetInRegister elements of the register. */
      template<typename UserType>
      void write(const RegisterPath &registerPathName, std::vector<UserType> &vector, size_t wordOffsetInRegister=0,
          bool enforceRawAccess=false);


      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readReg(uint32_t regOffset, int32_t *data, uint8_t bar) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readArea(uint32_t regOffset, int32_t *data, size_t size, uint8_t bar) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeArea(uint32_t regOffset, int32_t const *data, size_t size, uint8_t bar);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readReg(const std::string &regName, int32_t *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void readReg(const std::string &regName, const std::string &regModule,
          int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeReg(const std::string &regName, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::write() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      virtual void writeReg(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use the getBufferingRegisterAccessor instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      boost::shared_ptr<mtca4u::RegisterAccessor> getRegisterAccessor(const std::string &registerName,
          const std::string &module = std::string()) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use the RegisterCatalogue (see getRegisterCatalogue()) instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      std::list<mtca4u::RegisterInfoMap::RegisterInfo> getRegistersInModule(
          const std::string &moduleName) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use the RegisterCatalogue (see getRegisterCatalogue()) to obtain a list of
       *  registers and create accessors using getBufferingRegisterAccessor instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      std::list<  boost::shared_ptr<mtca4u::RegisterAccessor> > getRegisterAccessorsInModule(
          const std::string &moduleName) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use getRegisterCatalogue() instead.
       *  @todo Add printed runtime warning after release of version 0.8
       */
      boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Open by alias name instead.
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void open(boost::shared_ptr<DeviceBackend> deviceBackend, boost::shared_ptr<mtca4u::RegisterInfoMap> &registerMap);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Open by alias name instead.
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void open(boost::shared_ptr<DeviceBackend> deviceBackend);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use getRegisterAccessor2D() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      template<typename customClass>
      boost::shared_ptr<customClass> getCustomAccessor(
          const std::string &dataRegionName,
          const std::string &module = std::string()) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::getBufferingRegisterAccessor(RegisterPath(module)/registerName)
       *  instead.
       */
      template<typename UserType>
      BufferingRegisterAccessor<UserType> getBufferingRegisterAccessor(
          const std::string &module, const std::string &registerName) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use Device::getTwoDRegisterAccessor(RegisterPath(module)/registerName) instead.
       */
      template<typename UserType>
      TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(
          const std::string &module, const std::string &registerName) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void readDMA(uint32_t regOffset, int32_t *data, size_t size,
          uint8_t bar) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Change warning into runtime error after release of version 0.8
       */
      virtual void writeDMA(uint32_t regOffset, int32_t const *data, size_t size,
          uint8_t bar);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void readDMA(const std::string &regName, int32_t *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use readArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void readDMA(const std::string &regName, const std::string &regModule,
          int32_t *data, size_t dataSize = 0,
          uint32_t addRegOffset = 0) const;

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void writeDMA(const std::string &regName, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  This function is deprecated. Use writeArea() instead!
       *  @todo Add printed runtime warning after release of version 0.6
       */
      virtual void writeDMA(const std::string &regName,
          const std::string &regModule, int32_t const *data,
          size_t dataSize = 0, uint32_t addRegOffset = 0);

      /** \brief <b>DEPRECATED</b>
       *
       *  \deprecated
       *  A typedef for backward compatibility. Don't use this in new code. It will be removed in a future release.
       *  Use mtca4u::RegisterAccessor instead (which is not nested inside this class).
       */
      typedef mtca4u::RegisterAccessor RegisterAccessor;

    protected:

      boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

      void checkPointersAreNotNull() const;
  };

  /********************************************************************************************************************/

  template <typename customClass>
  boost::shared_ptr<customClass> Device::getCustomAccessor(
      const std::string &dataRegionName, const std::string &module) const {
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::getCustomAccessor() detected.                          **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "** Use Device::getTwoDRegisterAccessor() instead!                                              **" << std::endl;// LCOV_EXCL_LINE
    std::cerr << "*************************************************************************************************" << std::endl;// LCOV_EXCL_LINE
    return customClass::createInstance(dataRegionName,module,_deviceBackendPointer,boost::shared_ptr<RegisterInfoMap>());
  }

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessor<UserType> Device::getBufferingRegisterAccessor(const RegisterPath &registerPathName,
      size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) const {
    checkPointersAreNotNull();
    return BufferingRegisterAccessor<UserType>(
        _deviceBackendPointer->getBufferingRegisterAccessor<UserType>(registerPathName, numberOfWords,
            wordOffsetInRegister, enforceRawAccess) );
  }

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessor<UserType> Device::getBufferingRegisterAccessor(
      const std::string &module, const std::string &registerName) const {
    return getBufferingRegisterAccessor<UserType>(RegisterPath(module)/registerName);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  TwoDRegisterAccessor<UserType> Device::getTwoDRegisterAccessor(
      const RegisterPath &registerPathName) const {
    checkPointersAreNotNull();
    return TwoDRegisterAccessor<UserType>(_deviceBackendPointer->getTwoDRegisterAccessor<UserType>(registerPathName));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  TwoDRegisterAccessor<UserType> Device::getTwoDRegisterAccessor(
      const std::string &module, const std::string &registerName) const {
    checkPointersAreNotNull();
    return TwoDRegisterAccessor<UserType>(_deviceBackendPointer->getTwoDRegisterAccessor<UserType>(
        RegisterPath(module)/registerName));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  UserType Device::read(const RegisterPath &registerPathName, bool enforceRawAccess) const {
    auto acc = getBufferingRegisterAccessor<UserType>(registerPathName, enforceRawAccess);
    acc.read();
    return acc;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType> Device::read(const RegisterPath &registerPathName, size_t numberOfWords,
      size_t wordOffsetInRegister, bool enforceRawAccess) const {
    auto acc = getBufferingRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister,
        enforceRawAccess);
    acc.read();
    std::vector<UserType> vector;
    acc.swap(vector);
    return vector;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath &registerPathName, UserType value, bool enforceRawAccess) {
    auto acc = getBufferingRegisterAccessor<UserType>(registerPathName, enforceRawAccess);
    acc = value;
    acc.write();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath &registerPathName, std::vector<UserType> &vector, size_t wordOffsetInRegister,
      bool enforceRawAccess) {
     auto acc = getBufferingRegisterAccessor<UserType>(registerPathName, vector.size(), wordOffsetInRegister, enforceRawAccess);
     acc.swap(vector);
     acc.write();
     acc.swap(vector);
   }

} // namespace mtca4u

#endif /* MTCA4U_DEVICE_H */

//
// Include the register accessor header for backwards compatibility, as it used to be part of the Device class.
// This include must be at the end of this file as it uses the Device class.
//
#include "RegisterAccessor.h"
#include "MultiplexedDataAccessor.h"
