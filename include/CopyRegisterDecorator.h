// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

namespace ChimeraTK {

  /** Runtime type trait to identify CopyRegisterDecorators independent of their
   * type. This is used by the TransferGroup to find all CopyRegisterDecorators
   * and trigger the postRead() action on them first. */
  struct CopyRegisterDecoratorTrait {};

  /** Decorator for NDRegisterAccessors which makes a copy of the data from the
   * target accessor. This must be used in implementations of
   * TransferElement::replaceTransferElement() when a used accessor shall be
   * replaced with an accessor used already in another place and thus a copy of
   * the data shall be made. Note that this decorator is special in the sense that
   * the TransferGroup will call postRead() on them first. Therefore it is
   * mandatory to use exactly this implementation (potentially extended by
   * inheritance) and not reimplement it directly based on the
   * NDRegisterAccessorDecorator<T>. */
  template<typename T>
  struct CopyRegisterDecorator : ChimeraTK::NDRegisterAccessorDecorator<T>, CopyRegisterDecoratorTrait {
    explicit CopyRegisterDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>>& target)
    : ChimeraTK::NDRegisterAccessorDecorator<T>(target) {
      if(!target->isReadable()) {
        throw ChimeraTK::logic_error("ChimeraTK::CopyRegisterDecorator: Target accessor is not readable.");
      }
    }

    void doPreRead(TransferType) override {
      // Do nothing. This should only ever be called from the TransferGroup which has already handled
      // the preRead differently
    }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("ChimeraTK::CopyRegisterDecorator: Accessor is not writeable.");
    }

    void doPostRead(TransferType type, bool hasNewData) override {
      _target->postRead(type, hasNewData);
      if(hasNewData) {
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i] = _target->accessChannel(i);
      }
    }

    void doReadTransferSynchronously() {
      std::cerr << "CopyRegisterDecorator::doReadTransferSynchronously: Must not be called" << std::endl;
      assert(false);
    }

    [[nodiscard]] bool isReadOnly() const override { return true; }

    [[nodiscard]] bool isWriteable() const override { return false; }

    using ChimeraTK::NDRegisterAccessorDecorator<T>::_target;
    using ChimeraTK::NDRegisterAccessorDecorator<T>::buffer_2D;
  };

} // namespace ChimeraTK
