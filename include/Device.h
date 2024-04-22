// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AccessMode.h"
#include "DeviceBackend.h"
#include "ForwardDeclarations.h"
#include "OneDRegisterAccessor.h"
#include "ScalarRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"
#include "VoidRegisterAccessor.h"

#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>

// These headers are added for convenience of application programmers:
#include "BackendFactory.h" // IWYU pragma: keep
#include "Exception.h"      // IWYU pragma: keep
#include "Utilities.h"      // IWYU pragma: keep (for setDMapFilePath())

namespace ChimeraTK {

  /**
   * Class allows to read/write registers from device
   *
   * @todo update documentation
   * Allows to read/write registers from device by passing the name of the register instead of offset from the beginning
   * of address space. Type of the object used to control access to device must be passed as a template parameter and
   * must be an type defined in libdev class.
   *
   * The device can open and close a device for you. If you let the Device open the device you will not be able to get
   * a handle to this device directly, you can only close it with the Device. Should you create RegisterAccessor
   * objects, which contain shared pointers to this device, the device will stay opened and functional even if the
   * Device object which created the RegisterAccessor goes out of scope. In this case you cannot close the device. It
   * will finally be closed when the the last RegisterAccessor pointing to it goes out of scope. The same holds if you
   * open another device with the same Device: You lose direct access to the previous device, which stays open as long
   * as there are RegisterAccessors pointing to it.
   */
  class Device {
   public:
    /**
     * Create device instance without associating a backend yet.
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
    Device() = default;

    /**
     * Initialize device and accociate a backend.
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
    explicit Device(const std::string& aliasName);

    /**
     * Open a device by the given alias name from the DMAP file.
     */
    void open(std::string const& aliasName);

    /**
     * Re-open the device after previously closeing it by calling close(), or when it was constructed with a given
     * aliasName.
     */
    void open();

    /**
     * Close the device. The connection with the alias name is kept so the device can be re-opened using the open()
     * function without argument.
     */
    void close();

