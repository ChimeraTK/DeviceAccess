/*
 * NDRegisterAccessorDecorator.h
 *
 *  Created on: Nov 23 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H
#define CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H

#include <boost/make_shared.hpp>

#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  namespace detail {

    /** Do not use directly, use NDRegisterAccessorDecorator instead!
     *
     *  Intermediate class just to make implementations of
     * postRead/preWrite/postWrite depending on whether TargetUserType is equal or
     * unequal to UserType. Default implementations for these functions are provided
     * only in case TargetUserType is equal to UserType. The functions must be
     * implemented by the actual decorator, if the types are unequal. Otherwise they
     * still can be implemented, but the default provides a useful behavior for many
     *  cases (buffers are simply swapped). */
    template<typename UserType, typename TargetUserType = UserType>
    class NDRegisterAccessorDecoratorImpl : public NDRegisterAccessor<UserType> {
     public:
      using NDRegisterAccessor<UserType>::NDRegisterAccessor;

      void doPreRead() override = 0;

      void doPostRead() override = 0;

      void doPreWrite() override = 0;

      void doPostWrite() override = 0;

      void interrupt() override { _target->interrupt(); }

      ChimeraTK::VersionNumber getVersionNumber() const override { return _target->getVersionNumber(); }

      ChimeraTK::DataValidity dataValidity() const override { return _target->dataValidity(); }

      void setDataValidity(ChimeraTK::DataValidity validity = ChimeraTK::DataValidity::ok) override {
        _target->setDataValidity(validity);
      }

     protected:
      /// The accessor to be decorated
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> _target;
    };

    template<typename UserType>
    class NDRegisterAccessorDecoratorImpl<UserType, UserType> : public NDRegisterAccessor<UserType> {
     public:
      using NDRegisterAccessor<UserType>::NDRegisterAccessor;

      void doPreRead() override { _target->preRead(); }

      void doPostRead() override {
        _target->postRead();
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
      }

      void doPreWrite() override {
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
        _target->preWrite();
      }

      void doPostWrite() override {
        _target->postWrite();
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
      }

      void interrupt() override { _target->interrupt(); }

      ChimeraTK::VersionNumber getVersionNumber() const override { return _target->getVersionNumber(); }

      ChimeraTK::DataValidity dataValidity() const override { return _target->dataValidity(); }

      void setDataValidity(ChimeraTK::DataValidity validity = ChimeraTK::DataValidity::ok) override {
        _target->setDataValidity(validity);
      }

     protected:
      using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;

      /// The accessor to be decorated
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _target;
    };

  } // namespace detail

  /** Base class for decorators of the NDRegisterAccessor. This is basically an
   * empty decorator, so implementations can easily extend by overriding only
   * single functions (and usually calling the implementations of this class
   * inside the overrides). */
  template<typename UserType, typename TargetUserType = UserType>
  class NDRegisterAccessorDecorator : public detail::NDRegisterAccessorDecoratorImpl<UserType, TargetUserType> {
   public:
    NDRegisterAccessorDecorator(const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>>& target)
    : detail::NDRegisterAccessorDecoratorImpl<UserType, TargetUserType>(
          target->getName(), target->getUnit(), target->getDescription()) {
      _target = target;

      // set ID to match the decorated accessor
      this->_id = _target->getId();

      // initialise buffers
      buffer_2D.resize(_target->getNumberOfChannels());
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].resize(_target->getNumberOfSamples());
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      return _target->doWriteTransfer(versionNumber);
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber = {}) override {
      return _target->doWriteTransferDestructively(versionNumber);
    }

    void doReadTransfer() override { _target->doReadTransfer(); }

    bool doReadTransferNonBlocking() override { return _target->doReadTransferNonBlocking(); }

    bool doReadTransferLatest() override { return _target->doReadTransferLatest(); }

    TransferFuture doReadTransferAsync() override { return TransferFuture(_target->readAsync(), this); }

    void doPreRead() override { _target->preRead(); }

    void transferFutureWaitCallback() override { _target->transferFutureWaitCallback(); }

    bool isReadOnly() const override { return _target->isReadOnly(); }

    bool isReadable() const override { return _target->isReadable(); }

    bool isWriteable() const override { return _target->isWriteable(); }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override {
      return _target->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      auto result = _target->getInternalElements();
      result.push_front(_target);
      return result;
    }

    void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) override {
      _target->setPersistentDataStorage(storage);
    }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> newElement) override;

    AccessModeFlags getAccessModeFlags() const override { return _target->getAccessModeFlags(); }

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;

    using detail::NDRegisterAccessorDecoratorImpl<UserType, TargetUserType>::_target;
  };

  namespace detail {

    /** Factory to create an instance of the CopyRegisterDecorator. This factory is
     * required to break a circular dependency between this include file and
     * CopyRegisterDecorator.h, which would occur if we would just create the
     * instance here. */
    template<typename T>
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> createCopyDecorator(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> target);

  } // namespace detail

} // namespace ChimeraTK

template<typename UserType, typename TargetUserType>
void ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetUserType>::replaceTransferElement(
    boost::shared_ptr<ChimeraTK::TransferElement> newElement) {
  auto casted = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<TargetUserType>>(newElement);
  if(casted && newElement->mayReplaceOther(_target)) {
    if(_target != newElement) {
      _target = detail::createCopyDecorator<TargetUserType>(casted);
    }
  }
  else {
    _target->replaceTransferElement(newElement);
  }
}

#endif /* CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H */
