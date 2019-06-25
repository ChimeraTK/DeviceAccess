#include "VersionNumberUpdatingRegisterDecorator.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  template<typename T>
  void VersionNumberUpdatingRegisterDecorator<T>::doPostRead() {
    NDRegisterAccessorDecorator<T, T>::doPostRead();

    // update the version number
    if(!isNonblockingRead) {
      _owner->setCurrentVersionNumber(this->getVersionNumber());
    }

    // Check if the data validity flag changed. If yes, propagate this information to the owning module.
    auto valid = ChimeraTK::NDRegisterAccessorDecorator<T>::dataValidity();
    if(valid != lastValidity) {
      if(valid == DataValidity::faulty) {
        _owner->incrementDataFaultCounter();
      }
      else {
        _owner->decrementDataFaultCounter();
      }
      lastValidity = valid;
    }
  }

  template<typename T>
  void VersionNumberUpdatingRegisterDecorator<T>::doPreWrite() {
    ChimeraTK::NDRegisterAccessorDecorator<T>::setDataValidity(_owner->getDataValidity());
    NDRegisterAccessorDecorator<T, T>::doPreWrite();
  }

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::VersionNumberUpdatingRegisterDecorator);
