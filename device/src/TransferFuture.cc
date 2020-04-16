#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/chrono.hpp>
#include <boost/ratio.hpp>

#include <ChimeraTK/cppext/finally.hpp>

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
    catch(ChimeraTK::runtime_error& e) {
      _transferElement->hasSeenException = true;
      _transferElement->activeException = e;
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
