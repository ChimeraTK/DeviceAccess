#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/ratio.hpp>
#include <boost/chrono.hpp>

namespace ChimeraTK {

  void TransferFuture::wait() {
    _transferElement->transferFutureWaitCallback();
    _notifications.pop_wait();
    _transferElement->postRead();
  }

  bool TransferFuture::hasNewData() {
    return !(_notifications.empty());
  }

  ChimeraTK::TransferElementID TransferFuture::getTransferElementID() {
    return _transferElement->getId();
  }

  namespace detail {

    cppext::future_queue<void> getFutureQueueFromTransferFuture(ChimeraTK::TransferFuture &future) {
      return future._notifications;
    }

  } /* namespace detail */

}
