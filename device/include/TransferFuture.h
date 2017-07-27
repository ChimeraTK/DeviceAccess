/*
 * TransferFuture.h
 *
 *  Created on: Feb 20, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TRANSFER_FUTURE_H
#define CHIMERATK_TRANSFER_FUTURE_H

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

#include "VersionNumber.h"
#include "DeviceException.h"
#include "TimeStamp.h"

namespace mtca4u {
  class TransferElement;
  class TransferFutureIterator;
}

namespace ChimeraTK {

  /**
   * Special future returned by TransferElement::readAsync(). See its function description for more details.
   * 
   * Due due a bug in BOOST 1.58 (already fixed in later versions, but we need to support this version), it is not
   * possible to achieve this behaviour with a continuation with a launch policy of boost::launch::deferred, so we have
   * to live with this wrapper class.
   * 
   * Important note: It is strongly recommended to only use those member functions which are present also in
   * boost::shared_future<void> and use the "auto" keyword for the type where possible. This would simplify a possible
   * transition to boost::shared_future<void> when support for the buggy BOOST version is no longer required.
   * 
   * @todo can we make all other functions private?
   */
  class TransferFuture {
    public:
      
      /** Data class for the information transported by the future itself. This class will be extended by backends to
       *  transport additional information, if needed. */
      struct Data {

          Data(VersionNumber versionNumber) : _versionNumber(versionNumber) {}

          /** The version number associated with the transferred data. It must be set properly by the backend to allow
           *  functions like readAny() to sort results properly. */
          VersionNumber _versionNumber;

      };
      
      /** Shortcut for the plain boost::shared_future type. */
      typedef boost::shared_future<Data*> PlainFutureType;
      
      /** Shortcut for the corresponding boost::promise type. */
      typedef boost::promise<Data*> PromiseType;

      /** Block the current thread until the new data has arrived. The TransferElement::postRead() action is
       *  automatically executed before returning, so the new data is directly available in the buffer. */
      virtual void wait();

      /** Check if new data has arrived. If yes, the TransferElement::postRead() action is automatically
       *  executed before returning, so the new data is directly available in the buffer. Otherwise the buffer
       *  is kept unchanged and a subsequent call to wait() or hasNewData() is required to obtain the data.
       
          @todo deprecate and replace with wait_for() to make the interface more similar to standard boost futures.
       */
      bool hasNewData();

      /** Default constructor to generate a dysfunctional future (just for late initialisation) */
      TransferFuture()
      : _transferElement(nullptr) {}

      /** Constructor to generate an already fulfilled future. */
      TransferFuture(mtca4u::TransferElement *transferElement, Data *data)
      : _transferElement(transferElement) {
        PromiseType prom;
        _theFuture = prom.get_future().share();
        prom.set_value(data);
      }
      
      /** Constructor accepting a "plain" boost::shared_future */
      TransferFuture(PlainFutureType plainFuture, mtca4u::TransferElement *transferElement)
      : _theFuture(plainFuture), _transferElement(transferElement)
      {}
      
      /** Return the underlying BOOST future. Be caerful when using it. Simply waiting on that future is not sufficient
       *  since the very purpose of this class is to add functionality. Always call TransferFuture::wait() before
       *  accessing the TransferElement again! */
      PlainFutureType& getBoostFuture() { return _theFuture; }
      
      /** Return the corresponding TransferElement */
      mtca4u::TransferElement& getTransferElement() { return *_transferElement; }
      
      /** Make the TransferFuture non-copyable (otherwise we get problems with polymorphism) */
      TransferFuture(const TransferFuture &other) = delete;
      TransferFuture& operator=(const TransferFuture &other) = delete;
      
      /** Allow move operation on the TransferFuture */
      TransferFuture(const TransferFuture &&other)
      : _theFuture(other._theFuture), _transferElement(other._transferElement) {}

      TransferFuture& operator=(const TransferFuture &&other) {
        _theFuture = other._theFuture;
        _transferElement = other._transferElement;
        return *this;
      }

      
    protected:

      /** The plain boost future */
      PlainFutureType _theFuture;
      
      /** Pointer to the TransferElement */
      mtca4u::TransferElement *_transferElement;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRANSFER_FUTURE_H */
