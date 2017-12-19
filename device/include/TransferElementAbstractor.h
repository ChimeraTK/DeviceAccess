/*
 * TransferElementAbstractor.h
 *
 *  Created on: Dec 18, 2017
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TRANSFER_ELEMENT_ABSTRACTOR_H
#define MTCA4U_TRANSFER_ELEMENT_ABSTRACTOR_H

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
#include "TransferElement.h"
#include "TransferElementID.h"

namespace ChimeraTK {
  class PersistentDataStorage;
}

namespace mtca4u {

  class TransferGroup;

  using ChimeraTK::TransferFuture;

  /*******************************************************************************************************************/

  /** Base class for register accessors abstractors independent of the UserType */
  class TransferElementAbstractor {

    public:

      /** Create an uninitialised abstractor - just for late initialisation */
      TransferElementAbstractor() {}

      /** Abstract base classes need a virtual destructor. */
      virtual ~TransferElementAbstractor() {}

      /** Returns the name that identifies the process variable. */
      const std::string& getName() const { return _implUntyped->getName(); }

      /** Returns the engineering unit. If none was specified, it will default to "n./a." */
      const std::string& getUnit() const { return _implUntyped->getUnit(); }

      /** Returns the description of this variable/register */
      const std::string& getDescription() const { return _implUntyped->getDescription(); }

      /** Returns the \c std::type_info for the value type of this transfer element.
       *  This can be used to determine the type at runtime. */
      const std::type_info& getValueType() const { return _implUntyped->getValueType(); }

      /** Read the data from the device. If AccessMode::wait_for_new_data was set, this function will block until new
       *  data has arrived. Otherwise it still might block for a short time until the data transfer was complete. */
      void read() { _implUntyped->read(); }

      /** Read the next value, if available in the input buffer.
       *
       *  If AccessMode::wait_for_new_data was set, this function returns immediately and the return value indicated
       *  if a new value was available (<code>true</code>) or not (<code>false</code>).
       *
       *  If AccessMode::wait_for_new_data was not set, this function is identical to read(), which will still return
       *  quickly. Depending on the actual transfer implementation, the backend might need to transfer data to obtain
       *  the current value before returning. Also this function is not guaranteed to be lock free. The return value
       *  will be always true in this mode. */
      bool readNonBlocking() { return _implUntyped->readNonBlocking(); }

      /** Read the latest value, discarding any other update since the last read if present. Otherwise this function
       *  is identical to readNonBlocking(), i.e. it will never wait for new values and it will return whether a
       *  new value was available if AccessMode::wait_for_new_data is set. */
      bool readLatest() { return _implUntyped->readLatest(); }

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
       *  Note for implementations: Inside this function and before launching the actual transfer, the flag
       *  readTransactionInProgress must be cleared, then preRead() has to be called. Otherwise postRead() will not
       *  get executed after the transfer. postRead() on the other hand must not be called inside this function,
       *  since this would update the user buffer, which should only happen when waiting on the TransferFuture.
       *  TransferFuture::wait() will automatically call postRead() before returning. Decorators must also call
       *  preRead() in their implementations of readAsync()!
       *
       *  Note: This feature is still experimental. Expect API changes without notice! */
      TransferFuture readAsync() { return _implUntyped->readAsync(); }

      /**
      * Returns the version number that is associated with the last transfer (i.e. last read or write). See
      * ChimeraTK::VersionNumber for details.
      */
      ChimeraTK::VersionNumber getVersionNumber() const { return _implUntyped->getVersionNumber(); }

      /** Write the data to device. The return value is true, old data was lost on the write transfer (e.g. due to an
       *  buffer overflow). In case of an unbuffered write transfer, the return value will always be false. */
      bool write(ChimeraTK::VersionNumber versionNumber={}) { return _implUntyped->write(versionNumber); }

      /** Check if transfer element is read only, i\.e\. it is readable but not writeable. */
      bool isReadOnly() const { return _implUntyped->isReadOnly(); }

      /** Check if transfer element is readable. It throws an acception if you try to read and
       *  isReadable() is not true.*/
      bool isReadable() const { return _implUntyped->isReadable(); }

      /** Check if transfer element is writeable. It throws an acception if you try to write and
       *  isWriteable() is not true.*/
      bool isWriteable() const { return _implUntyped->isWriteable(); }

      /**
       *  Obtain the underlying TransferElements with actual hardware access. If this transfer element
       *  is directly reading from / writing to the hardware, it will return a list just containing
       *  a shared pointer of itself.
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _implUntyped->getHardwareAccessingElements();
      }

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
      std::list< boost::shared_ptr<TransferElement> > getInternalElements() {
        auto result = _implUntyped->getInternalElements();
        result.push_front(_implUntyped);
        return result;
      }

      /**
       *  Obtain the highest level implementation TransferElement. For TransferElements which are itself an
       *  implementation this will directly return a shared pointer to this. If this TransferElement is a user
       *  frontend, the pointer to the internal implementation is returned.
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      boost::shared_ptr<TransferElement> getHighLevelImplElement() { return _implUntyped; }

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      bool isInitialised() const {
        return _implUntyped != nullptr;
      }

      /**
       *  Search for all underlying TransferElements which are considered identicel (see sameRegister()) with
       *  the given TransferElement. These TransferElements are then replaced with the new element. If no underlying
       *  element matches the new element, this function has no effect.
       */
      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) = 0;

      /**
      *  Associate a persistent data storage object to be updated on each write operation of this ProcessArray. If no
      *  persistent data storage as associated previously, the value from the persistent storage is read and send to
      *  the receiver.
      *
      *  Note: A call to this function will be ignored, if the TransferElement does not support persistent data
      *  storage (e.g. read-only variables or device registers) @todo TODO does this make sense?
      */
      void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) {
        _implUntyped->setPersistentDataStorage(storage);
      };

      /**
       * Obtain unique ID for this TransferElement. If this TransferElement is the abstractor side of the bridge, this
       * function will return the unique ID of the actual implementation. This means that e.g. two instances of
       * ScalarRegisterAccessor created by the same call to Device::getScalarRegisterAccessor() (e.g. by copying the
       * accessor to another using NDRegisterAccessorBridge::replace()) will have the same ID, while two instances
       * obtained by to difference calls to Device::getScalarRegisterAccessor() will have a different ID even when
       * accessing the very same register.
       */
      TransferElementID getId() const { return _implUntyped->getId(); };

    protected:

      /** Construct from TransferElement implementation */
      explicit TransferElementAbstractor(boost::shared_ptr< TransferElement > impl)
      : _implUntyped(impl)
      {}

      /** Untyped pointer to implementation */
      boost::shared_ptr< TransferElement > _implUntyped;
  };

  /*******************************************************************************************************************/

  // Note: the %iterator in the third line prevents doxygen from creating a link which it cannot resolve.
  // It reads: std::list<TransferFuture::PlainFutureType>::iterator
  /** Internal class needed for TransferElement::readAny(): Provide a wrapper around the list iterator to effectivly
   *  allow passing std::list<TransferFuture>::iterator to boost::wait_for_any() which otherwise expects e.g.
   *  std::list<TransferFuture::PlainFutureType>::%iterator. This class provides the same interface as an interator of
   *  the expected type but adds the function getTransferFuture(), so we can obtain the TransferElement from the
   *  return value of boost::wait_for_any(). */
  class TransferFutureIterator {
    public:
      TransferFutureIterator(const std::list<TransferFuture>::iterator &it) : _it(it) {}
      TransferFutureIterator operator++() { TransferFutureIterator ret(_it); ++_it; return ret; }
      TransferFutureIterator operator++(int) { ++_it; return *this; }
      bool operator!=(const TransferFutureIterator &rhs) {return _it != rhs._it;}
      bool operator==(const TransferFutureIterator &rhs) {return _it == rhs._it;}
      TransferFuture::PlainFutureType& operator*() {return _it->getBoostFuture();}
      TransferFuture::PlainFutureType& operator->() {return _it->getBoostFuture();}
      TransferFuture& getTransferFuture() const {return *_it;}
    private:
      std::list<TransferFuture>::iterator _it;
  };

} /* namespace mtca4u */
namespace std {
  template<>
  struct iterator_traits<mtca4u::TransferFutureIterator> {
      typedef ChimeraTK::TransferFuture::PlainFutureType value_type;
      typedef size_t difference_type;
      typedef std::forward_iterator_tag iterator_category;
  };
} /* namespace std */
namespace mtca4u {

