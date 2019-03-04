/*
 * TransferFuture.h
 *
 *  Created on: Feb 20, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TRANSFER_FUTURE_H
#define CHIMERATK_TRANSFER_FUTURE_H

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

#include <ChimeraTK/cppext/future_queue.hpp>

#include "TransferElementID.h"
#include "VersionNumber.h"

namespace ChimeraTK {
  class TransferElement;
  class TransferElementAbstractor;
  class TransferFuture;
} // namespace ChimeraTK

namespace ChimeraTK {

  namespace detail {

    /**
     *  Non-member function to obtain the underlying future_queue from a
     * TransferFuture. This function should only be used internally or by
     * implementations of DeviceBackend.
     *
     *  The queue can also be used for later transfers. Note that if
     * AccessMode::wait_for_new_data is not used for the accessor one must still
     * trigger the transfers by calling readAsync().
     */
    cppext::future_queue<void> getFutureQueueFromTransferFuture(ChimeraTK::TransferFuture& future);

    /**
     *  Exception to be thrown by continuations of the notification queue used in
     * TransferFuture when a value shall be discarded. This is needed to avoid
     * notifications of the application if a value should never reach the
     *  application. The exception is caught in TransferFuture::wait() and
     * TransferFuture::hasNewData() and should never be visible to the application.
     */
    class DiscardValueException {};

  } /* namespace detail */

  /**
   * Special future returned by TransferElement::readAsync(). See its function
   * description for more details.
   *
   * Design note: Due due a bug in BOOST 1.58 (already fixed in later versions,
   * but we need to support this version), it is not possible to achieve this
   * behaviour with a continuation with a launch policy of
   * boost::launch::deferred, so we have to live with this wrapper class. In
   * addition, the wait callback function of boost::future would be an
   * insufficient replacement for the callback mechanism in this TransferFuture,
   * since we need to distinguish whether the wait will block or not.
   */
  class TransferFuture {
   public:
    /** Block the current thread until the new data has arrived. The
     * TransferElement::postRead() action is automatically executed before
     * returning, so the new data is directly available in the buffer. */
    void wait();

    /** Check if new data has arrived. If new data has arrived (and thus this
     * function returned true), the user still
     *  has to call wait() to initiate the transfer to the user buffer in the
     * accessor. */
    bool hasNewData();

    /** Default constructor to generate a dysfunctional future. To initialise the
     * future properly, reset() must be called afterwards. This pattern is used
     * since the TransferFuture is a member of the TransferElement and only
     *  references are returned by readAsync(). */
    TransferFuture() : _transferElement(nullptr) {}

    /** Construct from a future_queue<void>. The queue should contain
     * notifications when a transfer is complete. It is the responsibility of this
     * TransferFuture to call the proper postRead() function, so the notification
     * must only be about the completion of the equivalent of doReadTransfer(). */
    TransferFuture(cppext::future_queue<void> notifications, ChimeraTK::TransferElement* transferElement)
    : _notifications(notifications), _transferElement(transferElement) {}

    /** "Decorating copy constructor": copy from other TransferFuture but override
     * the transfer element. The
     *  typical use case is a decorating TransferElement. */
    TransferFuture(const TransferFuture& other, ChimeraTK::TransferElement* transferElement)
    : _notifications(other._notifications), _transferElement(transferElement) {}

    /** "Decorating move constructor": move from other TransferFuture but override
     * the transfer element. The
     *  typical use case is a decorating TransferElement. */
    TransferFuture(TransferFuture&& other, ChimeraTK::TransferElement* transferElement)
    : _notifications(std::move(other._notifications)), _transferElement(transferElement) {}

    /** Copy constructor */
    TransferFuture(const TransferFuture&) = default;

    /** Move constructor */
    TransferFuture(TransferFuture&&) = default;

    /** Copy assignment operator */
    TransferFuture& operator=(const TransferFuture&) = default;

    /** Move assignment operator */
    TransferFuture& operator=(TransferFuture&&) = default;

    /** Comparison operator: Returns true if the two TransferFutures belong to the
     * same TransferElement. This is usually sufficient, since only one valid
     * TransferFuture can belong to a TransferElement at a time. */
    bool operator==(const TransferFuture& other) { return _transferElement == other._transferElement; }

    /** Return the TransferElementID of the associated TransferElement */
    ChimeraTK::TransferElementID getTransferElementID();

    /** Check if the TransferFuture is valid */
    bool valid() const { return _transferElement != nullptr; }

   protected:
    /** The plain future_queue used for the notifications */
    cppext::future_queue<void> _notifications;

    /** Pointer to the TransferElement */
    ChimeraTK::TransferElement* _transferElement;

    friend cppext::future_queue<void> ChimeraTK::detail::getFutureQueueFromTransferFuture(
        ChimeraTK::TransferFuture& future);
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRANSFER_FUTURE_H */
