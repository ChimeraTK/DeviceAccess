/*
 * Device.h
 */

#ifndef CHIMERA_TK_DEVICE_H
#  define CHIMERA_TK_DEVICE_H

#  include <boost/algorithm/string.hpp>
#  include <boost/shared_ptr.hpp>

#  include "AccessMode.h"
#  include "BackendFactory.h"
#  include "BufferingRegisterAccessor.h"
#  include "DeviceBackend.h"
#  include "Exception.h"
#  include "ForwardDeclarations.h"
#  include "OneDRegisterAccessor.h"
#  include "ScalarRegisterAccessor.h"
#  include "TwoDRegisterAccessor.h"
#  include "Utilities.h" // make setDMapFilePath() available

// Note: for backwards compatibility there is RegisterAccessor.h and
// MultiplexedDataAccessor.h included at the end of this file.

namespace ChimeraTK {

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

    /** Create device instance without associating a backend yet.
     *
     * A backend has to be explicitly associated using open method which
     * has the alias or CDD as argument.
     *
     * Usage:
     * \code
     *   Device d();
     *   d.open("(pci:pciedevs5?map=mps_v00.07.map)");
     * \endcode
     */
    Device()=default;

    /**
     * \brief Initialize device and accociate a backend.
     *
     * The device is not opened yet.
     *
     * Usage:
     * \code
     *   Device d("(pci:pciedevs5?map=mps_v00.07.map)");
     *   d.open();
     * \endcode
     *
     * \param aliasName The ChimeraTK device descriptor for the device.
     */
    Device(const std::string &aliasName);

    /** Destructor */
    virtual ~Device();

    /** Open a device by the given alias name from the DMAP file.
     */
    virtual void open(std::string const& aliasName);

    /** Re-open the device after previously closeing it by calling close().
     */
    virtual void open();

    /** Close the device. The connection with the alias name is kept so the device
     * can be re-opened using the open() function without argument.
     */
    virtual void close();

