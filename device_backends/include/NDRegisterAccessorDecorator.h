/*
 * NDRegisterAccessorDecorator.h
 *
 *  Created on: Nov 23 2017
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_N_D_REGISTER_ACCESSOR_DECORATOR_H
#define MTCA4U_N_D_REGISTER_ACCESSOR_DECORATOR_H

#include <boost/make_shared.hpp>

#include "NDRegisterAccessor.h"

namespace mtca4u {

  /** Base class for decorators of the NDRegisterAccessor. This is basically an empty decorator, so implementations can
   *  easily extend by overriding only single functions (and usually calling the implementations of this class inside
   *  the overrides). */
  template<typename UserType, typename TargetUserType=UserType>
  class NDRegisterAccessorDecorator : public NDRegisterAccessor<UserType> {

    public:

      NDRegisterAccessorDecorator(const boost::shared_ptr<mtca4u::NDRegisterAccessor<TargetUserType>> &target)
      : NDRegisterAccessor<UserType>(target->getName(), target->getUnit(), target->getDescription()),
        _target(target)
      {
        // set ID to match the decorated accessor
        this->_id = _target->getId();

        // initialise buffers
        buffer_2D.resize(_target->getNumberOfChannels());
        for(size_t i=0; i<_target->getNumberOfChannels(); ++i) buffer_2D[i] = _target->accessChannel(i);
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        preWrite();
        bool dataLost = _target->write(versionNumber);
        postWrite();
        return dataLost;
      }

      void doReadTransfer() override {
        _target->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        return _target->doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        return _target->doReadTransferLatest();
      }

      TransferFuture readAsync() override {
        return TransferFuture(_target->readAsync(), this);
      }

      bool asyncTransferActive() override {
        return _target->asyncTransferActive();
      }

      void preRead() override {
        _target->preRead();
      }

      void transferFutureWaitCallback() override {
        _target->transferFutureWaitCallback();
      }

      void postRead() override {
        throw std::logic_error("You must implement this function, if UserType is unequal TargetUserType.");
      }

      void preWrite() override {
        throw std::logic_error("You must implement this function, if UserType is unequal TargetUserType.");
      }

      void postWrite() override {
        throw std::logic_error("You must implement this function, if UserType is unequal TargetUserType.");
      }

      void clearAsyncTransferActive() override {
        _target->clearAsyncTransferActive();
      }

      bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const> &other) const override {
        return _target->isSameRegister(other);
      }

      bool isReadOnly() const override {
        return _target->isReadOnly();
      }

      bool isReadable() const override {
        return _target->isReadable();
      }

      bool isWriteable() const override {
        return _target->isWriteable();
      }

      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return _target->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        if(_target->isSameRegister(newElement)) {
          _target = boost::static_pointer_cast< NDRegisterAccessor<TargetUserType> >(newElement);
        }
        else {
        _target->replaceTransferElement(newElement);
        }
      }

      void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) override {
        _target->setPersistentDataStorage(storage);
      }

    protected:

      using mtca4u::NDRegisterAccessor<UserType>::buffer_2D;

      /// The accessor to be decorated
      boost::shared_ptr<mtca4u::NDRegisterAccessor<TargetUserType>> _target;

  };

  /** Partial template specialisation for TargetUserType = UserType
   *  @todo is there any other way than copying basically the entire code??? */
  template<typename UserType>
  class NDRegisterAccessorDecorator<UserType, UserType> : public NDRegisterAccessor<UserType> {

    public:

      NDRegisterAccessorDecorator(const boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> &target)
      : NDRegisterAccessor<UserType>(target->getName(), target->getUnit(), target->getDescription()),
        _target(target)
      {
        // set ID to match the decorated accessor
        this->_id = _target->getId();

        // initialise buffers
        buffer_2D.resize(_target->getNumberOfChannels());
        for(size_t i=0; i<_target->getNumberOfChannels(); ++i) buffer_2D[i] = _target->accessChannel(i);
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        preWrite();
        bool dataLost = _target->write(versionNumber);
        postWrite();
        return dataLost;
      }

      void doReadTransfer() override {
        _target->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        return _target->doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        return _target->doReadTransferLatest();
      }

      TransferFuture readAsync() override {
        return TransferFuture(_target->readAsync(), this);
      }

      bool asyncTransferActive() override {
        return _target->asyncTransferActive();
      }

      void preRead() override {
        _target->preRead();
      }

      void transferFutureWaitCallback() override {
        _target->transferFutureWaitCallback();
      }

      void postRead() override {
        _target->postRead();
        for(size_t i=0; i<_target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
      }

      void preWrite() override {
        for(size_t i=0; i<_target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
      }

      void postWrite() override {
        for(size_t i=0; i<_target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
      }

      void clearAsyncTransferActive() override {
        _target->clearAsyncTransferActive();
      }

      bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const> &other) const override {
        return _target->isSameRegister(other);
      }

      bool isReadOnly() const override {
        return _target->isReadOnly();
      }

      bool isReadable() const override {
        return _target->isReadable();
      }

      bool isWriteable() const override {
        return _target->isWriteable();
      }

      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return _target->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
        if(_target->isSameRegister(newElement)) {
          _target = boost::static_pointer_cast< NDRegisterAccessor<UserType> >(newElement);
        }
        else {
        _target->replaceTransferElement(newElement);
        }
      }

      void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) override {
        _target->setPersistentDataStorage(storage);
      }

    protected:

      using mtca4u::NDRegisterAccessor<UserType>::buffer_2D;

      /// The accessor to be decorated
      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> _target;
  };

}

#endif /* MTCA4U_N_D_REGISTER_ACCESSOR_DECORATOR_H */

