/*
 * NDRegisterAccessorDecorator.h
 *
 *  Created on: Nov 23 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H
#define CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H

#include <boost/make_shared.hpp>

#include <ChimeraTK/cppext/finally.hpp>

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

      void doPreRead(TransferType type) override = 0;

      void doPostRead(TransferType type, bool hasNewData) override = 0;

      void doPreWrite(TransferType type, VersionNumber versionNumber) override = 0;

      void doPostWrite(TransferType type, VersionNumber versionNumber) override = 0;

      void interrupt() override { _target->interrupt(); }

     protected:
      /// The accessor to be decorated
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetUserType>> _target;
    };

    template<typename UserType>
    class NDRegisterAccessorDecoratorImpl<UserType, UserType> : public NDRegisterAccessor<UserType> {
     public:
      using NDRegisterAccessor<UserType>::NDRegisterAccessor;

      void doPreRead(TransferType type) override { _target->preRead(type); }

      void doPostRead(TransferType type, bool hasNewData) override {
        _target->setActiveException(this->_activeException);
        _target->postRead(type, hasNewData);
        if(!hasNewData) return;
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
        this->_dataValidity = _target->dataValidity();
        this->_versionNumber = _target->getVersionNumber();
      }

      void doPreWrite(TransferType type, VersionNumber versionNumber) override {
        for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
        _target->setDataValidity(this->_dataValidity);
        _target->preWrite(type, versionNumber);
      }

      void doPostWrite(TransferType type, VersionNumber versionNumber) override {
        // swap back buffers unconditionally (even if postWrite() throws) at the end of this function
        auto _ = cppext::finally([&] {
          for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].swap(_target->accessChannel(i));
        });
        _target->setActiveException(this->_activeException);
        _target->postWrite(type, versionNumber);
      }

      void interrupt() override { _target->interrupt(); }

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
          target->getName(), target->getAccessModeFlags(), target->getUnit(), target->getDescription()) {
      _target = target;
      this->_readQueue = _target->getReadQueue();
      this->_exceptionBackend = _target->getExceptionBackend();

      // set ID to match the decorated accessor
      this->_id = _target->getId();

      // initialise buffers
      buffer_2D.resize(_target->getNumberOfChannels());
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) buffer_2D[i].resize(_target->getNumberOfSamples());
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
      return _target->writeTransfer(versionNumber);
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
      return _target->writeTransferDestructively(versionNumber);
    }

    void doReadTransferSynchronously() override { _target->readTransfer(); }

    void doPreRead(TransferType type) override { _target->preRead(type); }

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

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
      _target->setExceptionBackend(exceptionBackend);
    }

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
  _target->setExceptionBackend(this->_exceptionBackend);
}

#endif /* CHIMERA_TK_N_D_REGISTER_ACCESSOR_DECORATOR_H */