  /*******************************************************************************************************************/

  /** Read data asynchronously from all given TransferElements and wait until one of the TransferElements has
    *  new data. The ID of the TransferElement which received new data is returned as a reference. In case multiple
    *  TransferElements receive new data simultaneously (or already have new data available before the call to
    *  readAny()), the ID of the TransferElement with the oldest VersionNumber (see getVersionNumber()) will be returned
    *  by this function. This ensures that data is received in the order of sending (unless data is "dated back"
    *  and sent with an older version number, which might be the case e.g. when using the ControlSystemAdapter).
    *
    *  Note that the behaviour is undefined when putting the same TransferElement into the list - a result might
    *  be e.g. that it blocks for ever. This is due to a limitation in the underlying boost::wait_for_any()
    *  function. */
  inline TransferElementID readAny(std::list<std::reference_wrapper<TransferElementAbstractor>> elementsToRead) {

    // build list of TransferFutures for all elements. Since readAsync() is a virtual call and we need to visit all
    // elements at least twice (once in wait_for_any and a second time for sorting by version number), this is assumed
    // to be less expensive than calling readAsync() on the fly in the TransferFutureIterator instead.
    std::list<TransferFuture> futureList;
    for(auto &elem : elementsToRead) {
      futureList.push_back(elem.get().readAsync());
    }
    // wait until any future is ready
    auto iter = boost::wait_for_any(TransferFutureIterator(futureList.begin()), TransferFutureIterator(futureList.end()));

    // Find the variable which has the oldest version number (to guarantee the order of updates).
    // Start with assuming that the future returned by boost::wait_for_any() has the oldes version number.
    TransferFuture theUpdate = iter.getTransferFuture();
    for(auto future : futureList) {
      // skip if this is the future returned by boost::wait_for_any()
      if(future == theUpdate) continue;
      // also skip if the future is not yet ready
      if(!future.hasNewData()) continue;
      // compare  version number with the version number of the stored future
      if(future.getBoostFuture().get()->_versionNumber < theUpdate.getBoostFuture().get()->_versionNumber) {
        theUpdate = future;
      }
    }

    // complete the transfer (i.e. run postRead())
    theUpdate.wait();

    // return the transfer element as a shared pointer
    return theUpdate.getTransferElementID();

  }

