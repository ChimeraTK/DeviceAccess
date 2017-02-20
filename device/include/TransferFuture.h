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
      
      /** Block the current thread until the new data has arrived. The TransferElement::postRead() action is
       *  automatically executed before returning, so the new data is directly available in the buffer. */
      virtual void wait();

      /** Default constructor to generate a dysfunctional future (just for late initialisation) */
      TransferFuture()
      : _transferElement(nullptr) {}

      /** Constructor to generate an already fulfilled future. */
      TransferFuture(mtca4u::TransferElement *transferElement)
      : _transferElement(transferElement) {
        boost::promise<void> prom;
        _theFuture = prom.get_future().share();
        prom.set_value();
      }
      
      /** Constructor accepting a "plain" boost::shared_future */
      TransferFuture(boost::shared_future<void> plainFuture, mtca4u::TransferElement *transferElement)
      : _theFuture(plainFuture), _transferElement(transferElement)
      {}

    protected:
      
      friend class mtca4u::TransferElement;
      friend class mtca4u::TransferFutureIterator;

      /** The plain boost future */
      boost::shared_future<void> _theFuture;
      
      /** Pointer to the TransferElement */
      mtca4u::TransferElement *_transferElement;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRANSFER_FUTURE_H */
