#ifndef CHIMERATK_DEVICE_BACKEND_H
#define CHIMERATK_DEVICE_BACKEND_H

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <fcntl.h>
#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#include "AccessMode.h"
#include "Exception.h"
#include "ForwardDeclarations.h"
#include "RegisterCatalogue.h"
#include "RegisterInfoMap.h"
#include "VirtualFunctionTemplate.h"

namespace ChimeraTK {

  /** The base class for backends providing IO functionality for the Device class.
   * Note that most backends should actually be based on the DeviceBackendImpl
   * class (unless it is a decorator backend). The actual IO is always performed
   * through register accessors, which is obtained through the
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

    /** Return whether a device has been connected or not. A device is considered
     * connected when it is created. */
    virtual bool isConnected() = 0;

    /** Return wether a device is working as intended, usually this means it is opened
     *  and does not have any errors. The default implementeation in DeviceBackendImpl reports
     *  (_opened && !_hasErrors). Backends can overload it to implement a more sophisticated error reporting. The
     *  implementation might involve a communication attempt with the device.
     *
     *  Notice: isFunctional() shall only return false if there are known errors (or the device is closed).
     *  If the working state is unknown, the response should be \c true. Client code will then try to read/write
     *  and might get an exception, while isFunctional()==false means you surely will get an exception.
     */
    virtual bool isFunctional() const = 0;

    /** Return the register catalogue with detailed information on all registers.
     */
    virtual const RegisterCatalogue& getRegisterCatalogue() const = 0;

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

    /** Set the backend into an exception state.
     *  All backends must remember this, turn off asyncronous reads and all accessors will throw a ChimeraTK::runtime_error on read and write operations,
     *  saying that there is a "previous, unrecovered fault", until open() has been called successfully.
     */
    virtual void setException() = 0;

    /** DEPRECATED
     *
     *  \deprecated {
     *  This function is deprecated. Do not use the backend directly, always use a
     * Device.
     *  @todo Add runtine warning after release of version 0.12
     *  }
     */
    template<typename UserType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor(
        const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess);

    /** DEPRECATED
     *
     *  \deprecated {
     *  This function is deprecated. Do not use the backend directly, always use a
     * Device.
     *  @todo Add runtime warning after release of version 0.9
     *  }
     */
    virtual boost::shared_ptr<const RegisterInfoMap> getRegisterMap() const = 0;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DeviceBackend::getRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    return CALL_VIRTUAL_FUNCTION_TEMPLATE(
        getRegisterAccessor_impl, UserType, registerPathName, numberOfWords, wordOffsetInRegister, flags);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DeviceBackend::getRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, bool enforceRawAccess) {
    if(!enforceRawAccess) {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(getRegisterAccessor_impl, UserType, registerPathName, numberOfWords,
          wordOffsetInRegister, AccessModeFlags({}));
    }
    else {
      return CALL_VIRTUAL_FUNCTION_TEMPLATE(getRegisterAccessor_impl, UserType, registerPathName, numberOfWords,
          wordOffsetInRegister, AccessModeFlags({AccessMode::raw}));
    }
  }

} // namespace ChimeraTK

#endif /*CHIMERATK_DEVICE_BACKEND_H*/
