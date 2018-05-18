/*
 * TransferElement.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TRANSFER_ELEMENT_H
#define CHIMERA_TK_TRANSFER_ELEMENT_H

#include <vector>
#include <string>
#include <typeinfo>
#include <list>
#include <functional>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include "DeviceException.h"
#include "TimeStamp.h"
#include "TransferFuture.h"
#include "VersionNumber.h"
#include "TransferElementID.h"
#include "ExperimentalFeatures.h"

namespace ChimeraTK {
  class PersistentDataStorage;
}

namespace ChimeraTK {

  class TransferGroup;

  using ChimeraTK::TransferFuture;

  /*******************************************************************************************************************/

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement : public boost::enable_shared_from_this<TransferElement> {

    public:

      /** Creates a transfer element with the specified name. */
      TransferElement(std::string const &name = std::string(), std::string const &unit = std::string(unitNotSet),
                      std::string const &description = std::string())
      : _name(name), _unit(unit), _description(description), isInTransferGroup(false) {}

      /** Copying and moving is not allowed */
      TransferElement(const TransferElement &other) = delete;
      TransferElement(TransferElement &&other) = delete;
      TransferElement& operator=(const TransferElement &other) = delete;
      TransferElement& operator=(TransferElement &&other) = delete;

      /** Abstract base classes need a virtual destructor. */
      virtual ~TransferElement();

      /** A typedef for more compact syntax */
      typedef boost::shared_ptr<TransferElement> SharedPtr;

      /** Returns the name that identifies the process variable. */
      const std::string& getName() const {
        return _name;
      }

      /** Returns the engineering unit. If none was specified, it will default to "n./a." */
      const std::string& getUnit() const {
        return _unit;
      }

      /** Returns the description of this variable/register */
      const std::string& getDescription() const {
        return _description;
      }

      /** Returns the \c std::type_info for the value type of this transfer element.
       *  This can be used to determine the type at runtime.
       */
      virtual const std::type_info& getValueType() const = 0;

      /** Read the data from the device. If AccessMode::wait_for_new_data was set, this function will block until new
       *  data has arrived. Otherwise it still might block for a short time until the data transfer was complete. */
      void read() {
        if(TransferElement::isInTransferGroup) {
          throw DeviceException("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed.",
              DeviceException::NOT_IMPLEMENTED);
        }
        if(hasActiveFuture) {
          activeFuture.wait();
          return;
        }
        this->readTransactionInProgress = false;
        preRead();
        doReadTransfer();
        postRead();
      }

      /** Read the next value, if available in the input buffer.
       *
       *  If AccessMode::wait_for_new_data was set, this function returns immediately and the return value indicated
       *  if a new value was available (<code>true</code>) or not (<code>false</code>).
       *
       *  If AccessMode::wait_for_new_data was not set, this function is identical to read(), which will still return
       *  quickly. Depending on the actual transfer implementation, the backend might need to transfer data to obtain
       *  the current value before returning. Also this function is not guaranteed to be lock free. The return value
       *  will be always true in this mode. */
      bool readNonBlocking() {
        if(hasActiveFuture) {
          if(activeFuture.hasNewData()) {
            activeFuture.wait();
            return true;
          }
          else {
            return false;
          }
        }
        this->readTransactionInProgress = false;
        preRead();
        bool ret = doReadTransferNonBlocking();
        if(ret) postRead();
        return ret;
      }

      /** Read the latest value, discarding any other update since the last read if present. Otherwise this function
       *  is identical to readNonBlocking(), i.e. it will never wait for new values and it will return whether a
       *  new value was available if AccessMode::wait_for_new_data is set. */
      bool readLatest() {
        bool ret = false;
        if(hasActiveFuture) {
          if(activeFuture.hasNewData()) {
            activeFuture.wait();
            ret = true;
          }
          else {
            return false;
          }
        }
        this->readTransactionInProgress = false;
        preRead();
        bool ret2 = doReadTransferLatest();
        if(ret2) postRead();
        return ret || ret2;
      }

      /** Read data from the device in the background and return a future which will be fulfilled when the data is
       *  ready. When the future is fulfilled, the transfer element will already contain the new data, there is no
       *  need to call read() or readNonBlocking() (which would trigger another data transfer).
       *
       *  It is allowed to call this function multiple times, which will return the same (shared) future until it
       *  is fulfilled. If other read functions (like read() or readNonBlocking()) are called before the future
       *  previously returned by this function was fulfilled, that call will be equivalent to the respective call
       *  on the future (i.e. TransferFuture::wait() resp. TransferFuture::hasNewData()) and thus the future will
       *  hae been used afterwards.
       *
       *  The future will be fulfilled at the time when normally read() would return. A call to this function is
       *  roughly logically equivalent to:
       *    boost::async( boost::bind(&TransferElement::read, this) );
       *  (Although such implementation would disallow accessing the user data buffer until the future is fulfilled,
       *  which is not the case for this function.)
       *
       *  Design note: A special type of future has to be returned to allow an abstraction from the implementation
       *  details of the backend. This allows - depending on the backend type - a more efficient implementation
       *  without launching a thread.
       *
       *  Note: This feature is still experimental. Expect API changes without notice! */
      TransferFuture& readAsync() {
        if(!hasActiveFuture) {
          ChimeraTK::ExperimentalFeatures::check("asynchronous read");

          // call preRead
          this->readTransactionInProgress = false;
          this->preRead();

          // initiate asynchronous transfer and return future
          activeFuture = doReadTransferAsync();
          hasActiveFuture = true;
        }
        return activeFuture;
      }

      /** Write the data to device. The return value is true, old data was lost on the write transfer (e.g. due to an
       *  buffer overflow). In case of an unbuffered write transfer, the return value will always be false. */
      bool write(ChimeraTK::VersionNumber versionNumber={}) {
        // Note: this override is final to prevent implementations from implementing this logic incorrectly. Originally
        // this function was non-virtual in TransferElement, but NDRegisterAccessorBridge has to use a different
        // implementation.
        if(TransferElement::isInTransferGroup) {
          throw DeviceException("Calling read() or write() on an accessor which is part of a TransferGroup is not allowed.",
              DeviceException::NOT_IMPLEMENTED);
        }
        this->writeTransactionInProgress = false;
        preWrite();
        bool ret = doWriteTransfer(versionNumber);
        postWrite();
        return ret;
      }

      /**
      * Returns the version number that is associated with the last transfer (i.e. last read or write). See
      * ChimeraTK::VersionNumber for details.
      */
      virtual ChimeraTK::VersionNumber getVersionNumber() const {
        return ChimeraTK::VersionNumber();
      }

      /** Check if transfer element is read only, i\.e\. it is readable but not writeable. */
      virtual bool isReadOnly() const = 0;

      /** Check if transfer element is readable. It throws an acception if you try to read and
       *  isReadable() is not true.*/
      virtual bool isReadable() const = 0;

      /** Check if transfer element is writeable. It throws an acception if you try to write and
       *  isWriteable() is not true.*/
      virtual bool isWriteable() const = 0;

      /** Read the data from the device but do not fill it into the user buffer of this TransferElement. Calling this
       *  function after preRead() and followed by postRead() is exactly equivalent to a call to just read().
       *
       *  Implementation note: This function must return within ~1 second after boost::thread::interrupt() has been
       *  called on the thread calling this function. */
      virtual void doReadTransfer() = 0;

      /** Read the data from the device without blocking but do not fill it into the user buffer of this
       *  TransferElement. Calling this function after preRead() and followed by postRead() is exactly equivalent to a
       *  call to just readNonBlocking(). For the return value, see readNonBlocking().
       */
      virtual bool doReadTransferNonBlocking() = 0;

      /** Read the latest data from the device without blocking but do not fill it into the user buffer of this
       *  TransferElement. Calling this function after preRead() and followed by postRead() is exactly equivalent to a
       *  call to just readLatest(). For the return value, see readNonBlocking().
       */
      virtual bool doReadTransferLatest() = 0;

      /** Start the actual asynchronous read transfer. This function must be implemented by the backends and will be
       *  called inside readAsync(). At that point, it is guaranteed that there is no unfinished transfer still ongoing
       *  (i.e. no still-valid TransferFuture is present) and preRead() has already been called.
       *
       *  The backend must never touch the user buffer (i.e. NDRegisterAccessor::buffer_2D) inside this function, as it
       *  may only be filled inside postRead(). postRead() will get called by the TransferFuture when the user calls
       *  wait().
       *
       *  Note: The TransferFuture returned by this function must have been constructed with this TransferElement as
       *  the transferElement in the constructor. Otherwise postRead() will not be properly called! */
      virtual TransferFuture doReadTransferAsync() = 0;

      /** Perform any pre-read tasks if necessary.
       *
       *  Called by read() etc. Also the TransferGroup will call this function before a read is executed directly
       *  on the underlying accessor. */
      void preRead() {
        if(readTransactionInProgress) return;
        doPreRead();
        readTransactionInProgress = true;
      }

      /** Backend specific implementation of preRead(). preRead() will call this function, but it will make sure that
       *  it gets called only once per transfer. */
    protected:
      virtual void doPreRead() {}
    public:

      /** Transfer the data from the device receive buffer into the user buffer, while converting the data into the
       *  user data format if needed.
       *
       *  Called by read() etc. Also the TransferGroup will call this function after a read was executed directly on
       *  the underlying accessor. This function must be implemented to extract the read data from the underlying
       *  accessor and expose it to the user. */
      void postRead() {
        if(!readTransactionInProgress) return;
        readTransactionInProgress = false;
        doPostRead();
        hasActiveFuture = false;
      }

      /** Backend specific implementation of postRead(). postRead() will call this function, but it will make sure that
       *  it gets called only once per transfer. */
    protected:
      virtual void doPostRead() {}
    public:

      /** Function called by the TransferFuture before entering a potentially blocking wait(). In contrast to a wait
       *  callback of a boost::future/promise, this function is not called when just checking whether the result is
       *  ready or not. Usually it is not necessary to implement this function, but decorators should pass it on. One
       *  use case is the ApplicationCore TestDecoratorRegisterAccessor, which needs to be informed before blocking
       *  the thread execution. */
      virtual void transferFutureWaitCallback() {}

      /** Transfer the data from the user buffer into the device send buffer, while converting the data from then
       *  user data format if needed.
       *
       *  Called by write(). Also the TransferGroup will call this function before a write will be executed directly
       *  on the underlying accessor. This function implemented be used to transfer the data to be written into the
       *  underlying accessor. */
      void preWrite() {
        if(writeTransactionInProgress) return;
        doPreWrite();
        writeTransactionInProgress = true;
      }

      /** Backend specific implementation of preWrite(). preWrite() will call this function, but it will make sure that
       *  it gets called only once per transfer. */
    protected:
      virtual void doPreWrite() {}
    public:

      /** Perform any post-write cleanups if necessary. If during preWrite() e.g. the user data buffer was swapped
       *  away, it must be swapped back in this function so the just sent data is available again to the calling
       *  program.
       *
       *  Called by write(). Also the TransferGroup will call this function after a write was executed directly
       *  on the underlying accessor. */
      void postWrite() {
        if(!writeTransactionInProgress) return;
        writeTransactionInProgress = false;
        doPostWrite();
      }

      /** Backend specific implementation of postWrite(). postWrite() will call this function, but it will make sure that
       *  it gets called only once per transfer. */
    protected:
      virtual void doPostWrite() {}
    public:

      /** Write the data to device. The return value is true, old data was lost on the write transfer (e.g. due to an
       *  buffer overflow). In case of an unbuffered write transfer, the return value will always be false.
       *
       *  Calling this function after preWrite() and followed by postWrite() is exactly equivalent to a call to
       *  write(). */
      virtual bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) = 0;

      /**
       *  Check whether the TransferElement can be used in places where the TransferElement "other" is currently used,
       *  e.g. to merge the two transfers. This function must be used in replaceTransferElement() by implementations
       *  which use other TransferElements, to determine if a used TransferElement shall be replaced with the
       *  TransferElement "other".
       *
       *  The purpose of this function is not to determine if at any point in the hierarchy an replacement could be
       *  done. This function only works on a single level. It is not used by the TransferGroup to determine
       *  replaceTransferElement() whether shall be used (it is always called). Instead this function can be used
       *  by decorators etc. inside their implementation of replaceTransferElement() to determine if they might swap
       *  their implementation(s).
       *
       *  Note for decorators and similar implementations: This function must not be decorated. It should only return
       *  true if this should actually be replaced with other in the call to replaceTransferElement() one level up in
       *  the hierarchy. If the replacement should be done further down in the hierarchy, simply return false. It
       *  should only return if other is fully identical to this (i.e. behaves identical in all situations but might be
       *  another instance).
       */
      virtual bool mayReplaceOther(const boost::shared_ptr<TransferElement const> &other) const {
        (void) other; // prevent warning
        return false;
      }

      /** @brief Deprecated, do not use
       *  @deprecated The time stamp will be replaced with a unique counter.
       *  Only used for backward compatibility with the control system adapter. All implementations
       *  in DeviceAccess will throw an exception with DeviceException::NOT_IMPLEMENTED.
       *
       *  Returns the time stamp associated with the current value of the transfer element.
       *  Typically, this is the time when the value was updated.
       */
      virtual TimeStamp getTimeStamp() const{
        throw DeviceException("getTimeStamp is not implemented in DeviceAccess.", DeviceException::NOT_IMPLEMENTED);
      }

      /** @brief Deprecated, do not use
       *  @deprecated Only used for backward compatibility with the control system adapter.
       *  This feature will be removed soon, maybe even before the next tag. DO NOT USE IT!!
       */
      virtual bool isArray() const{
        throw DeviceException("isArray is deprecated and intentionally not implemented in DeviceAccess.", DeviceException::NOT_IMPLEMENTED);
      }

      /**
       *  Obtain the underlying TransferElements with actual hardware access. If this transfer element
       *  is directly reading from / writing to the hardware, it will return a list just containing
       *  a shared pointer of itself.
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() = 0;

      /**
       *  Obtain the full list of TransferElements internally used by this TransferElement. The function is recursive,
       *  i.e. elements used by the elements returned by this function are also added to the list. It is guaranteed
       *  that the directly used elements are first in the list and the result from recursion is appended to the list.
       *
       *  Example: A decorator would return a list with its target TransferElement followed by the result of
       *  getInternalElements() called on its target TransferElement.
       *
       *  If this TransferElement is not using any other element, it should return an empty vector. Thus those elements
       *  which return a list just containing themselves in getHardwareAccessingElements() will return an empty list
       *  here in getInternalElements().
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      virtual std::list< boost::shared_ptr<TransferElement> > getInternalElements() = 0;

      /**
       *  Obtain the highest level implementation TransferElement. For TransferElements which are itself an
       *  implementation this will directly return a shared pointer to this. If this TransferElement is a user
       *  frontend, the pointer to the internal implementation is returned.
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() {
        return shared_from_this();
      }

      /**
       *  Search for all underlying TransferElements which are considered identicel (see sameRegister()) with
       *  the given TransferElement. These TransferElements are then replaced with the new element. If no underlying
       *  element matches the new element, this function has no effect.
       */
      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        (void) newElement; // prevent warning
      }

      /** Constant string to be used as a unit when the unit is not provided or known */
      static constexpr char unitNotSet[] = "n./a.";

      /**
      *  Associate a persistent data storage object to be updated on each write operation of this ProcessArray. If no
      *  persistent data storage as associated previously, the value from the persistent storage is read and send to
      *  the receiver.
      *
      *  Note: A call to this function will be ignored, if the TransferElement does not support persistent data
      *  storage (e.g. read-only variables or device registers) @todo TODO does this make sense?
      */
      virtual void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage>) {}

      /**
       * Obtain unique ID for this TransferElement. If this TransferElement is the abstractor side of the bridge, this
       * function will return the unique ID of the actual implementation. This means that e.g. two instances of
       * ScalarRegisterAccessor created by the same call to Device::getScalarRegisterAccessor() (e.g. by copying the
       * accessor to another using NDRegisterAccessorBridge::replace()) will have the same ID, while two instances
       * obtained by to difference calls to Device::getScalarRegisterAccessor() will have a different ID even when
       * accessing the very same register.
       */
      TransferElementID getId() const { return _id; }

    protected:

      /** Identifier uniquely identifying the TransferElement */
      std::string _name;

      /** Engineering unit. Defaults to "n./a.", if none was specified */
      std::string _unit;

      /** Description of this variable/register */
      std::string _description;

      /** The ID of this TransferElement */
      TransferElementID _id;

      /** Allow generating a unique ID from derived classes*/
      void makeUniqueId() {
        _id.makeUnique();
      }

      /** Flag whether this TransferElement has been added to a TransferGroup or not */
      bool isInTransferGroup;

      friend class TransferGroup;
      friend class TransferFuture;

      /** Flag whether a read transaction is in progress. This flag will be set in preRead() and cleared in postRead()
       *  and is used to prevent multiple calls to these functions during a single transfer. It should also be reset
       *  before starting a new read transaction - this happens only inside the implementation of read() etc. and in
       *  the TransferGroup. */
      bool readTransactionInProgress{false};

      /** Flag whether a write transaction is in progress. This flag is similar to readTransactionInProgress but
       *  affects preWrite() and postWrite(). */
      bool writeTransactionInProgress{false};

      /// Flag whether there is a valid activeFuture or not
      bool hasActiveFuture{false};

      /// last future returned by doReadTransferAsync() (valid if hasActiveFuture == true)
      TransferFuture activeFuture;

  };

} /* namespace ChimeraTK */
namespace std {

  /*******************************************************************************************************************/

  /** Hash function for putting TransferElementID e.g. into an std::unordered_map */
  template<>
  struct hash<ChimeraTK::TransferElementID> {
      std::size_t operator()(const ChimeraTK::TransferElementID &f) const {
          return std::hash<size_t>{}(f._id);
      }
  };

  /*******************************************************************************************************************/

  /** Comparison for putting TransferElementID e.g. into an std::map */
  template<>
  struct less<ChimeraTK::TransferElementID> {
    // these typedefs are mandatory before C++17, even though they seem to be unused by gcc
    typedef bool result_type;
    typedef ChimeraTK::TransferElementID first_argument_type;
    typedef ChimeraTK::TransferElementID second_argument_type;
    bool operator()(const ChimeraTK::TransferElementID &a, const ChimeraTK::TransferElementID &b) const {
      return a._id < b._id;
    }
  };
}
#endif /* CHIMERA_TK_TRANSFER_ELEMENT_H */
