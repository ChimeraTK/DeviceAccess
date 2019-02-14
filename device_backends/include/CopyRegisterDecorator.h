/*
 * CopyRegisterDecorator.h
 *
 *  Created on: Dec 12 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_COPY_REGISTER_DECORATOR_H
#define CHIMERATK_COPY_REGISTER_DECORATOR_H

#include "NDRegisterAccessorDecorator.h"

namespace ChimeraTK {

  /** Runtime type trait to identify CopyRegisterDecorators independent of their type. This is used by the
   *  TransferGroup to find all CopyRegisterDecorators and trigger the postRead() action on them first. */
  struct CopyRegisterDecoratorTrait {};

  /** Decorator for NDRegisterAccessors which makes a copy of the data from the target accessor. This must be used
   *  in implementations of TransferElement::replaceTransferElement() when a used accessor shall be replaced with
   *  an accessor used already in another place and thus a copy of the data shall be made. Note that this decorator
   *  is special in the sense that the TransferGroup will call postRead() on them first. Therefore it is mandatory
   *  to use exactly this implementation (potentially extended by inheritance) and not reimplement it directly based
   *  on the NDRegisterAccessorDecorator<T>. */
  template<typename T>
  struct CopyRegisterDecorator : ChimeraTK::NDRegisterAccessorDecorator<T>, CopyRegisterDecoratorTrait {
    CopyRegisterDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>>& target)
    : ChimeraTK::NDRegisterAccessorDecorator<T>(target) {
      if(!target->isReadable()) {
        throw ChimeraTK::logic_error("ChimeraTK::CopyRegisterDecorator: Target accessor is not readable.");
      }
    }

    void doPreWrite() override {
      throw ChimeraTK::logic_error("ChimeraTK::CopyRegisterDecorator: Accessor is not writeable.");
    }

    void doPostRead() override {
      _target->postRead();
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i] = _target->accessChannel(i);
    }

    bool isReadOnly() const override { return true; }

    bool isWriteable() const override { return false; }

    using ChimeraTK::NDRegisterAccessorDecorator<T>::_target;
    using ChimeraTK::NDRegisterAccessorDecorator<T>::buffer_2D;
  };

} // namespace ChimeraTK

#endif /* CHIMERATK_COPY_REGISTER_DECORATOR_H */
