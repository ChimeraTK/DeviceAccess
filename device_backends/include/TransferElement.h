/*
 * TransferElement.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TRANSFER_ELEMENT_H
#define CHIMERA_TK_TRANSFER_ELEMENT_H

#include <functional>
#include <list>
#include <string>
#include <typeinfo>
#include <vector>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include <ChimeraTK/cppext/future_queue.hpp>

#include "AccessMode.h"
#include "Exception.h"
#include "TransferElementID.h"
#include "VersionNumber.h"

namespace ChimeraTK {
  class PersistentDataStorage;
}

namespace ChimeraTK {

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

  /**
   * @brief Used to indicate the applicable opereation on a Transferelement.
   */
  enum class TransferType { read, readNonBlocking, readLatest, write, writeDestructively };

  namespace detail {
    /**
     *  Exception to be thrown by continuations of the _readQueue when a value shall be discarded. This is needed to avoid
     * notifications of the application if a value should never reach the
     * application. The exception is caught in readTransferAyncWaitingImpl() and
     * readTransferAyncNonWaitingImpl() and should never be visible to the application.
     */
    class DiscardValueException {};
  } /* namespace detail */

  /*******************************************************************************************************************/

  /** Base class for register accessors which can be part of a TransferGroup */
  class TransferElement : public boost::enable_shared_from_this<TransferElement> {
   public:
    /** Creates a transfer element with the specified name. */
    TransferElement(std::string const& name, AccessModeFlags accessModeFlags,
        std::string const& unit = std::string(unitNotSet), std::string const& description = std::string())
    : _name(name), _unit(unit), _description(description), _isInTransferGroup(false),
      _accessModeFlags(accessModeFlags) {}

    /** Copying and moving is not allowed */
    TransferElement(const TransferElement& other) = delete;
    TransferElement(TransferElement&& other) = delete;
    TransferElement& operator=(const TransferElement& other) = delete;
    TransferElement& operator=(TransferElement&& other) = delete;

    /** Abstract base classes need a virtual destructor. */
    virtual ~TransferElement();

    /** A typedef for more compact syntax */
    typedef boost::shared_ptr<TransferElement> SharedPtr;

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
    void setDataValidity(DataValidity validity = DataValidity::ok) {
      assert(isWriteable());
      _dataValidity = validity;
    }

    /** Return current validity of the data. Will always return DataValidity::ok if the
     * backend does not support it */
    DataValidity dataValidity() const { return _dataValidity; }

    /** Read the data from the device. If AccessMode::wait_for_new_data was set,
     * this function will block until new data has arrived. Otherwise it still
     * might block for a short time until the data transfer was complete. */
    void read() {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on an accessor "
                                     "which is part of a TransferGroup "
                                     "is not allowed.");
      }
      this->readTransactionInProgress = false;
      preRead(TransferType::read);
      if(!_hasSeenException) readTransfer();
      postRead(TransferType::read, !_hasSeenException);
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
      this->readTransactionInProgress = false;
      preRead(TransferType::readNonBlocking);
      bool hasNewData = false;
      if(!_hasSeenException) {
        hasNewData = readTransferNonBlocking();
      }
      postRead(TransferType::readNonBlocking, hasNewData);
      return hasNewData;
    }

    /** Read the latest value, discarding any other update since the last read if
     * present. Otherwise this function is identical to readNonBlocking(), i.e. it
     * will never wait for new values and it will return whether a new value was
     * available if AccessMode::wait_for_new_data is set. */
    bool readLatest() {
      bool hasNewData = false;
      // Call readNonBlocking until there is no new data to be read any more
      while(readNonBlocking()) {
        // remember whether we have new data
        hasNewData = true;
      }
      return hasNewData;
    }

    /** Write the data to device. The return value is true, old data was lost on
     * the write transfer (e.g. due to an buffer overflow). In case of an
     * unbuffered write transfer, the return value will always be false. */
    bool write(ChimeraTK::VersionNumber versionNumber = {}) {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on an accessor "
                                     "which is part of a TransferGroup "
                                     "is not allowed.");
      }
      this->writeTransactionInProgress = false;
      preWrite(TransferType::write, versionNumber);
      bool previousDataLost =
          true; // the value here does not matter. If there was an exception, it will be re-thrown in postWrite, so it is never returned
      if(!_hasSeenException) {
        previousDataLost = writeTransfer(versionNumber);
      }
      postWrite(TransferType::write, versionNumber);
      return previousDataLost;
    }

    /** Just like write(), but allows the implementation to destroy the content of the user buffer in the
     *  process. This is an optional optimisation, hence there is a default implementation which just calls the normal
     *  doWriteTransfer(). In any case, the application must expect the user buffer of the TransferElement to contain
     *  undefined data after calling this function. */
    bool writeDestructively(ChimeraTK::VersionNumber versionNumber = {}) {
      if(TransferElement::_isInTransferGroup) {
        throw ChimeraTK::logic_error("Calling read() or write() on an accessor "
                                     "which is part of a TransferGroup "
                                     "is not allowed.");
      }
      this->writeTransactionInProgress = false;
      preWrite(TransferType::writeDestructively, versionNumber);
      bool previousDataLost =
          true; // the value here does not matter. If there was an exception, it will be re-thrown in postWrite, so it is never returned
      if(!_hasSeenException) {
        previousDataLost = writeTransferDestructively(versionNumber);
      }
      postWrite(TransferType::writeDestructively, versionNumber);
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

    /** Check if transfer element is readable. It throws an acception if you try
     * to read and isReadable() is not true.*/
    virtual bool isReadable() const = 0;

    /** Check if transfer element is writeable. It throws an acception if you try
     * to write and isWriteable() is not true.*/
    virtual bool isWriteable() const = 0;

   private:
    /**
     *  Helper for exception handling in the transfer functions, to avoid code duplication.
     */
    template<typename ReturnType, typename Callable>
    ReturnType handleTransferException(Callable function, ReturnType returnOnException) {
      try {
        return function();
      }
      catch(ChimeraTK::runtime_error&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
        return returnOnException;
      }
      catch(boost::thread_interrupted&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
        return returnOnException;
      }
      catch(...) {
        std::cout << "BUG: Wrong exception type thrown in transfer function!" << std::endl;
        std::terminate();
      }
    }

   private:
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
     *  This function internally calles doReadTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doReadTransfer() are caught and rethrown in postRead().
     */
    void readTransfer() noexcept {
      handleTransferException<bool>(
          [this] {
            if(_accessModeFlags.has(AccessMode::wait_for_new_data)) {
              readTransferAsyncWaitingImpl();
            }
            else {
              doReadTransferSynchronously();
            }
            return true; // need to return something, is ignored later
          },
          false);
    }

   protected:
    /**
     *  Implementation version of readTransfer(). This function must be implemented by the backend. For the
     *  functional description read the documentation of readTransfer().
     *  
     *  Implementation notes:
     *  - This function must return within ~1 second after boost::thread::interrupt() has been called on the thread
     *    calling this function.
     *  - Decorators must delegate the call to readTransfer() of the decorated target.
     *  - Delegations within the same object should go to the "do" version, e.g. to this->doReadTransferLatest()
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
     *  This function internally calles doReadTransferNonBlocking(), which is implemented by the backend. runtime_error
     *  exceptions thrown in doRedoReadTransferNonBlockingadTransfer() are caught and rethrown in postRead().
     */
    bool readTransferNonBlocking() noexcept {
      return handleTransferException<bool>(
          [this]() {
            if(_accessModeFlags.has(AccessMode::wait_for_new_data)) {
              return readTransferAsyncNonWaitingImpl();
            }
            else {
              doReadTransferSynchronously();
              return true;
            }
          },
          false);
    }

   public:
    /** Perform any pre-read tasks if necessary.
     *
     *  Called by read() etc. Also the TransferGroup will call this function before a read is executed directly on the
     *  underlying accessor. */
    void preRead(TransferType type) noexcept {
      if(readTransactionInProgress) return;
      assert(!_hasSeenException);
      try {
        doPreRead(type);
      }
      catch(ChimeraTK::logic_error&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(ChimeraTK::runtime_error&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(boost::thread_interrupted&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(...) {
        std::cout << "BUG: Wrong exception type thrown in doPreRead()!" << std::endl;
        std::terminate();
      }
      readTransactionInProgress = true;
    }

    /** Backend specific implementation of preRead(). preRead() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     * 
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
   protected:
    virtual void doPreRead(TransferType) {}

   public:
    /** Transfer the data from the device receive buffer into the user buffer,
     *  while converting the data into the user data format if needed.
     *
     *  Called by read() etc. Also the TransferGroup will call this function after
     *  a read was executed directly on the underlying accessor. This function must
     *  be implemented to extract the read data from the underlying accessor and
     *  expose it to the user. */
    void postRead(TransferType type, bool hasNewData) {
      if(!readTransactionInProgress) return;
      readTransactionInProgress = false;
      doPostRead(type, hasNewData);
      // Note: doPostRead can throw an exception, but in that case hasSeenException must be false (we can only have one
      // exception at a time). In case other code is added here later which needs to be executed after doPostRead()
      // always, a try-catch block may be necessary.
      if(_hasSeenException) {
        _hasSeenException = false;
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
     *  - If the flag hasNewData is false, the user buffer must stay unaltered. This is true also if the backend did
     *    not throw any exception and did not return false in the doReadTransfer...() call. The flag is also controlled
     *    by the TransferGroup and will be set to false, if an exception has been seen on any other transfer. */
   protected:
    virtual void doPostRead(TransferType, bool /*hasNewData*/) {}

   public:
    /** Transfer the data from the user buffer into the device send buffer, while
     * converting the data from then user data format if needed.
     *
     *  Called by write(). Also the TransferGroup will call this function before a
     * write will be executed directly on the underlying accessor. This function
     * implemented be used to transfer the data to be written into the
     *  underlying accessor. */
    void preWrite(TransferType type, ChimeraTK::VersionNumber versionNumber) noexcept {
      if(writeTransactionInProgress) return;
      assert(!_hasSeenException);
      try {
        if(versionNumber < getVersionNumber()) {
          throw ChimeraTK::logic_error(
              "The version number passed to write() is less than the last version number used.");
        }
        writeTransactionInProgress = true; // must not be set, if the logic_error is thrown above due to the old version
        doPreWrite(type, versionNumber);
      }
      catch(ChimeraTK::logic_error&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(ChimeraTK::runtime_error&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(boost::thread_interrupted&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      // Needed for throwing TypeChangingDecorator. Is this even a good concept?
      catch(boost::numeric::bad_numeric_cast&) {
        _hasSeenException = true;
        _activeException = std::current_exception();
      }
      catch(...) {
        std::cout << "BUG: Wrong exception type thrown in doPreWrite()!" << std::endl;
        std::terminate();
      }
    }

    /** Backend specific implementation of preWrite(). preWrite() will call this function, but it will make sure that
     *  it gets called only once per transfer.
     * 
     *  No actual communication may be done. Hence, no runtime_error exception may be thrown by this function. Also it
     *  must be acceptable to call this function while the device is closed or not functional (see isFunctional()) and
     *  no exception is thrown. */
   protected:
    virtual void doPreWrite(TransferType, VersionNumber) {}

   public:
    /** Perform any post-write cleanups if necessary. If during preWrite() e.g.
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
      if(_hasSeenException) {
        _hasSeenException = false;
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
     *  This function internally calles doWriteTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doWriteTransfer() are caught and rethrown in postWrite().
     */
    bool writeTransfer(ChimeraTK::VersionNumber versionNumber) noexcept {
      return handleTransferException<bool>([&] { return doWriteTransfer(versionNumber); }, true);
    }

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
     *  This function internally calles doWriteTransfer(), which is implemented by the backend. runtime_error exceptions
     *  thrown in doWriteTransfer() are caught and rethrown in postWrite().
     */
    bool writeTransferDestructively(ChimeraTK::VersionNumber versionNumber) noexcept {
      return handleTransferException<bool>([&] { return doWriteTransferDestructively(versionNumber); }, true);
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
     *  Search for all underlying TransferElements which are considered identicel
     * (see sameRegister()) with the given TransferElement. These TransferElements
     * are then replaced with the new element. If no underlying element matches
     * the new element, this function has no effect.
     */
    virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
      (void)newElement; // prevent warning
    }

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
    virtual void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage>) {}

    /**
     * Obtain unique ID for this TransferElement, see TransferElementID for
     * details.
     */
    TransferElementID getId() const { return _id; }

    /**
     *  Cancel any pending transfer and throw boost::thread_interrupted in its
     * postRead. This function can be used to shutdown a thread waiting on the
     * transfer to complete (which might never happen due to the sending part of
     *  the application already shut down).
     *
     *  Note: there is no guarantee that the exception is actually thrown if no
     * transfer is currently running or if the transfer completes while this
     * function is called. Also for transfer elements which never block for an
     *  extended period of time this function may just do nothing. To really shut
     * down the receiving thread, a interrupt request should be sent to the thread
     * and boost::this_thread::interruption_point() shall be called after each
     * transfer.
     */
    virtual void interrupt() {}

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
    bool _isInTransferGroup;

    /** The access mode flags for this transfer element.*/
    AccessModeFlags _accessModeFlags;

    friend class TransferGroup;
    friend class TransferFuture;

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
    /// The queue for asyncronous read transfers. This is the void queue which is a continuation of the actial data transport queue,
    /// which is implementation dependent. With _readQueue the exception propagation and waiting for new data is implemented in TransferElement.
    cppext::future_queue<void> _readQueue;

    /// The version number of the last successful transfer. Part of the application buffer \ref transferElement_A_5 "(see TransferElement specification A.5)"
    VersionNumber _versionNumber{nullptr};

    /// The validity of the data in the application buffer. Part of the application buffer \ref transferElement_A_5 "(see TransferElement specification A.5)"
    DataValidity _dataValidity{DataValidity::ok};

    /// Flag whether doPreXxx() or doXXXTransferYYY() has thrown an exception (which should be rethrown in postXXX()).
    bool _hasSeenException{false};

    /// Exception to be rethrown in postXXX() in case hasSeenException == true
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
    // these typedefs are mandatory before C++17, even though they seem to be
    // unused by gcc
    typedef bool result_type;
    typedef ChimeraTK::TransferElementID first_argument_type;
    typedef ChimeraTK::TransferElementID second_argument_type;
    bool operator()(const ChimeraTK::TransferElementID& a, const ChimeraTK::TransferElementID& b) const {
      return a._id < b._id;
    }
  };
} // namespace std
#endif /* CHIMERA_TK_TRANSFER_ELEMENT_H */
