#ifndef VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
#define VERSIONNUMBERUPDATINGREGISTERDECORATOR_H

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

/********************************************************************************************************************/

namespace ChimeraTK {

  class EntityOwner;

  /**
   *  NDRegisterAccessorDecorator which propagates meta data attached to input process variables through the owning
   *  ApplicationModule. It will set the current version number of the owning ApplicationModule in postRead. At the
   *  same time it will also propagate the DataValidity flag to/from the owning module.
   */
  template<typename T>
  class MetaDataPropagatingRegisterDecorator : public NDRegisterAccessorDecorator<T, T> {
   public:
    MetaDataPropagatingRegisterDecorator(const boost::shared_ptr<NDRegisterAccessor<T>>& target, EntityOwner* owner)
    : NDRegisterAccessorDecorator<T, T>(target), _owner(owner) {}

    void doPreRead(TransferType type) override { NDRegisterAccessorDecorator<T, T>::doPreRead(type); }

    void doPostRead(TransferType type, bool hasNewData) override;
    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

   protected:
    EntityOwner* _owner;

    /** value of validity flag from last read operation */
    DataValidity lastValidity{DataValidity::ok};

    using TransferElement::_dataValidity;
    using NDRegisterAccessorDecorator<T>::_target;
    using NDRegisterAccessorDecorator<T>::buffer_2D;
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MetaDataPropagatingRegisterDecorator);

} /* namespace ChimeraTK */

#endif // VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
