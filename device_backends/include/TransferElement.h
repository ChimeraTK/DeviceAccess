// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AccessMode.h"
#include "DeviceBackend.h"
#include "Exception.h"
#include "TransferElementID.h"
#include "VersionNumber.h"

#include <ChimeraTK/cppext/future_queue.hpp>

#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include <functional>
#include <iostream>
#include <list>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace ChimeraTK {
  class PersistentDataStorage;
  class TransferGroup;

  /**
   * @brief The current state of the data
   *
   * This is a flag to describe the validity of the data. It should be used to signalize
   * whether or not to trust the data currently. It MUST NOT be used to signalize any
   * communication errors with a device, rather to signalize the consumer after such an
   * error that the data is currently not trustable, because we are performing calculations
   * with the last known valid data, for example.
   */
  enum class DataValidity {
    ok,    /// The data is considered valid
    faulty /// The data is not considered valid
  };

  std::ostream& operator<<(std::ostream& os, const DataValidity& validity);

  /**
   * @brief Used to indicate the applicable operation on a Transferelement.
   */
  enum class TransferType { read, readNonBlocking, readLatest, write, writeDestructively };

  namespace detail {
    /**
     *  Exception to be thrown by continuations of the _readQueue when a value shall be discarded. This is needed to
     * avoid notifications of the application if a value should never reach the application. The exception is caught in
     * readTransferAyncWaitingImpl() and readTransferAyncNonWaitingImpl() and should never be visible to the
     * application.
     */
    class DiscardValueException {};

  } /* namespace detail */

  /*******************************************************************************************************************/

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement : public boost::enable_shared_from_this<TransferElement> {
   public:
    /** Creates a transfer element with the specified name. */
    TransferElement(std::string name, AccessModeFlags accessModeFlags, std::string unit = std::string(unitNotSet),
        std::string description = std::string())
    : _name(std::move(name)), _unit(std::move(unit)), _description(std::move(description)),
      _accessModeFlags(std::move(accessModeFlags)) {}

    /** Copying and moving is not allowed */
    TransferElement(const TransferElement& other) = delete;
    TransferElement(TransferElement&& other) = delete;
    TransferElement& operator=(const TransferElement& other) = delete;
    TransferElement& operator=(TransferElement&& other) = delete;

    /** Abstract base classes need a virtual destructor. */
    virtual ~TransferElement() = default;

    /** A typedef for more compact syntax */
    using SharedPtr = boost::shared_ptr<TransferElement>;

    /** Returns the name that identifies the process variable. */
    const std::string& getName() const { return _name; }

    /** Returns the engineering unit. If none was specified, it will default to
     * "n./a." */
    const std::string& getUnit() const { return _unit; }

    /** Returns the description of this variable/register */
    const std::string& getDescription() const { return _description; }

    /** Returns the \c std::type_info for the value type of this transfer element.
     *  This can be used to determine the type at runtime.
     */
    virtual const std::type_info& getValueType() const = 0;

    /** Return the AccessModeFlags for this TransferElement. */
    AccessModeFlags getAccessModeFlags() const { return _accessModeFlags; }

    /** Set the current DataValidity for this TransferElement. Will do nothing if the
     * backend does not support it */
    void setDataValidity(DataValidity validity = DataValidity::ok) { _dataValidity = validity; }

    /** Return current validity of the data. Will always return DataValidity::ok if the
     * backend does not support it */
    DataValidity dataValidity() const { return _dataValidity; }

    /** Read the data from the device. If AccessMode::wait_for_new_data was set,
     * this function will block until new data has arrived. Otherwise it still
     * might block for a short time until the data transfer was complete. */
    void read() {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on the TransferElement '" + _name +
            "' which is part of a TransferGroup is not allowed.");
      }
      this->readTransactionInProgress = false;

      preReadAndHandleExceptions(TransferType::read);
      if(!_activeException) {
        handleTransferException([&] { readTransfer(); });
      }

      postReadAndHandleExceptions(TransferType::read, !_activeException);
    }

    /**
     *  Read the next value, if available in the input buffer.
     *
     *  If AccessMode::wait_for_new_data was set, this function returns immediately and the return value indicated if
     *  a new value was available (<code>true</code>) or not (<code>false</code>).
     *
     *  If AccessMode::wait_for_new_data was not set, this function is identical to read(), which will still return
     *  quickly. Depending on the actual transfer implementation, the backend might need to transfer data to obtain
     *  the current value before returning. Also this function is not guaranteed to be lock free. The return value will
     *  be always true in this mode.
     */
    bool readNonBlocking() {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on the TransferElement '" + _name +
            "' which is part of a TransferGroup is not allowed.");
      }
      this->readTransactionInProgress = false;
      preReadAndHandleExceptions(TransferType::readNonBlocking);
      bool updateDataBuffer = false;
      if(!_activeException) {
        handleTransferException([&] { updateDataBuffer = readTransferNonBlocking(); });
      }

      bool retVal = updateDataBuffer;
      if(_activeException) {
        auto previousVersionNumber = _versionNumber;
        auto previousDataValidity = _dataValidity;
        // always call postRead with updateDataBuffer = false in case of an exception
        postReadAndHandleExceptions(TransferType::readNonBlocking, false);
        // Usually we do not reach this point because postRead() is re-throwing the _activeException.
        // If we reach this point the exception has been suppressed. We have to calculate a
        // new return value because the dataBuffer has not changed, but the meta data
        // could have, in which case we have to return true.
        retVal = (previousVersionNumber != _versionNumber) || (previousDataValidity != _dataValidity);
      }
      else {
        // call postRead with updateDataBuffer as returned by readTransferNonBlocking
        postReadAndHandleExceptions(TransferType::readNonBlocking, updateDataBuffer);
      }
      return retVal;
    }

    /** Read the latest value, discarding any other update since the last read if
     * present. Otherwise this function is identical to readNonBlocking(), i.e. it
     * will never wait for new values and it will return whether a new value was
     * available if AccessMode::wait_for_new_data is set. */
    bool readLatest() {
      if(_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        bool updateDataBuffer = false;
        // Call readNonBlocking until there is no new data to be read any more
        while(readNonBlocking()) {
          // remember whether we have new data
          updateDataBuffer = true;
        }
        return updateDataBuffer;
      }
      // Without wait_for_new_data readNonBlocking always returns true, and the while loop above would never end.
      // Hence we just call the (synchronous) read and return true;
      read();
      return true;
    }

    /** Write the data to device. The return value is true, old data was lost on
     * the write transfer (e.g. due to an buffer overflow). In case of an
     * unbuffered write transfer, the return value will always be false. */
    bool write(ChimeraTK::VersionNumber versionNumber = {}) {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on the TransferElement '" + _name +
            "' which is part of a TransferGroup is not allowed.");
      }
      this->writeTransactionInProgress = false;
      bool previousDataLost = true; // the value here does not matter. If there was an exception, it will be re-thrown
                                    // in postWrite, so it is never returned

      preWriteAndHandleExceptions(TransferType::write, versionNumber);
      if(!_activeException) {
        handleTransferException([&] { previousDataLost = writeTransfer(versionNumber); });
      }

      postWriteAndHandleExceptions(TransferType::write, versionNumber);
      return previousDataLost;
    }

    /** Just like write(), but allows the implementation to destroy the content of the user buffer in the
     *  process. This is an optional optimisation, hence there is a default implementation which just calls the normal
     *  doWriteTransfer(). In any case, the application must expect the user buffer of the TransferElement to contain
     *  undefined data after calling this function. */
    bool writeDestructively(ChimeraTK::VersionNumber versionNumber = {}) {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on the TransferElement '" + _name +
            "' which is part of a TransferGroup is not allowed.");
      }
      this->writeTransactionInProgress = false;

      preWriteAndHandleExceptions(TransferType::writeDestructively, versionNumber);
      bool previousDataLost = true; // the value here does not matter. If there was an exception, it will be re-thrown
                                    // in postWrite, so it is never returned
      if(!_activeException) {
        handleTransferException([&] { previousDataLost = writeTransferDestructively(versionNumber); });
      }

      postWriteAndHandleExceptions(TransferType::writeDestructively, versionNumber);
      return previousDataLost;
    }

    /**
     *  Returns the version number that is associated with the last transfer (i.e.
     * last read or write). See ChimeraTK::VersionNumber for details. The
     * VersionNumber object also allows to determine the time stamp.
     *
     *  Implementation notes:
     *
     * Reading accessors have to update the TransferElement _versionNumber variable in
     * their doPostRead function. For TransferElements with AccessMode::wait_for_new_data
     * it has to be created already when the data is received. It must be stored
     * together with the payload data and only written to the application buffer (which _versionNumber is a part of)
     * in postWrite.
     * Accessors which rely on other accessors to obtain their data update the value from their target
     * after a successful transfer.
     */
    ChimeraTK::VersionNumber getVersionNumber() const { return _versionNumber; }

    /** Check if transfer element is read only, i\.e\. it is readable but not
     * writeable. */
    virtual bool isReadOnly() const = 0;

    /** Check if transfer element is readable. It throws an exception if you try
     * to read and isReadable() is not true.*/
    virtual bool isReadable() const = 0;

    /** Check if transfer element is writeable. It throws an exception if you try
     * to write and isWriteable() is not true.*/
    virtual bool isWriteable() const = 0;

    /** Set an active exception. This function is called by all decorator-like TransferElements
     *  to propagate exceptions to their target.
     *  The argument is passed by reference. After returning from this function, it is {nullptr}.
     *  This function must not be called with nullptr (see spec FIXME).
     */
    void setActiveException(std::exception_ptr& setThisException) {
      if(setThisException) {
        _activeException = setThisException;
        setThisException = nullptr;
      }
    }

    /** Set the backend to which the exception has to be reported.
     *  Each backend has to do this when creating TransferElements. However, not
     *  all TransferElements will have it set, for instance ProcessArrays in the ControlSystemAdapter and Application
     * core,# which don't have backends at all. This function is only to be called inside of
     * DeviceBackend::getRegisterAccessor()!
     *
     *  It is virtual because some accessor implementations like NumericAddressedBackendRegisterAccessor have an inner
     * layer (LowLevelTransferElement), and all layers need to know the exception backend. Hence the functions needs to
     * be overridden in this case.
     */
    virtual void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) {
      _exceptionBackend = std::move(exceptionBackend);
    }

    /** Return the exception backend. Needed by decorators to set their _exceptionBackend to the target's.
     */
    boost::shared_ptr<DeviceBackend> getExceptionBackend() { return _exceptionBackend; }

    /** Function to get a copy of the read queue. This functions should only be used by decorators to initialise their
     *  TransferElement::_readQueue() member.
     */
    cppext::future_queue<void> getReadQueue() { return _readQueue; }

   protected:
    /** The backend to which the runtime_errors are reported via DeviceBackend::setException().
     *  Creating backends set it in DeviceBackend::getRegisterAccessor(). Decorators have to set it in the constructor
     * from their target.
     */
    boost::shared_ptr<DeviceBackend> _exceptionBackend;

   private:
    /**
     *  Helper for exception handling in the transfer functions, to avoid code duplication.
     */
    template<typename Callable>
    void handleTransferException(Callable function) {
      try {
        function();
      }
      catch(ChimeraTK::runtime_error&) {
        _activeException = std::current_exception();
      }
      catch(boost::thread_interrupted&) {
        _activeException = std::current_exception();
      }
    }

    // helper function that just gets rid of the DiscardValueException and otherwise does a pop_wait on the _readQueue.
    // It does not deal with other exceptions. This is done in handleTransferException.
    void readTransferAsyncWaitingImpl() {
    retry:
      try {
        _readQueue.pop_wait();
      }
      catch(detail::DiscardValueException&) {
        goto retry;
      }
    }

   public:
    /**
     *  Read the data from the device but do not fill it into the user buffer of this TransferElement. This function
     *  must be called after preRead() and before postRead(). If the accessor has the AccessMode::wait_for_new_data
     *  flag, the function will block until new data has been pushed by the sender.
     *
     *  This function internally calls doReadTransferSynchronously(), which is implemented by the backend, or waits
     *  for data on the _readQueue, depending whether AccessMode::wait_for_new_data is set.
     *  runtime_error exceptions thrown in the transfer are caught and rethrown in postRead().
     */
    void readTransfer() {
      if(_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        readTransferAsyncWaitingImpl();
      }
      else {
        doReadTransferSynchronously();
      }
    }

   protected:
    /**
     *  Implementation version of readTransfer() for synchronous reads. This function must be implemented by the
     * backend. For the functional description read the documentation of readTransfer().
     *
     *  Implementation notes:
     *  - This function must return within ~1 second after boost::thread::interrupt() has been called on the thread
     *    calling this function.
     *  - Decorators must delegate the call to readTransfer() of the decorated target.
     *  - Delegations within the same object should go to the "do" version, e.g. to
     * BaseClass::doReadTransferSynchronously()
     */
    virtual void doReadTransferSynchronously() = 0;

   private:
    // helper function that just gets rid of the DiscardValueException and otherwise does a pop on the _readQueue.
    // It does not deal with other exceptions. This is done in handleTransferException.
    bool readTransferAsyncNonWaitingImpl() {
    retry:
      try {
        return _readQueue.pop();
      }
      catch(detail::DiscardValueException&) {
        goto retry;
      }
    }

   public:
    /**
     *  Read the data from the device but do not fill it into the user buffer of this TransferElement. This function
     *  must be called after preRead() and before postRead(). Even if the accessor has the AccessMode::wait_for_new_data
     *  flag, this function will not block if no new data is available. For the meaning of the return value, see
     *  readNonBlocking().
     *
     *  For TransferElements with AccessMode::wait_for_new_data this function checks whether there is new data on the
     * _readQueue. Without AccessMode::wait_for_new_data it calls doReadTransferSynchronously, which is implemented by
     * the backend. runtime_error exceptions thrown in the transfer are caught and rethrown in postRead().
     */
    bool readTransferNonBlocking() {
      if(_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        return readTransferAsyncNonWaitingImpl();
      }
      doReadTransferSynchronously();
      return true;
    }

   private:
    /** Helper function to catch the exceptions. Avoids code duplication.*/
    void preReadAndHandleExceptions(TransferType type) noexcept {
      try {
        preRead(type);
      }
      catch(ChimeraTK::logic_error&) {
        _activeException = std::current_exception();
      }
      catch(ChimeraTK::runtime_error&) {
        _activeException = std::current_exception();
      }
      catch(boost::thread_interrupted&) {
        _activeException = std::current_exception();
      }
    }

   public:
    /** Perform any pre-read tasks if necessary.
     *
     *  Called by read() etc. Also the TransferGroup will call this function before a read is executed directly on the
     *  underlying accessor. */
    void preRead(TransferType type) {
      if(readTransactionInProgress) return;
      _activeException = {nullptr};

      readTransactionInProgress = true; // remember that doPreRead has been called. It might throw, so we remember
                                        // before we call it
      doPreRead(type);
    }

    /** Backend specific implementation of preRead(). preRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
   protected:
    virtual void doPreRead(TransferType) {}

   private:
    /** Helper function to catch the exceptions. Avoids code duplication. Here, the exception is actually coming
     *  through, but before setException is called. */
    void postReadAndHandleExceptions(TransferType type, bool updateDataBuffer) {
      try {
        postRead(type, updateDataBuffer);
      }
      catch(ChimeraTK::runtime_error& ex) {
        if(_exceptionBackend) {
          _exceptionBackend->setException(ex.what());
        }
        throw;
      }
    }

   public:
    /** Transfer the data from the device receive buffer into the user buffer,
     *  while converting the data into the user data format if needed.
     *
     *  Called by read() etc. Also the TransferGroup will call this function after
     *  a read was executed directly on the underlying accessor. This function must
     *  be implemented to extract the read data from the underlying accessor and
     *  expose it to the user. */
    void postRead(TransferType type, bool updateDataBuffer) {
      // only delegate to doPostRead() the first time postRead() is called in a row.
      if(readTransactionInProgress) {
        readTransactionInProgress = false;
        doPostRead(type, updateDataBuffer);
      }

      // Throw on each call of postRead(). All high-level elements for a shared low-level transfer element must see the
      // exception. Note: doPostRead can throw an exception, but in that case _activeException must be false (we can
      // only have one exception at a time). In case other code is added here later which needs to be executed after
      // doPostRead() always, a try-catch block may be necessary.
      if(_activeException) {
        // don't clear the active connection. This is done in preRead().
        std::rethrow_exception(_activeException);
      }
    }

    /** Backend specific implementation of postRead(). postRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown.
     *
     *  Notes for backend implementations:
     *  - If the flag updateDataBuffer is false, the data buffer must stay unaltered. Full implementations (backends)
     * must also leave the meta data (version number and data validity) unchanged. Decorators are allowed to change the
     * meta data (for instance set the DataValidity::faulty).
     */
   protected:
    virtual void doPostRead(TransferType, bool /*updateDataBuffer*/) {}

   private:
    /** helper functions to avoid code duplication */
    void preWriteAndHandleExceptions(TransferType type, ChimeraTK::VersionNumber versionNumber) noexcept {
      try {
        preWrite(type, versionNumber);
      }
      catch(ChimeraTK::logic_error&) {
        _activeException = std::current_exception();
      }
      catch(ChimeraTK::runtime_error&) {
        _activeException = std::current_exception();
      }
      catch(boost::thread_interrupted&) {
        _activeException = std::current_exception();
      }
    }

   public:
    /** Transfer the data from the user buffer into the device send buffer, while
     * converting the data from then user data format if needed.
     *
     *  Called by write(). Also the TransferGroup will call this function before a
     * write will be executed directly on the underlying accessor. This function
     * implemented be used to transfer the data to be written into the
     *  underlying accessor. */
    void preWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) {
      if(writeTransactionInProgress) return;

      _activeException = {};
      if(versionNumber < getVersionNumber()) {
        throw ChimeraTK::logic_error("The version number passed to write() of TransferElement '" + _name +
            "' is less than the last version number used.");
      }
      writeTransactionInProgress = true; // must not be set, if the logic_error is thrown above due to the old version
      doPreWrite(type, versionNumber);
    }

    /** Backend specific implementation of preWrite(). preWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
   protected:
    virtual void doPreWrite(TransferType, VersionNumber) {}

   private:
    /** Helper function to catch the exceptions. Avoids code duplication. Here, the exception is actually coming
     *  through, but before setException is called. */
    void postWriteAndHandleExceptions(TransferType type, VersionNumber versionNumber) {
      try {
        postWrite(type, versionNumber);
      }
      catch(ChimeraTK::runtime_error& ex) {
        if(_exceptionBackend) {
          _exceptionBackend->setException(ex.what());
        }
        throw;
      }
    }

   public:
    /** Perform any post-write clean-ups if necessary. If during preWrite() e.g.
     * the user data buffer was swapped away, it must be swapped back in this
     * function so the just sent data is available again to the calling program.
     *
     *  Called by write(). Also the TransferGroup will call this function after a
     * write was executed directly on the underlying accessor. */
    void postWrite(TransferType type, VersionNumber versionNumber) {
      if(writeTransactionInProgress) {
        writeTransactionInProgress = false;
        doPostWrite(type, versionNumber);
      }

      // Note: doPostWrite can throw an exception, but in that case hasSeenException must be false (we can only have one
      // exception at a time). In case other code is added here later which needs to be executed after doPostWrite()
      // always, a try-catch block may be necessary.
      // Another note: If writeTransactionInProgress == false, there can still be an exception, if the version number
      // used in a write was too old (see preWrite).
      if(_activeException) {
        std::rethrow_exception(_activeException);
      }

      // only after a successful write the version number is updated
      _versionNumber = versionNumber;
    }

    /** Backend specific implementation of postWrite(). postWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     *
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
   protected:
    virtual void doPostWrite(TransferType, VersionNumber) {}

   public:
    /**
     *  Write the data to the device. This function must be called after preWrite() and before postWrite(). If the
     *  return value is true, old data was lost on the write transfer (e.g. due to an buffer overflow). In case of an
     *  unbuffered write transfer, the return value will always be false.
     *
     *  This function internally calls doWriteTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doWriteTransfer() are caught and rethrown in postWrite().
     */
    bool writeTransfer(ChimeraTK::VersionNumber versionNumber) { return doWriteTransfer(versionNumber); }

   protected:
    /**
     *  Implementation version of writeTransfer(). This function must be implemented by the backend. For the
     *  functional description read the documentation of writeTransfer().
     *
     *  Implementation notes:
     *  - Decorators must delegate the call to writeTransfer() of the decorated target.
     */
    virtual bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) = 0;

   public:
    /**
     *  Write the data to the device. The implementation is allowed to destroy the content of the user buffer in the
     *  process. This is an optional optimisation, hence the behaviour might be identical to writeTransfer().
     *
     *  This function must be called after preWrite() and before postWrite(). If the
     *  return value is true, old data was lost on the write transfer (e.g. due to an buffer overflow). In case of an
     *  unbuffered write transfer, the return value will always be false.
     *
     *  This function internally calls doWriteTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doWriteTransfer() are caught and rethrown in postWrite().
     */
    bool writeTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
      return doWriteTransferDestructively(versionNumber);
    }

   protected:
    /**
     *  Implementation version of writeTransferDestructively(). This function must be implemented by the backend. For
     *  the functional description read the documentation of writeTransfer().
     *
     *  Implementation notes:
     *  - Decorators must delegate the call to writeTransfer() of the decorated target.
     *  - Delegations within the same object should go to the "do" version, e.g. to this->doWriteTransfer()
     *  - The implementation may destroy the content of the user buffer in the process. This is an optional
     *    optimisation, hence there is a default implementation which just calls the normal doWriteTransfer().
     */
    virtual bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
      return doWriteTransfer(versionNumber);
    }

   public:
    /**
     *  Check whether the TransferElement can be used in places where the
     * TransferElement "other" is currently used, e.g. to merge the two transfers.
     * This function must be used in replaceTransferElement() by implementations
     *  which use other TransferElements, to determine if a used TransferElement
     * shall be replaced with the TransferElement "other".
     *
     *  The purpose of this function is not to determine if at any point in the
     * hierarchy an replacement could be done. This function only works on a
     * single level. It is not used by the TransferGroup to determine
     *  replaceTransferElement() whether shall be used (it is always called).
     * Instead this function can be used by decorators etc. inside their
     * implementation of replaceTransferElement() to determine if they might swap
     *  their implementation(s).
     *
     *  Note for decorators and similar implementations: This function must not be
     * decorated. It should only return true if this should actually be replaced
     * with other in the call to replaceTransferElement() one level up in the
     * hierarchy. If the replacement should be done further down in the hierarchy,
     * simply return false. It should only return if other is fully identical to
     * this (i.e. behaves identical in all situations but might be another
     * instance).
     */
    virtual bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const {
      (void)other; // prevent warning
      return false;
    }

    /**
     *  Obtain the underlying TransferElements with actual hardware access. If
     * this transfer element is directly reading from / writing to the hardware,
     * it will return a list just containing a shared pointer of itself.
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    virtual std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() = 0;

    /**
     *  Obtain the full list of TransferElements internally used by this
     * TransferElement. The function is recursive, i.e. elements used by the
     * elements returned by this function are also added to the list. It is
     * guaranteed that the directly used elements are first in the list and the
     * result from recursion is appended to the list.
     *
     *  Example: A decorator would return a list with its target TransferElement
     * followed by the result of getInternalElements() called on its target
     * TransferElement.
     *
     *  If this TransferElement is not using any other element, it should return
     * an empty vector. Thus those elements which return a list just containing
     * themselves in getHardwareAccessingElements() will return an empty list here
     * in getInternalElements().
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    virtual std::list<boost::shared_ptr<TransferElement>> getInternalElements() = 0;

    /**
     *  Obtain the highest level implementation TransferElement. For
     * TransferElements which are itself an implementation this will directly
     * return a shared pointer to this. If this TransferElement is a user
     *  frontend, the pointer to the internal implementation is returned.
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    virtual boost::shared_ptr<TransferElement> getHighLevelImplElement() { return shared_from_this(); }

    /**
     *  Search for all underlying TransferElements which are considered identical
     * (see sameRegister()) with the given TransferElement. These TransferElements
     * are then replaced with the new element. If no underlying element matches
     * the new element, this function has no effect.
     */
    // FIXME #11279 Implement API breaking changes from linter warnings
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual void replaceTransferElement([[maybe_unused]] boost::shared_ptr<TransferElement> newElement) {}

    /** Create a CopyRegisterDecorator of the right type decorating this
     * TransferElement. This is used by
     *  TransferElementAbstractor::replaceTransferElement() to decouple two
     * accessors which are replaced on the abstractor level. */
    virtual boost::shared_ptr<TransferElement> makeCopyRegisterDecorator() = 0;

    /** Constant string to be used as a unit when the unit is not provided or
     * known */
    static constexpr char unitNotSet[] = "n./a.";

    /**
     *  Associate a persistent data storage object to be updated on each write
     * operation of this ProcessArray. If no persistent data storage as associated
     * previously, the value from the persistent storage is read and send to the
     * receiver.
     *
     *  Note: A call to this function will be ignored, if the TransferElement does
     * not support persistent data storage (e.g. read-only variables or device
     * registers) @todo TODO does this make sense?
     */
    // FIXME #11279 Implement API breaking changes from linter warnings
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage>) {}

    /**
     * Obtain unique ID for this TransferElement, see TransferElementID for
     * details.
     */
    TransferElementID getId() const { return _id; }

    /**
     * Return from a blocking read immediately and throw boost::thread_interrupted.
     * This function can be used to shutdown a thread waiting on data to arrive,
     * which might never happen because the sending part of the application is already shut down,
     * or there is no new data at the moment.
     *
     * This function can only be used for TransferElements with AccessMode::wait_for_new_data. Otherwise it
     * will throw a ChimeraTK::logic_error.
     *
     * Note that this function does not stop the sending thread. It just places a boost::thread_interrupted exception on
     * the _TransferElement::_readQueue, so a waiting read() has something to receive and returns. If regular data is
     * put into the queue just before the exception, this is received first. Hence it is not guaranteed that the read
     * call that is supposed to be interrupted will actually throw an exception. But it is guaranteed that it returns
     * immediately. An it is guaranteed that eventually the boost::thread_interrupted exception will be received.
     *
     * See  \ref transferElement_B_8_6 "Technical specification: TransferElement B.8.6"
     *
     * Implementation notice: This default implementation is always throwing. Each ThransferElement implementation
     * that supports AccessMode::wait_for_new_data has to override it like this:
     *   void interrupt() override { this->interrupt_impl(this->_myDataTransportQueue); }
     */
    virtual void interrupt() {
      if(!this->_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error(
            "TransferElement::interrupt() called on '" + _name + "' but AccessMode::wait_for_new_data is not set.");
      }
      throw ChimeraTK::logic_error("TransferElement::interrupt() must be overridden by all implementations with "
                                   "AccessMode::wait_for_new_data. (TransferElement '" +
          _name + "')");
    }

    /** Implementation of interrupt() for TransferElements which support AccessMode::wait_for_new_data */
    template<typename QUEUE_TYPE>
    void interrupt_impl(QUEUE_TYPE& dataTransportQueue) {
      if(!this->_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        throw ChimeraTK::logic_error(
            "TransferElement::interrupt() called on '" + _name + "' but AccessMode::wait_for_new_data is not set.");
      }

      dataTransportQueue.push_overwrite_exception(std::make_exception_ptr(boost::thread_interrupted()));
    }

    /** Check whether a read transaction is in progress, i.e. preRead() has been called but not yet postRead(). */
    bool isReadTransactionInProgress() const { return readTransactionInProgress; }

    /** Check whether a write transaction is in progress, i.e. preWrite() has been called but not yet postWrite(). */
    bool isWriteTransactionInProgress() const { return writeTransactionInProgress; }

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
    void makeUniqueId() { _id.makeUnique(); }

    /** Flag whether this TransferElement has been added to a TransferGroup or not
     */
    bool _isInTransferGroup{false};

    /** The access mode flags for this transfer element.*/
    AccessModeFlags _accessModeFlags;

    friend class TransferGroup;
    friend class ReadAnyGroup;

   private:
    /** Flag whether a read transaction is in progress. This flag will be set in
     * preRead() and cleared in postRead() and is used to prevent multiple calls
     * to these functions during a single transfer. It should also be reset before
     * starting a new read transaction - this happens only inside the
     * implementation of read() etc. and in the TransferGroup. */
    bool readTransactionInProgress{false};

    /** Flag whether a write transaction is in progress. This flag is similar to
     *  readTransactionInProgress but affects preWrite() and postWrite(). */
    bool writeTransactionInProgress{false};

   protected:
    /// The queue for asynchronous read transfers. This is the void queue which is a continuation of the actual data
    /// transport queue, which is implementation dependent. With _readQueue the exception propagation and waiting for
    /// new data is implemented in TransferElement.
    cppext::future_queue<void> _readQueue;

    /// The version number of the last successful transfer. Part of the application buffer \ref transferElement_A_5
    /// "(see TransferElement specification A.5)"
    VersionNumber _versionNumber{nullptr};

    /// The validity of the data in the application buffer. Part of the application buffer \ref transferElement_A_5
    /// "(see TransferElement specification A.5)"
    DataValidity _dataValidity{DataValidity::ok};

    /// Exception to be rethrown in postXXX() in case hasSeenException == true
    /// Can be set via setActiveException().
    std::exception_ptr _activeException{nullptr};
  }; // namespace ChimeraTK

} /* namespace ChimeraTK */
namespace std {

  /*******************************************************************************************************************/

  /** Hash function for putting TransferElementID e.g. into an std::unordered_map
   */
  template<>
  struct hash<ChimeraTK::TransferElementID> {
    std::size_t operator()(const ChimeraTK::TransferElementID& f) const { return std::hash<size_t>{}(f._id); }
  };

  /*******************************************************************************************************************/

  /** Comparison for putting TransferElementID e.g. into an std::map */
  template<>
  struct less<ChimeraTK::TransferElementID> {
    bool operator()(const ChimeraTK::TransferElementID& a, const ChimeraTK::TransferElementID& b) const {
      return a._id < b._id;
    }
  };
} // namespace std
