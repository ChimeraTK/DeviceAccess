#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/chrono.hpp>
#include <boost/ratio.hpp>

namespace ChimeraTK {

  void TransferFuture::wait() {
    _transferElement->transferFutureWaitCallback();
  retry:
    try {
      _notifications.pop_wait();
    }
    catch(detail::DiscardValueException&) {
      goto retry;
    }

    _transferElement->postRead(TransferType::readAsync, !_transferElement->hasSeenException);
  }

  bool TransferFuture::hasNewData() {
  retry:
    try {
      return !(_notifications.empty());
    }
    catch(detail::DiscardValueException&) {
      goto retry;
    }
  }

  ChimeraTK::TransferElementID TransferFuture::getTransferElementID() { return _transferElement->getId(); }

  namespace detail {

    cppext::future_queue<void> getFutureQueueFromTransferFuture(ChimeraTK::TransferFuture& future) {
      return future._notifications;
    }

  } /* namespace detail */

} // namespace ChimeraTK
