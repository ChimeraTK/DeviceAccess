#include "TransferFuture.h"
#include "TransferElement.h"

namespace ChimeraTK {
  
  void TransferFuture::wait() {
    _theFuture.wait();
    _transferElement->postRead();
    _transferElement->hasActiveFuture = false;
  }

};
