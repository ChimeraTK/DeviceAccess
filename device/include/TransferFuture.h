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
#include "TransferElementID.h"

namespace mtca4u {
  class TransferElement;
  class TransferElementAbstractor;
}

namespace ChimeraTK {

  /**
   * Special future returned by TransferElement::readAsync(). See its function description for more details.
   *
   * Design note: Due due a bug in BOOST 1.58 (already fixed in later versions, but we need to support this version),
   * it is not possible to achieve this behaviour with a continuation with a launch policy of boost::launch::deferred,
   * so we have to live with this wrapper class. In addition, the wait callback function of boost::future would be an
   * insufficient replacement for the callback mechanism in this TransferFuture, since we need to distinguish whether
   * the wait will block or not.
   */
  class TransferFuture {
    public:

      /** Block the current thread until the new data has arrived. The TransferElement::postRead() action is
       *  automatically executed before returning, so the new data is directly available in the buffer. */
      void wait();

      /** Check if new data has arrived. If new data has arrived (and thus this function returned true), the user still
       *  has to call wait() to initiate the transfer to the user buffer in the accessor. */
      bool hasNewData();

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

      /** Default constructor to generate a dysfunctional future. To initialise the future properly, reset() must be
       *  called afterwards. This pattern is used since the TransferFuture is a member of the TransferElement and only
       *  references are returned by readAsync(). */
      TransferFuture()
      : _transferElement(nullptr) {}

      /** Construct from boost future */
      TransferFuture(PlainFutureType plainFuture, mtca4u::TransferElement *transferElement)
      : _theFuture(plainFuture), _transferElement(transferElement) {}

      /** "Decorating copy constructor": copy from other TransferFuture but override the transfer element. The
       *  typical use case is a decorating TransferElement. */
      TransferFuture(const TransferFuture &other, mtca4u::TransferElement *transferElement)
      : _theFuture(other._theFuture), _transferElement(transferElement) {}

      /** "Decorating move constructor": move from other TransferFuture but override the transfer element. The
       *  typical use case is a decorating TransferElement. */
      TransferFuture(TransferFuture &&other, mtca4u::TransferElement *transferElement)
      : _theFuture(std::move(other._theFuture)), _transferElement(transferElement) {}

      /** Copy constructor */
      TransferFuture(const TransferFuture &) = default;

      /** Move constructor */
      TransferFuture(TransferFuture &&) = default;

      /** Copy assignment operator */
      TransferFuture& operator=(const TransferFuture &) = default;

      /** Move assignment operator */
      TransferFuture& operator=(TransferFuture &&) = default;

      /** Comparison operator: Returns true if the two TransferFutures belong to the same TransferElement. This is
       *  usually sufficient, since only one valid TransferFuture can belong to a TransferElement at a time. */
      bool operator==(const TransferFuture &other) {
        return _transferElement == other._transferElement;
      }

      /** Return the TransferElementID of the associated TransferElement */
      mtca4u::TransferElementID getTransferElementID();

    //protected:    /// @todo make protected after changing tests and ControlSystemAdapter

      /** Return the underlying BOOST future. Be careful when using it. Simply waiting on that future is not sufficient
       *  since the very purpose of this class is to add functionality. Always call TransferFuture::wait() before
       *  accessing the TransferElement again! */
      PlainFutureType& getBoostFuture() { return _theFuture; }

    protected:

      /** The plain boost future */
      PlainFutureType _theFuture;

      /** Pointer to the TransferElement */
      mtca4u::TransferElement *_transferElement;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRANSFER_FUTURE_H */
