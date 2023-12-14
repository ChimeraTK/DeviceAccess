// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AccessMode.h"
#include "Exception.h"
#include "ForwardDeclarations.h"
#include "MetadataCatalogue.h"
#include "RegisterCatalogue.h"
#include "VirtualFunctionTemplate.h"

#include <boost/enable_shared_from_this.hpp>

#include <string>

namespace ChimeraTK {

  /** The base class for backends providing IO functionality for the Device class.
   * Note that most backends should actually be based on the DeviceBackendImpl
   * class (unless it is a decorator backend). The actual IO is always performed
   * through register accessors, which is obtained through the
   *  getBufferingRegisterAccessor() member function. */
  class DeviceBackend : public boost::enable_shared_from_this<DeviceBackend> {
   public:
    /** Every virtual class needs a virtual desctructor. */
    virtual ~DeviceBackend() = default;

    /** Open the device */
    virtual void open() = 0;

    /** Close the device */
    virtual void close() = 0;

    /** Return whether a device has been opened or not. */
    virtual bool isOpen() = 0;

    /** Deprecated since 2022-03-03. Do not use. */
    [[deprecated]] virtual bool isConnected() = 0;

    /** Return whether a device is working as intended, usually this means it is opened
     *  and does not have any errors. The default implementation in DeviceBackendImpl reports
     *  (_opened && !_hasErrors). Backends can overload it to implement a more sophisticated error reporting. The
     *  implementation might involve a communication attempt with the device.
     *
     *  Notice: isFunctional() shall only return false if there are known errors (or the device is closed).
     *  If the working state is unknown, the response should be \c true. Client code will then try to read/write
     *  and might get an exception, while isFunctional()==false means you surely will get an exception.
     */
    virtual bool isFunctional() const noexcept = 0;

    /**
     *  Return the register catalogue with detailed information on all registers.
     */
    virtual RegisterCatalogue getRegisterCatalogue() const = 0;

    /**
     *  Return the device metadata catalogue
     */
    virtual MetadataCatalogue getMetadataCatalogue() const = 0;

    /** Get a NDRegisterAccessor object from the register name. */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl,
        boost::shared_ptr<NDRegisterAccessor<T>>(const RegisterPath&, size_t, size_t, AccessModeFlags));

    /** Return a device information string containing hardware details like the
     * firmware version number or the slot number used by the board. The format
     * and contained information of this string is completely backend
     * implementation dependent, so the string may only be printed to the user as
     * an informational output. Do not try to parse this string or extract
     * information from it programmatically. */
    virtual std::string readDeviceInfo() = 0;

    /**
     * Set the backend into an exception state.
     * All backends must remember this, turn off asyncronous reads and all accessors will throw a
     * ChimeraTK::runtime_error on read and write operations with the provided message string, until open() has been
     * called successfully.
     */
    virtual void setException(const std::string& message) noexcept = 0;

    /**
     *  Activate asyncronous read for all transfer elements where AccessMode::wait_for_new_data is set.
     *  If Device::activateAsyncRead() is called while the device is not opened or has an error, this call has no
     *  effect. If it is called when no deactivated transfer element exists, this call also has no effect. When
     *  Device::activateAsyncRead() returns, it is not guaranteed that all initial values have been received already.
     *
     *  For more details, see \ref transferElement_B_8_5 "Technical specification: TransferElement B.8.5".
     */
    virtual void activateAsyncRead() noexcept {}

    /**
     * Function to be called by backends when needing to check for an active exception. If an active exception is found,
     * the appropriate ChimeraTK::runtime_error is thrown by this function.
     */
    virtual void checkActiveException() = 0;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DeviceBackend::getRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    return CALL_VIRTUAL_FUNCTION_TEMPLATE(
        getRegisterAccessor_impl, UserType, registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
