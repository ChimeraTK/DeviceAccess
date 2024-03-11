// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TransferElementAbstractor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  std::vector<boost::shared_ptr<TransferElement>> TransferElementAbstractor::getHardwareAccessingElements() {
    return _impl->getHardwareAccessingElements();
  }

  /********************************************************************************************************************/

  std::list<boost::shared_ptr<TransferElement>> TransferElementAbstractor::getInternalElements() {
    auto result = _impl->getInternalElements();
    result.push_front(_impl);
    return result;
  }

  /********************************************************************************************************************/

  void TransferElementAbstractor::replaceTransferElement(const boost::shared_ptr<TransferElement>& newElement) {
    if(newElement->mayReplaceOther(_impl)) {
      if(newElement != _impl) {
        _impl = newElement->makeCopyRegisterDecorator();
      }
    }
    else {
      _impl->replaceTransferElement(newElement);
    }
  }

  /********************************************************************************************************************/

  void TransferElementAbstractor::setPersistentDataStorage(
      boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) {
    _impl->setPersistentDataStorage(std::move(storage));
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
