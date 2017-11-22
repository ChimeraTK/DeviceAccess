#include "TransferFuture.h"
#include "TransferElement.h"

#include <boost/ratio.hpp>
#include <boost/chrono.hpp>

namespace ChimeraTK {

  void TransferFuture::wait() {
    _theFuture.wait();
    _transferElement->postRead();
    _transferElement->hasActiveFuture = false;
  }

  bool TransferFuture::hasNewData() {
    auto status = _theFuture.wait_for(boost::chrono::duration<int, boost::centi>(0));
    if(status != boost::future_status::timeout) {
      wait();
      return true;
    }
    return false;
  }

}
