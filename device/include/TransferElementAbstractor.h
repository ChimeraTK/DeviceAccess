/*
 * TransferElementAbstractor.h
 *
 *  Created on: Dec 18, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_TRANSFER_ELEMENT_ABSTRACTOR_H
#define CHIMERA_TK_TRANSFER_ELEMENT_ABSTRACTOR_H

#include <functional>
#include <list>
#include <string>
#include <typeinfo>
#include <vector>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include "Exception.h"
#include "TransferElement.h"
#include "TransferElementID.h"
#include "TransferFuture.h"
#include "VersionNumber.h"

namespace ChimeraTK {
  class PersistentDataStorage;
}

namespace ChimeraTK {

  class TransferGroup;

  using ChimeraTK::TransferFuture;

  /*******************************************************************************************************************/

  /** Base class for register accessors abstractors independent of the UserType */
  class TransferElementAbstractor {
   public:
    /** Construct from TransferElement implementation */
    explicit TransferElementAbstractor(boost::shared_ptr<TransferElement> impl) : _impl(impl) {}

    /** Create an uninitialised abstractor - just for late initialisation */
    TransferElementAbstractor() {}

    /** Returns the name that identifies the process variable. */
    const std::string& getName() const { return _impl->getName(); }

    /** Returns the engineering unit. If none was specified, it will default to
     * "n./a." */
    const std::string& getUnit() const { return _impl->getUnit(); }

    /** Returns the description of this variable/register */
    const std::string& getDescription() const { return _impl->getDescription(); }

    /** Returns the \c std::type_info for the value type of this transfer element.
     *  This can be used to determine the type at runtime. */
    const std::type_info& getValueType() const { return _impl->getValueType(); }

    /** Return the AccessModeFlags for this TransferElement. */
    AccessModeFlags getAccessModeFlags() const { return _impl->getAccessModeFlags(); }

    /** Read the data from the device. If AccessMode::wait_for_new_data was set,
     * this function will block until new data has arrived. Otherwise it still
     * might block for a short time until the data transfer was complete. */
    void read() { _impl->read(); }

    /** Read the next value, if available in the input buffer.
     *
     *  If AccessMode::wait_for_new_data was set, this function returns
     * immediately and the return value indicated if a new value was available
     * (<code>true</code>) or not (<code>false</code>).
     *
     *  If AccessMode::wait_for_new_data was not set, this function is identical
     * to read(), which will still return quickly. Depending on the actual
     * transfer implementation, the backend might need to transfer data to obtain
     *  the current value before returning. Also this function is not guaranteed
     * to be lock free. The return value will be always true in this mode. */
    bool readNonBlocking() { return _impl->readNonBlocking(); }

    /** Read the latest value, discarding any other update since the last read if
     * present. Otherwise this function is identical to readNonBlocking(), i.e. it
     * will never wait for new values and it will return whether a new value was
     * available if AccessMode::wait_for_new_data is set. */
    bool readLatest() { return _impl->readLatest(); }

    /** Read data from the device in the background and return a future which will
     * be fulfilled when the data is ready. When the future is fulfilled, the
     * transfer element will already contain the new data, there is no need to
     * call read() or readNonBlocking() (which would trigger another data
     * transfer).
     *
     *  It is allowed to call this function multiple times, which will return the
     * same (shared) future until it is fulfilled. If other read functions (like
     * read() or readNonBlocking()) are called before the future previously
     * returned by this function was fulfilled, that call will be equivalent to
     * the respective call on the future (i.e. TransferFuture::wait() resp.
     * TransferFuture::hasNewData()) and thus the future will hae been used
     * afterwards.
     *
     *  The future will be fulfilled at the time when normally read() would
     * return. A call to this function is roughly logically equivalent to:
     *    boost::async( boost::bind(&TransferElement::read, this) );
     *  (Although such implementation would disallow accessing the user data
     * buffer until the future is fulfilled, which is not the case for this
     * function.)
     *
     *  Design note: A special type of future has to be returned to allow an
     * abstraction from the implementation details of the backend. This allows -
     * depending on the backend type - a more efficient implementation without
     * launching a thread.
     *
     *  Note for implementations: Inside this function and before launching the
     * actual transfer, the flag readTransactionInProgress must be cleared, then
     * preRead() has to be called. Otherwise postRead() will not get executed
     * after the transfer. postRead() on the other hand must not be called inside
     * this function, since this would update the user buffer, which should only
     * happen when waiting on the TransferFuture. TransferFuture::wait() will
     * automatically call postRead() before returning. Decorators must also call
     *  preRead() in their implementations of readAsync()!
     *
     *  Note: This feature is still experimental. Expect API changes without
     * notice! */
    TransferFuture readAsync() { return _impl->readAsync(); }

    /**
     * Returns the version number that is associated with the last transfer (i.e.
     * last read or write). See ChimeraTK::VersionNumber for details.
     */
    ChimeraTK::VersionNumber getVersionNumber() const { return _impl->getVersionNumber(); }

    /** Write the data to device. The return value is true, old data was lost on
     * the write transfer (e.g. due to an buffer overflow). In case of an
     * unbuffered write transfer, the return value will always be false. */
    bool write(ChimeraTK::VersionNumber versionNumber = {}) { return _impl->write(versionNumber); }

    /** Just like write(), but allows the implementation to destroy the content of the user buffer in the
     *  process. This is an optional optimisation, hence there is a default implementation which just calls the normal
     *  doWriteTransfer(). In any case, the application must expect the user buffer of the TransferElement to contain
     *  undefined data after calling this function. */
    bool writeDestructively(ChimeraTK::VersionNumber versionNumber = {}) {
      return _impl->writeDestructively(versionNumber);
    }

    /** Check if transfer element is read only, i\.e\. it is readable but not
     * writeable. */
    bool isReadOnly() const { return _impl->isReadOnly(); }

    /** Check if transfer element is readable. It throws an acception if you try
     * to read and isReadable() is not true.*/
    bool isReadable() const { return _impl->isReadable(); }

    /** Check if transfer element is writeable. It throws an acception if you try
     * to write and isWriteable() is not true.*/
    bool isWriteable() const { return _impl->isWriteable(); }

    /**
     *  Obtain the underlying TransferElements with actual hardware access. If
     * this transfer element is directly reading from / writing to the hardware,
     * it will return a list just containing a shared pointer of itself.
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() {
      return _impl->getHardwareAccessingElements();
    }

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
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() {
      auto result = _impl->getInternalElements();
      result.push_front(_impl);
      return result;
    }

    /**
     *  Obtain the highest level implementation TransferElement. For
     * TransferElements which are itself an implementation this will directly
     * return a shared pointer to this. If this TransferElement is a user
     *  frontend, the pointer to the internal implementation is returned.
     *
     *  Note: Avoid using this in application code, since it will break the
     * abstraction!
     */
    boost::shared_ptr<TransferElement> getHighLevelImplElement() { return _impl; }

    /** Return if the accessor is properly initialised. It is initialised if it
     * was constructed passing the pointer to an implementation (a
     * NDRegisterAccessor), it is not initialised if it was constructed only using
     * the placeholder constructor without arguments. */
    bool isInitialised() const { return _impl != nullptr; }

    /** Assign a new accessor to this TransferElementAbstractor. Since another
     * TransferElementAbstractor is passed as argument, both
     * TransferElementAbstractor will then point to the same accessor and thus are
     * sharing the same buffer. To obtain a new copy of the accessor with a
     * distinct buffer, the corresponding getXXRegisterAccessor() function of
     * Device must be called. */
    void replace(const TransferElementAbstractor& newAccessor) { _impl = newAccessor._impl; }

    /** Alternative signature of relace() with the same functionality, used when a
     * pointer to the implementation has been obtained directly (instead of a
     * TransferElementAbstractor). */
    void replace(boost::shared_ptr<TransferElement> newImpl) { _impl = newImpl; }

    /**
     *  Search for all underlying TransferElements which are considered identicel
     * (see mayReplaceOther()) with the given TransferElement. These
     * TransferElements are then replaced with the new element. If no underlying
     *  element matches the new element, this function has no effect.
     */
    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
      if(newElement->mayReplaceOther(_impl)) {
        if(newElement != _impl) {
          _impl = newElement->makeCopyRegisterDecorator();
        }
      }
      else {
        _impl->replaceTransferElement(newElement);
      }
    }

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
    void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) {
      _impl->setPersistentDataStorage(storage);
    }

    /**
     * Obtain unique ID for the actual implementation of this TransferElement.
     * This means that e.g. two instances of ScalarRegisterAccessor created by the
     * same call to Device::getScalarRegisterAccessor() (e.g. by copying the
     * accessor to another using NDRegisterAccessorBridge::replace()) will have
     * the same ID, while two instances obtained by to difference calls to
     * Device::getScalarRegisterAccessor() will have a different ID even when
     * accessing the very same register.
     */
    TransferElementID getId() const { return _impl->getId(); }

    /** Function called by the TransferFuture before entering a potentially
     * blocking wait(). In contrast to a wait callback of a boost::future/promise,
     * this function is not called when just checking whether the result is ready
     * or not. Usually it is not necessary to implement this function, but
     * decorators should pass it on. One use case is the ApplicationCore
     * TestDecoratorRegisterAccessor, which needs to be informed before blocking
     *  the thread execution.
     *  Note: The ReadAnyGroup will trigger a call to this function of the first
     * TransferElement with AccessMode::wait_for_new_data in the group before
     * potentially blocking. */
    void transferFutureWaitCallback() { _impl->transferFutureWaitCallback(); }

    /** Set the current DataValidity for this TransferElement. Will do nothing if the
     * backend does not support it */
    void setDataValidity(DataValidity valid = DataValidity::ok) { _impl->setDataValidity(valid); }

    /** Return current validity of the data. Will always return DataValidity::ok if the
     * backend does not support it */
    DataValidity dataValidity() const { return _impl->dataValidity(); }

   protected:
    /** Untyped pointer to implementation */
    boost::shared_ptr<TransferElement> _impl;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_TRANSFER_ELEMENT_BRIDGE_H */