  /*******************************************************************************************************************/

  inline TransferElementID readAny(std::list<std::reference_wrapper<TransferElement>> elementsToRead) {

    // build list of TransferFutures for all elements. Since readAsync() is a virtual call and we need to visit all
    // elements at least twice (once in wait_for_any and a second time for sorting by version number), this is assumed
    // to be less expensive than calling readAsync() on the fly in the TransferFutureIterator instead.
    std::list<TransferFuture> futureList;
    for(auto &elem : elementsToRead) {
      futureList.push_back(elem.get().readAsync());
    }
    // wait until any future is ready
    auto iter = boost::wait_for_any(TransferFutureIterator(futureList.begin()), TransferFutureIterator(futureList.end()));

    // Find the variable which has the oldest version number (to guarantee the order of updates).
    // Start with assuming that the future returned by boost::wait_for_any() has the oldes version number.
    TransferFuture theUpdate = iter.getTransferFuture();
    for(auto future : futureList) {
      // skip if this is the future returned by boost::wait_for_any()
      if(future == theUpdate) continue;
      // also skip if the future is not yet ready
      if(!future.hasNewData()) continue;
      // compare  version number with the version number of the stored future
      if(future.getBoostFuture().get()->_versionNumber < theUpdate.getBoostFuture().get()->_versionNumber) {
        theUpdate = future;
      }
    }

    // complete the transfer (i.e. run postRead())
    theUpdate.wait();

    // return the transfer element as a shared pointer
    return theUpdate.getTransferElementID();

  }

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_ELEMENT_BRIDGE_H */