    /** Get a ScalarRegisterObject object for the given register.
     *
     *  The ScalarRegisterObject allows to read and write registers transparently
     * by using the accessor object like a variable of the type UserType. If
     * needed, the conversion to and from the UserType will be handled by a data
     * converter matching the register description in e.g. a map file.
     *
     *  The optional argument wordOffsetInRegister allows to access another but
     * the first word of a larger register.
     *
     *  The last optional argument flags allows to control certain details how the
     * register is accessed. It allows e.g. to enable raw access. See AccessMode
     * documentation for more details. Passing an access mode flag which is not
     * supported by the backend or the given register will raise a NOT_IMPLEMENTED
     * DeviceException. */
    template<typename UserType>
    ScalarRegisterAccessor<UserType> getScalarRegisterAccessor(const RegisterPath& registerPathName,
        size_t wordOffsetInRegister = 0, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /** Get a OneDRegisterAccessor object for the given register.
     *
     *  The OneDRegisterAccessor allows to read and write registers transparently
     * by using the accessor object like a vector of the type UserType. If needed,
     * the conversion to and from the UserType will be handled by a data converter
     * matching the register description in e.g. a map file.
     *
     *  The optional arguments numberOfWords and wordOffsetInRegister allow to
     * limit the accessor to a part of the register.
     *
     *  The last optional argument flags allows to control certain details how the
     * register is accessed. It allows e.g. to enable raw access. See AccessMode
     * documentation for more details. Passing an access mode flag which is not
     * supported by the backend or the given register will raise a NOT_IMPLEMENTED
     * DeviceException.
     *
     *  Hint: when passing the AccessModeFlags, just enclose the wanted flags in a
     * brace initialiser list without "AccessModeFlags", e.g.:
     * getOneDRegisterAccessor<int32_t>("reg",0,0,{AccessMode::raw}) */
    template<typename UserType>
    OneDRegisterAccessor<UserType> getOneDRegisterAccessor(const RegisterPath& registerPathName,
        size_t numberOfWords = 0, size_t wordOffsetInRegister = 0,
        const AccessModeFlags& flags = AccessModeFlags({})) const;

    /** Get a TwoDRegisterAccessor object for the given register. This allows to
     * read and write transparently 2-dimensional registers. The register accessor
     * is similar to the 1-dimensional BufferingRegisterAccessor.
     *
     *  The optional arguments allow to restrict the accessor to a region of
     * interest in the 2D register: numberOfElements specifies the number of
     * elements per channel to read from the register. elementsOffset is the first
     * element index for each channel to read.
     *
     *  TODO: Currently only the restriction to a smaller region of interest is
     * only possible in one dimension. Support for the second dimension should be
     * added as well, but it will involve an internal interface change. Since all
     * current backends will not be able to optimise the performance when reducing
     * the number of channels, this change is postponed. */
    template<typename UserType>
    TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(const RegisterPath& registerPathName,
        size_t numberOfElements = 0, size_t elementsOffset = 0,
        const AccessModeFlags& flags = AccessModeFlags({})) const;

    /** Return the register catalogue with detailed information on all registers.
     */
    const RegisterCatalogue& getRegisterCatalogue() const;

    /** Return a device information string. Format depends on the backend, use for
     * display only. */
    virtual std::string readDeviceInfo() const;

    /** Check if the device is currently opened. */
    bool isOpened() const;

    /** \brief <b>Inefficient convenience function</b> to read a single-word
     * register without obtaining an accessor.
     *
     *  \warning This function is inefficient as it creates and discards a
     * register accessor in each call of the function. For a better performance,
     * use register accessors instead (see Device::getScalarRegisterAccessor).
     *
     *  If a multi-word register is read, only the first element is returned. This
     * function only works for 1D registers. */
    template<typename UserType>
    UserType read(const RegisterPath& registerPathName, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /** \brief <b>Inefficient convenience function</b> to read a multi-word
     * register without obtaining an accessor.
     *
     *  \warning This function is inefficient as it creates and discards a
     * register accessor in each call of the function. For a better performance,
     * use register accessors instead (see Device::getOneDRegisterAccessor).
     *
     *  The function only works for 1D registers. For 2D registers, see
     * Device::getTwoDRegisterAccessor. The argument numberOfWords specifies how
     * many elements are read. The returned vector will have a length of
     * numberOfWords or shorter. numberOfWords must not exceed the size of the
     * register. The optional argument wordOffsetInRegister allows to skip the
     * first wordOffsetInRegister elements of the register. */
    template<typename UserType>
    std::vector<UserType> read(const RegisterPath& registerPathName, size_t numberOfWords,
        size_t wordOffsetInRegister = 0, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /** \brief <b>Inefficient convenience function</b> to write a single-word
     * register without obtaining an accessor.
     *
     *  \warning This function is inefficient as it creates and discards a
     * register accessor in each call of the function. For a better performance,
     * use register accessors instead (see Device::getScalarRegisterAccessor).
     *
     *  If a multi-word register is read, only the first element is written. This
     * function only works for 1D registers.*/
    template<typename UserType>
    void write(
        const RegisterPath& registerPathName, UserType value, const AccessModeFlags& flags = AccessModeFlags({}));

    /** \brief <b>Inefficient convenience function</b> to write a multi-word
     * register without obtaining an accessor.
     *
     *  \warning This function is inefficient as it creates and discards a
     * register accessor in each call of the function. For a better performance,
     * use register accessors instead (see Device::getOneDRegisterAccessor).
     *
     *  The function only works for 1D registers. For 2D registers, see
     * Device::getTwoDRegisterAccessor. The optional
     *  argument wordOffsetInRegister allows to skip the first
     * wordOffsetInRegister elements of the register. */
    template<typename UserType>
    void write(const RegisterPath& registerPathName,
        const std::vector<UserType>& vector,
        size_t wordOffsetInRegister = 0,
        const AccessModeFlags& flags = AccessModeFlags({}));

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  <b>Inefficient convenience function</b> to read a multi-word register
     * without obtaining an accessor. This version accepts a boolean flag to
     * enable raw access instead of the AccessModeFlags list. It is deprecated and
     * should not be used in new code. Use the new version with the
     * AccessModeFlags instead.
     *
     *  @todo Add printed runtime warning. Deprecated since version 0.12 */
    template<typename UserType>
    [[deprecated("Use new signature instead!")]] std::vector<UserType> read(const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  <b>Inefficient convenience function</b> to write a single-word register
     * without obtaining an accessor. This version accepts a boolean flag to
     * enable raw access instead of the AccessModeFlags list. It is deprecated and
     * should not be used in new code. Use the new version with the
     * AccessModeFlags instead.
     *
     *  @todo Add printed runtime warning. Deprecated since version 0.12 */
    template<typename UserType>
    [[deprecated("Use new signature instead!")]] void write(
        const RegisterPath& registerPathName, UserType value, bool enforceRawAccess);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  <b>Inefficient convenience function</b> to write a multi-word register
     * without obtaining an accessor. This version accepts a boolean flag to
     * enable raw access instead of the AccessModeFlags list. It is deprecated and
     * should not be used in new code. Use the new version with the
     * AccessModeFlags instead.
     *
     *  @todo Add printed runtime warning. Deprecated since version 0.12 */
    template<typename UserType>
    [[deprecated("Use new signature instead!")]] void write(const RegisterPath& registerPathName,
        std::vector<UserType>& vector, size_t wordOffsetInRegister, bool enforceRawAccess);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  Get a ScalarRegisterObject object for the given register.
     *  This version accepts a boolean flag to enable raw access instead of the
     * AccessModeFlags list. It is deprecated and should not be used in new code.
     * Use the new version with the AccessModeFlags instead.
     *
     *  @todo Add printed runtime warning. Deprecated since version 0.12 */

    template<typename UserType>
    [[deprecated("Use new signature instead!")]] ScalarRegisterAccessor<UserType> getScalarRegisterAccessor(
        const RegisterPath& registerPathName,
        size_t wordOffsetInRegister,
        bool enforceRawAccess) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  Get a OneDRegisterAccessor object for the given register.
     *  This version accepts a boolean flag to enable raw access instead of the
     * AccessModeFlags list. It is deprecated and should not be used in new code.
     * Use the new version with the AccessModeFlags instead.
     *
     *  @todo Add printed runtime warning. Deprecated since version 0.12 */
    template<typename UserType>
    [[deprecated("Use new signature instead!")]] OneDRegisterAccessor<UserType> getOneDRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister,
        bool enforceRawAccess) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    template<typename UserType>
    [[deprecated("Use getScalarRegisterAccessor or getOneDRegisterAccessor "
                 "instead!")]] BufferingRegisterAccessor<UserType>
        getBufferingRegisterAccessor(const RegisterPath& registerPathName,
            size_t numberOfWords = 0,
            size_t wordOffsetInRegister = 0,
            bool enforceRawAccess = false) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use read() instead!")]] virtual void readReg(uint32_t regOffset, int32_t* data, uint8_t bar) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeReg(uint32_t regOffset, int32_t data, uint8_t bar);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.8
     */
    [[deprecated("Use read() instead!")]] virtual void readArea(
        uint32_t regOffset, int32_t* data, size_t size, uint8_t bar) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeArea(
        uint32_t regOffset, int32_t const* data, size_t size, uint8_t bar);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void readReg(
        const std::string& regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use read() instead!")]] virtual void readReg(const std::string& regName, const std::string& regModule,
        int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeReg(
        const std::string& regName, int32_t const* data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use Device::write() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeReg(const std::string& regName,
        const std::string& regModule, int32_t const* data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use the getBufferingRegisterAccessor instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use getScalarRegisterAccessor() or getOneDRegisterAccessor() "
                 "instead!")]] boost::shared_ptr<ChimeraTK::RegisterAccessor>
        getRegisterAccessor(const std::string& registerName, const std::string& module = std::string()) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use the RegisterCatalogue (see
     * getRegisterCatalogue()) instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use getRegisterCatalogue() instead!")]] std::list<ChimeraTK::RegisterInfoMap::RegisterInfo>
        getRegistersInModule(const std::string& moduleName) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use the RegisterCatalogue (see
     * getRegisterCatalogue()) to obtain a list of registers and create accessors
     * using getBufferingRegisterAccessor instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use getRegisterCatalogue() instead and create accessors with "
                 "get*RegisterAccessor() functions!")]] std::list<boost::shared_ptr<ChimeraTK::RegisterAccessor>>
        getRegisterAccessorsInModule(const std::string& moduleName) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use getRegisterCatalogue() instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use getRegisterCatalogue() instead!")]] boost::shared_ptr<const RegisterInfoMap> getRegisterMap()
        const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Open by alias name instead.
     *  @todo Change warning into runtime error after release of version 0.9
     */
    [[deprecated("Open by alias or device identifier string instead!")]] virtual void open(
        boost::shared_ptr<DeviceBackend>
            deviceBackend,
        boost::shared_ptr<ChimeraTK::RegisterInfoMap>& registerMap);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Open by alias name instead.
     *  @todo Change warning into runtime error after release of version 0.9
     */
    [[deprecated("Open by alias or device identifier string instead!")]] virtual void open(
        boost::shared_ptr<DeviceBackend> deviceBackend);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use getTwoDRegisterAccessor() instead!
     *  @todo Change warning into runtime error after release of version 0.9
     */
    template<typename customClass>
    [[deprecated("Use getTwoDRegisterAccessor() instead!")]] boost::shared_ptr<customClass> getCustomAccessor(
        const std::string& dataRegionName,
        const std::string& module = std::string()) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use
     * Device::getBufferingRegisterAccessor(RegisterPath(module)/registerName)
     *  instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    template<typename UserType>
    [[deprecated("Use getOneDRegisterAccessor() "
                 "instead!")]] BufferingRegisterAccessor<UserType>
        getBufferingRegisterAccessor(const std::string& module, const std::string& registerName) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use
     * Device::getTwoDRegisterAccessor(RegisterPath(module)/registerName) instead.
     *  @todo Add printed runtime warning after release of version 0.9
     */
    template<typename UserType>
    [[deprecated("Use new signature instead!")]] TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(
        const std::string& module,
        const std::string& registerName) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use readArea() instead!
     *  @todo Change warning into runtime error after release of version 0.9
     */
    [[deprecated("Use read() instead!")]] virtual void readDMA(
        uint32_t regOffset, int32_t* data, size_t size, uint8_t bar) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use writeArea() instead!
     *  @todo Change warning into runtime error after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeDMA(
        uint32_t regOffset, int32_t const* data, size_t size, uint8_t bar);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use readArea() instead!
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use read() instead!")]] virtual void readDMA(
        const std::string& regName, int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use readArea() instead!
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use read() instead!")]] virtual void readDMA(const std::string& regName, const std::string& regModule,
        int32_t* data, size_t dataSize = 0, uint32_t addRegOffset = 0) const;

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use writeArea() instead!
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeDMA(
        const std::string& regName, int32_t const* data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  This function is deprecated. Use writeArea() instead!
     *  @todo Add printed runtime warning after release of version 0.9
     */
    [[deprecated("Use write() instead!")]] virtual void writeDMA(const std::string& regName,
        const std::string& regModule, int32_t const* data, size_t dataSize = 0, uint32_t addRegOffset = 0);

    /** \brief <b>DEPRECATED</b>
     *
     *  \deprecated
     *  A typedef for backward compatibility. Don't use this in new code. It will
     * be removed in a future release. Use ChimeraTK::BufferingRegisterAccessor
     * instead
     */
    typedef ChimeraTK::RegisterAccessor RegisterAccessor;

   protected:
    boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

    void checkPointersAreNotNull() const;
  };

  /********************************************************************************************************************/

  template<typename customClass>
  boost::shared_ptr<customClass> Device::getCustomAccessor(const std::string& dataRegionName,
      const std::string& module) const {
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Usage of deprecated function Device::getCustomAccessor() "
                 "detected.                          **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "** Use Device::getTwoDRegisterAccessor() instead!              "
                 "                                **"
              << std::endl; // LCOV_EXCL_LINE
    std::cerr << "***************************************************************"
                 "**********************************"
              << std::endl; // LCOV_EXCL_LINE
    return customClass::createInstance(
        dataRegionName, module, _deviceBackendPointer, boost::shared_ptr<RegisterInfoMap>());
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ScalarRegisterAccessor<UserType> Device::getScalarRegisterAccessor(const RegisterPath& registerPathName,
      size_t wordOffsetInRegister,
      const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return ScalarRegisterAccessor<UserType>(
        _deviceBackendPointer->getRegisterAccessor<UserType>(registerPathName, 1, wordOffsetInRegister, flags));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ScalarRegisterAccessor<UserType> Device::getScalarRegisterAccessor(const RegisterPath& registerPathName,
      size_t wordOffsetInRegister,
      bool enforceRawAccess) const {
    if(!enforceRawAccess) {
      return getScalarRegisterAccessor<UserType>(registerPathName, wordOffsetInRegister);
    }
    else {
      return getScalarRegisterAccessor<UserType>(
          registerPathName, wordOffsetInRegister, AccessModeFlags({AccessMode::raw}));
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  OneDRegisterAccessor<UserType> Device::getOneDRegisterAccessor(const RegisterPath& registerPathName,
      size_t numberOfWords, size_t wordOffsetInRegister, const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return OneDRegisterAccessor<UserType>(_deviceBackendPointer->getRegisterAccessor<UserType>(
        registerPathName, numberOfWords, wordOffsetInRegister, flags));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  OneDRegisterAccessor<UserType> Device::getOneDRegisterAccessor(const RegisterPath& registerPathName,
      size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) const {
    if(!enforceRawAccess) {
      return OneDRegisterAccessor<UserType>(_deviceBackendPointer->getRegisterAccessor<UserType>(
          registerPathName, numberOfWords, wordOffsetInRegister, {}));
    }
    else {
      return OneDRegisterAccessor<UserType>(_deviceBackendPointer->getRegisterAccessor<UserType>(
          registerPathName, numberOfWords, wordOffsetInRegister, {AccessMode::raw}));
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  TwoDRegisterAccessor<UserType> Device::getTwoDRegisterAccessor(const RegisterPath& registerPathName,
      size_t numberOfElements, size_t elementsOffset, const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return TwoDRegisterAccessor<UserType>(_deviceBackendPointer->getRegisterAccessor<UserType>(
        registerPathName, numberOfElements, elementsOffset, flags));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  UserType Device::read(const RegisterPath& registerPathName, const AccessModeFlags& flags) const {
    auto acc = getScalarRegisterAccessor<UserType>(registerPathName, 0, flags);
    acc.read();
    return acc;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType> Device::read(const RegisterPath& registerPathName, size_t numberOfWords,
      size_t wordOffsetInRegister, const AccessModeFlags& flags) const {
    auto acc = getOneDRegisterAccessor<UserType>(registerPathName, numberOfWords, wordOffsetInRegister, flags);
    acc.read();
    std::vector<UserType> vector(acc.getNElements());
    acc.swap(vector);
    return vector;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath& registerPathName, UserType value, const AccessModeFlags& flags) {
    auto acc = getScalarRegisterAccessor<UserType>(registerPathName, 0, flags);
    acc = value;
    acc.write();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath& registerPathName, const std::vector<UserType>& vector,
      size_t wordOffsetInRegister, const AccessModeFlags& flags) {
    auto acc = getOneDRegisterAccessor<UserType>(registerPathName, vector.size(), wordOffsetInRegister, flags);
    // the vector stays constant, but we have to work around so we can swap it and
    // later swap it back...
    std::vector<UserType>& mutable_vector = const_cast<std::vector<UserType>&>(vector);
    acc.swap(mutable_vector);
    acc.write();
    acc.swap(mutable_vector);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType> Device::read(const RegisterPath& registerPathName, size_t numberOfWords,
      size_t wordOffsetInRegister, bool enforceRawAccess) const {
    return read<UserType>(registerPathName, numberOfWords, wordOffsetInRegister,
        (enforceRawAccess ? AccessModeFlags({AccessMode::raw}) : AccessModeFlags({})));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath& registerPathName, UserType value, bool enforceRawAccess) {
    write(registerPathName, value, (enforceRawAccess ? AccessModeFlags({AccessMode::raw}) : AccessModeFlags({})));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void Device::write(const RegisterPath& registerPathName, std::vector<UserType>& vector, size_t wordOffsetInRegister,
      bool enforceRawAccess) {
    write(registerPathName, vector, wordOffsetInRegister,
        (enforceRawAccess ? AccessModeFlags({AccessMode::raw}) : AccessModeFlags({})));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessor<UserType> Device::getBufferingRegisterAccessor(const RegisterPath& registerPathName,
      size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) const {
    checkPointersAreNotNull();
    return BufferingRegisterAccessor<UserType>(_deviceBackendPointer->getRegisterAccessor<UserType>(
        registerPathName, numberOfWords, wordOffsetInRegister, enforceRawAccess));
  }

  /********************************************************************************************************************/

  template<typename UserType>
  BufferingRegisterAccessor<UserType> Device::getBufferingRegisterAccessor(const std::string& module,
      const std::string& registerName) const {
    return getBufferingRegisterAccessor<UserType>(RegisterPath(module) / registerName);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  TwoDRegisterAccessor<UserType> Device::getTwoDRegisterAccessor(const std::string& module,
      const std::string& registerName) const {
    checkPointersAreNotNull();
    return TwoDRegisterAccessor<UserType>(
        _deviceBackendPointer->getRegisterAccessor<UserType>(RegisterPath(module) / registerName, 0, 0, false));
  }

} // namespace ChimeraTK

#endif /* CHIMERA_TK_DEVICE_H */

//
// Include the register accessor header for backwards compatibility, as it used
// to be part of the Device class. This include must be at the end of this file
// as it uses the Device class.
//
#include "RegisterAccessor.h"
