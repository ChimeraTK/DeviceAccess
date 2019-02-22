#ifndef VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
#define VERSIONNUMBERUPDATINGREGISTERDECORATOR_H

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

/********************************************************************************************************************/

namespace ChimeraTK {

  class EntityOwner;

  /**
   *  NDRegisterAccessorDecorator which sets the current version number of the
   * owning ApplicationModule in postRead
   */
  template<typename T>
  struct VersionNumberUpdatingRegisterDecorator : NDRegisterAccessorDecorator<T, T> {
    VersionNumberUpdatingRegisterDecorator(const boost::shared_ptr<NDRegisterAccessor<T>>& target, EntityOwner* owner)
    : NDRegisterAccessorDecorator<T, T>(target), _owner(owner) {}

    void doPostRead() override;

   protected:
    EntityOwner* _owner;
  };

} /* namespace ChimeraTK */

#endif // VERSIONNUMBERUPDATINGREGISTERDECORATOR_H
