#include "VersionNumberUpdatingRegisterDecorator.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  template<typename T>
  void VersionNumberUpdatingRegisterDecorator<T>::doPostRead() {
    NDRegisterAccessorDecorator<T, T>::doPostRead();
    _owner->setCurrentVersionNumber(this->getVersionNumber());
  }

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::VersionNumberUpdatingRegisterDecorator);
