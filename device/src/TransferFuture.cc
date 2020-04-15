#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/chrono.hpp>
#include <boost/ratio.hpp>

#include <ChimeraTK/cppext/finally.hpp>

namespace ChimeraTK {

  void TransferFuture::wait() {
    _transferElement->transferFutureWaitCallback();

    auto _ = cppext::finally(
        [&] { _transferElement->postRead(TransferType::readAsync, !_transferElement->hasSeenException); });

  retry:
    try {
      _notifications.pop_wait();
    }
    catch(detail::DiscardValueException&) {
      goto retry;
    }
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