    /**
     * Get a ScalarRegisterObject object for the given register.
     *
     * The ScalarRegisterObject allows to read and write registers transparently by using the accessor object like a
     * variable of the type UserType. If needed, the conversion to and from the UserType will be handled by a data
     * converter matching the register description in e.g. a map file.
     *
     * The optional argument wordOffsetInRegister allows to access another but the first word of a larger register.
     *
     * The last optional argument flags allows to control certain details how the register is accessed. It allows e.g.
     * to enable raw access. See AccessMode documentation for more details. Passing an access mode flag which is not
     * supported by the backend or the given register will raise a ChimeraTK::logic_error.
     */
    template<typename UserType>
    [[nodiscard]] ScalarRegisterAccessor<UserType> getScalarRegisterAccessor(const RegisterPath& registerPathName,
        size_t wordOffsetInRegister = 0, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * Get a VoidRegisterAccessor object for the given register.
     */
    [[nodiscard]] VoidRegisterAccessor getVoidRegisterAccessor(
        const RegisterPath& registerPathName, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * Get a OneDRegisterAccessor object for the given register.
     *
     * The OneDRegisterAccessor allows to read and write registers transparently by using the accessor object like a
     * vector of the type UserType. If needed, the conversion to and from the UserType will be handled by a data
     * converter matching the register description in e.g. a map file.
     *
     * The optional arguments numberOfWords and wordOffsetInRegister allow to limit the accessor to a part of the
     * register.
     *
     * The last optional argument flags allows to control certain details how the register is accessed. It allows e.g.
     * to enable raw access. See AccessMode documentation for more details. Passing an access mode flag which is not
     * supported by the backend or the given register will raise a NOT_IMPLEMENTED DeviceException.
     *
     * Hint: when passing the AccessModeFlags, just enclose the wanted flags in a brace initialiser list without
     * "AccessModeFlags", e.g.: getOneDRegisterAccessor<int32_t>("reg",0,0,{AccessMode::raw})
     */
    template<typename UserType>
    [[nodiscard]] OneDRegisterAccessor<UserType> getOneDRegisterAccessor(const RegisterPath& registerPathName,
        size_t numberOfWords = 0, size_t wordOffsetInRegister = 0,
        const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * Get a TwoDRegisterAccessor object for the given register. This allows to read and write transparently
     * 2-dimensional registers. The register accessor is similar to the 1-dimensional BufferingRegisterAccessor.
     *
     * The optional arguments allow to restrict the accessor to a region of interest in the 2D register:
     * numberOfElements specifies the number of elements per channel to read from the register. elementsOffset is the
     * first element index for each channel to read.
     *
     * TODO: Currently only the restriction to a smaller region of interest is only possible in one dimension. Support
     * for the second dimension should be added as well, but it will involve an internal interface change. Since all
     * current backends will not be able to optimise the performance when reducing the number of channels, this change
     * is postponed.
     */
    template<typename UserType>
    TwoDRegisterAccessor<UserType> getTwoDRegisterAccessor(const RegisterPath& registerPathName,
        size_t numberOfElements = 0, size_t elementsOffset = 0,
        const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * Return the register catalogue with detailed information on all registers.
     */
    [[nodiscard]] RegisterCatalogue getRegisterCatalogue() const;

    /**
     * Return the register catalogue with detailed information on all registers.
     */
    [[nodiscard]] MetadataCatalogue getMetadataCatalogue() const;

    /**
     * Return a device information string. Format depends on the backend, use for display only.
     */
    [[nodiscard]] std::string readDeviceInfo() const;

    /**
     * Check if the device is currently opened.
     */
    [[nodiscard]] bool isOpened() const;

    /**
     * Return wether a device is working as intended, usually this means it is opened and does not have any errors. If a
     * backend does not implement any error reporting, this functions id equivalent to isOpened().
     */
    [[nodiscard]] bool isFunctional() const;

    /**
     * Activate asyncronous read for all transfer elements where AccessMode::wait_for_new_data is set.
     * If Device::activateAsyncRead() is called while the device is not opened or has an error, this call has no
     * effect. If it is called when no deactivated transfer element exists, this call also has no effect. When
     * Device::activateAsyncRead() returns, it is not guaranteed that all initial values have been received already.
     *
     * For more details, see \ref transferElement_B_8_5 "Technical specification: TransferElement B.8.5".
     */
    void activateAsyncRead() noexcept;

    /**
     * Set the device into an exception state.
     *
     * \li All asynchronous reads will be deactivated. Transfer elements with asynchronous read will reveice a
     * ChimeraTK::runtime_error.
     * \li All write and synchronous read operations will see a ChimeraTK::runtime_error.
     * \li The exception state stays until open() has successfully been called again.
     */
    void setException(const std::string& message);

    /**
     * Obtain the backend.
     *
     * Note: using the backend in normal application code likely breaks the abstraction.
     */
    [[nodiscard]] boost::shared_ptr<DeviceBackend> getBackend();

    /**
     * <b>Inefficient convenience function</b> to read a single-word register without obtaining an accessor.
     *
     * \warning This function is inefficient as it creates and discards a register accessor in each call of the
     * function. For a better performance, use register accessors instead (see Device::getScalarRegisterAccessor).
     *
     * If a multi-word register is read, only the first element is returned. This function only works for 1D registers.
     */
    template<typename UserType>
    [[nodiscard]] UserType read(
        const RegisterPath& registerPathName, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * <b>Inefficient convenience function</b> to read a multi-word register without obtaining an accessor.
     *
     * \warning This function is inefficient as it creates and discards a register accessor in each call of the
     * function. For a better performance, use register accessors instead (see Device::getOneDRegisterAccessor).
     *
     * The function only works for 1D registers. For 2D registers, see Device::getTwoDRegisterAccessor. The argument
     * numberOfWords specifies how many elements are read. The returned vector will have a length of numberOfWords or
     * shorter. numberOfWords must not exceed the size of the register. The optional argument wordOffsetInRegister
     * allows to skip the first wordOffsetInRegister elements of the register.
     */
    template<typename UserType>
    [[nodiscard]] std::vector<UserType> read(const RegisterPath& registerPathName, size_t numberOfWords,
        size_t wordOffsetInRegister = 0, const AccessModeFlags& flags = AccessModeFlags({})) const;

    /**
     * <b>Inefficient convenience function</b> to write a single-word register without obtaining an accessor.
     *
     * \warning This function is inefficient as it creates and discards a register accessor in each call of the
     * function. For a better performance, use register accessors instead (see Device::getScalarRegisterAccessor).
     *
     * If a multi-word register is read, only the first element is written. This function only works for 1D registers
     */
    template<typename UserType>
    void write(
        const RegisterPath& registerPathName, UserType value, const AccessModeFlags& flags = AccessModeFlags({}));

    /** <b>Inefficient convenience function</b> to write a multi-word register without obtaining an accessor.
     *
     * \warning This function is inefficient as it creates and discards a register accessor in each call of the
     * function. For a better performance, use register accessors instead (see Device::getOneDRegisterAccessor).
     *
     * The function only works for 1D registers. For 2D registers, see Device::getTwoDRegisterAccessor. The optional
     * argument wordOffsetInRegister allows to skip the first wordOffsetInRegister elements of the register.
     */
    template<typename UserType>
    void write(const RegisterPath& registerPathName, const std::vector<UserType>& vector,
        size_t wordOffsetInRegister = 0, const AccessModeFlags& flags = AccessModeFlags({}));

   protected:
    boost::shared_ptr<DeviceBackend> _deviceBackendPointer;

    void checkPointersAreNotNull() const;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  ScalarRegisterAccessor<UserType> Device::getScalarRegisterAccessor(
      const RegisterPath& registerPathName, size_t wordOffsetInRegister, const AccessModeFlags& flags) const {
    checkPointersAreNotNull();
    return ScalarRegisterAccessor<UserType>(
        _deviceBackendPointer->getRegisterAccessor<UserType>(registerPathName, 1, wordOffsetInRegister, flags));
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto& mutable_vector = const_cast<std::vector<UserType>&>(vector);
    acc.swap(mutable_vector);
    acc.write();
    acc.swap(mutable_vector);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
