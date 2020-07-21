#include "MetaDataPropagatingRegisterDecorator.h"
#include "EntityOwner.h"

namespace ChimeraTK {

  template<typename T>
  void MetaDataPropagatingRegisterDecorator<T>::doPostRead(TransferType type, bool hasNewData) {
    NDRegisterAccessorDecorator<T, T>::doPostRead(type, hasNewData);

    // update the version number
    if(type == TransferType::read) {
      _owner->setCurrentVersionNumber(this->getVersionNumber());
    }

    // Check if the data validity flag changed. If yes, propagate this information to the owning module.
    if(_dataValidity != lastValidity) {
      if(_dataValidity == DataValidity::faulty)
        _owner->incrementDataFaultCounter();
      else
        _owner->decrementDataFaultCounter();
      lastValidity = _dataValidity;
    }
  }

  template<typename T>
  void MetaDataPropagatingRegisterDecorator<T>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    // We cannot use NDRegisterAccessorDecorator<T> here because we need a different implementation of setting the target data validity.
    // So we have a complete implemetation here.
    if(_dataValidity == DataValidity::faulty) { // the application has manualy set the validity to faulty
      _target->setDataValidity(DataValidity::faulty);
    }
    else { // automatic propagation of the owner validity
      _target->setDataValidity(_owner->getDataValidity());
    }

    for(unsigned int i = 0; i < _target->getNumberOfChannels(); ++i) {
      buffer_2D[i].swap(_target->accessChannel(i));
    }
    _target->preWrite(type, versionNumber);
  }

} // namespace ChimeraTK

INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ChimeraTK::MetaDataPropagatingRegisterDecorator);
