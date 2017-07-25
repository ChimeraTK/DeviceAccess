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

#include "VersionNumberSource.h"
#include "DeviceException.h"
#include "TimeStamp.h"

namespace mtca4u {
  class TransferElement;
  class TransferFutureIterator;
}

namespace ChimeraTK {

  /** Special future returned by TransferElement::readAsync(). See its function description for more details. */
  class TransferFuture {
    public:
      
      /** Shortcut for the plain boost::shared_future type. */
      typedef boost::shared_future<VersionNumber::UnderlyingDataType> PlainFutureType;
      
      /** Shortcut for the corresponding boost::promise type. */
      typedef boost::promise<VersionNumber::UnderlyingDataType> PromiseType;

      /** Block the current thread until the new data has arrived. The TransferElement::postRead() action is
       *  automatically executed before returning, so the new data is directly available in the buffer. */
      virtual void wait();

      /** Check if new data has arrived. If yes, the TransferElement::postRead() action is automatically
       *  executed before returning, so the new data is directly available in the buffer. Otherwise the buffer
       *  is kept unchanged and a subsequent call to wait() or hasNewData() is required to obtain the data. */
      bool hasNewData();

      /** Default constructor to generate a dysfunctional future (just for late initialisation) */
      TransferFuture()
      : _transferElement(nullptr) {}

      /** Constructor to generate an already fulfilled future. */
      TransferFuture(mtca4u::TransferElement *transferElement, VersionNumber versionNumber={})
      : _transferElement(transferElement) {
        boost::promise<VersionNumber::UnderlyingDataType> prom;
        _theFuture = prom.get_future().share();
        if(versionNumber.isValid()) {
          prom.set_value(versionNumber);
        }
        else {
          prom.set_value(VersionNumberSource::nextVersionNumber());
        }
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
