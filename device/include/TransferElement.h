/*
 * TransferElement.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_TRANSFER_ELEMENT_H
#define MTCA4U_TRANSFER_ELEMENT_H

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

namespace ChimeraTK {
  class PersistentDataStorage;
}

namespace mtca4u {

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
      
      /** Copy constructor: do not allow copying when in TransferGroup, remove asynchronous read state */
      TransferElement(const TransferElement &other)
      : boost::enable_shared_from_this<TransferElement>(),
        _name(other._name),
        _unit(other._unit),
        _description(other._description),
        isInTransferGroup{false}
      {
        if(other.isInTransferGroup) {
          throw DeviceException("Copying a TransferElement which is part of a TransferGroup is not allowed.",
              DeviceException::WRONG_PARAMETER);
        }
      }
      
      /** Abstract base classes need a virtual destructor. */
      virtual ~TransferElement() {}

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
          bool ret = doReadTransferNonBlocking();
          if(ret) postRead();     // only needs to be called if new data was read
          return ret;
      }
      
      /** Read data from the device in the background and return a future which will be fulfilled when the data is
       *  ready. When the future is fulfilled, the transfer element will already contain the new data, there is no
       *  need to call read() or readNonBlocking() (which would trigger another data transfer). Any call to read(),
       *  readNonBlocking() or write() is illegal until the future has been fulfilled (and this fact has been noted
       *  by calling e.g. TransferFuture::wait()).
       * 
       *  It is allowed to call this function multiple times, which will return the same (shared) future until it
       *  is fulfilled.
       * 
       *  The future will be fulfilled at the time when normally read() would return. A call to this function is
       *  roughly logically equivalent to:
       *    boost::async( boost::bind(&TransferElemennt::read, this) );
       *  (Although such implementation would disallow accessing the user data buffer until the future is fulfilled,
       *  which is not the case for this function.)
       *
       *  Design note: A special type of future has to be returned to allow an abstraction from the implementation
       *  details of the backend. This allows - depending on the backend type - a more efficient implementation
       *  without launching a thread. A reference is returned since we need polymorphism. This is possible since
       *  the implementation anyway needs to store the object to be able to return it a second time if readAsync()
       *  is called again before the future is fulfilled.
       *
       *  Note: This feature is still experimental. Expect API changes without notice! */
      virtual TransferFuture& readAsync() = 0;
      
      /** Read the latest value, discarding any other update since the last read if present. Otherwise this function
       *  is identical to readNonBlocking(), i.e. it will never wait for new values and it will return whether a
       *  new value was available if AccessMode::wait_for_new_data is set. */
      bool readLatest() {
        bool ret = doReadTransferLatest();
        if(ret) postRead();     // only needs to be called if new data was read
        return ret;
      }

      /** Read data asynchronously from all given TransferElements and wait until one of the TransferElements has
       *  new data. The TransferElement which received new data is returned as a reference. When this function
       *  returns, the all elements but the returned TransferElement still have a pending transfer, thus calling
       *  read() or write() and similar functions on them is illegal. Only their current values may be accessed,
       *  another readAny() or readAsync() may be called. */
      static boost::shared_ptr<TransferElement> readAny(std::list<std::reference_wrapper<TransferElement>> elementsToRead);

      /**
      * Returns the version number that is associated with the last transfer (i.e. last read or write). See
      * ChimeraTK::VersionNumber for details.
      * 
      * The returned version number may be invalid (i.e. ChimeraTK::VersionNumber::isValid() returns false), if
      * AccessMode::wait_for_new_data as not been specified. If AccessMode::wait_for_new_data was specified, a valid
      * version number must be returned (either obtained from the ChimeraTK::VersionNumberSource or derived from an
      * existing version number e.g. of another variable).
      */
      virtual ChimeraTK::VersionNumber getVersionNumber() const {
        return ChimeraTK::VersionNumber();
      }

      /** Write the data to device. The return value is true, old data was lost on the write transfer (e.g. due to an
       *  buffer overflow). In case of an unbuffered write transfer, the return value will always be false. */
      virtual bool write(ChimeraTK::VersionNumber versionNumber={}) = 0;

      /** Read the data from the device but do not fill it into the user buffer of this TransferElement. Calling this
       *  function followed by postRead() is exactly equivalent to a call to just read().
       *
       *  Implementation note: This function must return within ~1 second after boost::thread::interrupt() has been
       *  called on the thread calling this function. */
      virtual void doReadTransfer() = 0;

      /** Read the data from the device without blocking but do not fill it into the user buffer of this
       *  TransferElement. Calling this function followed by postRead() is exactly equivalent to a call to just
       *  readNonBlocking(). For the return value, see readNonBlocking(). */
      virtual bool doReadTransferNonBlocking() = 0;

      /** Read the latest data from the device without blocking but do not fill it into the user buffer of this
       *  TransferElement. Calling this function followed by postRead() is exactly equivalent to a call to just
       *  readLatest(). For the return value, see readNonBlocking(). */
      virtual bool doReadTransferLatest() = 0;

      /** Transfer the data from the device receive buffer into the user buffer, while converting the data into the
       *  user data format if needed.
       * 
       *  Called by the TransferGroup after a read was executed directly on the underlying accessor. This function
       *  must be implemented to extract the read data from the underlying accessor and expose it to the user. */
      virtual void postRead() {};

      /** Transfer the data from the user buffer into the device send buffer, while converting the data from then
       *  user data format if needed.
       *  
       *  Called by the TransferGroup before a write will be executed directly on the underlying accessor. This
       *  function implemented be used to transfer the data to be written into the underlying accessor. */
      virtual void preWrite() {};

      /** Perform any post-write cleanups if necessary. If during preWrite() e.g. the user data buffer was swapped
       *  away, it must be swapped back in this function so the just sent data is available again to the calling
       *  program.
       *  
       *  Called by the TransferGroup after a write will be executed directly on the underlying accessor. */
      virtual void postWrite() {};

      /** 
       *  Check if the two TransferElements are identical, i.e. accessing the same hardware register. The definition of
       *  an "hardware register" is strongly depending on the backend implementation, thus using this function in
       *  application code will probably break the abstraction!
       *
       *  @todo Rename this function to something more appropriate (e.g. mayJoinTransfer?)
       */
      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const = 0;

      /** Check if transfer element is read only, i\.e\. it is readable but not writeable. */
      virtual bool isReadOnly() const = 0;

      /** Check if transfer element is readable. It throws an acception if you try to read and 
       *  isReadable() is not true.*/
      virtual bool isReadable() const = 0;
      
      /** Check if transfer element is writeable. It throws an acception if you try to write and 
       *  isWriteable() is not true.*/
      virtual bool isWriteable() const = 0;
      
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
       *
       *  Note: Avoid using this in application code, since it will break the abstraction!
       */
      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) = 0;

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
      virtual void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage>) {};

    protected:

      /** Identifier uniquely identifying the TransferElement */
      std::string _name;

      /** Engineering unit. Defaults to "n./a.", if none was specified */
      std::string _unit;

      /** Description of this variable/register */
      std::string _description;

      /** Flag whether this TransferElement has been added to a TransferGroup or not */
      bool isInTransferGroup;
      
      /** Flag whether there is an "active" TransferFuture which was given out by readAync() but its ready state was
       *  not yet obtained by the user e.g. through TransferFuture::wait(). */
      bool hasActiveFuture{false};
      
      /** The currently "active" future, if hasActiveFuture == true */
      TransferFuture activeFuture;
      
      friend class TransferGroup;
      friend class TransferFuture;
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
      TransferFutureIterator(const std::list<TransferFuture*>::iterator &it) : _it(it) {}
      TransferFutureIterator operator++() { TransferFutureIterator ret(_it); ++_it; return ret; }
      TransferFutureIterator operator++(int) { ++_it; return *this; }
      bool operator!=(const TransferFutureIterator &rhs) {return _it != rhs._it;}
      bool operator==(const TransferFutureIterator &rhs) {return _it == rhs._it;}
      TransferFuture::PlainFutureType& operator*() {return (*_it)->getBoostFuture();}
      TransferFuture::PlainFutureType& operator->() {return (*_it)->getBoostFuture();}
      TransferFuture& getTransferFuture() const {return **_it;}
    private:
      std::list<TransferFuture*>::iterator _it;
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
  
  inline boost::shared_ptr<TransferElement> TransferElement::readAny(std::list<std::reference_wrapper<TransferElement>> elementsToRead) {
    std::list<TransferFuture*> futureList;
    for(auto elem : elementsToRead) {
      auto *future = &(elem.get().readAsync());
      futureList.push_back(future);
    }
    // See class description of TransferFutureIterator. We need this trick to obtain the transfer element from the
    // returned iterator.
    auto iter = boost::wait_for_any(TransferFutureIterator(futureList.begin()), TransferFutureIterator(futureList.end()));
    boost::this_thread::interruption_point();
    iter.getTransferFuture().wait();    // complete the transfer (i.e. run postRead())
    boost::this_thread::interruption_point();
    return iter.getTransferFuture().getTransferElement().shared_from_this();
  }

} /* namespace mtca4u */

#endif /* MTCA4U_TRANSFER_ELEMENT_H */
