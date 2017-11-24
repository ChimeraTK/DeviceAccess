#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/ratio.hpp>
#include <boost/chrono.hpp>

namespace ChimeraTK {

  void TransferFuture::wait() {
    _transferElement->transferFutureWaitCallback();
    _theFuture.wait();
    _transferElement->postRead();
    _transferElement->clearAsyncTransferActive();
  }

  bool TransferFuture::hasNewData() {
    auto status = _theFuture.wait_for(boost::chrono::duration<int, boost::centi>(0));
    return (status != boost::future_status::timeout);
  }

}
