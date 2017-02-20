/*
 * TestDecoratorRegisterAccessor.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR

#include <mtca4u/NDRegisterAccessor.h>

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class TestDecoratorRegisterAccessor : public mtca4u::NDRegisterAccessor<UserType> {
    public:
      TestDecoratorRegisterAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> accessor)
      : mtca4u::NDRegisterAccessor<UserType>(accessor->getName(), accessor->getUnit(), accessor->getDescription()),
        _accessor(accessor) {
        buffer_2D.resize(_accessor->getNumberOfChannels());
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i] = _accessor->accessChannel(i);
      }

      mtca4u::TransferFuture readAsync() override {
        return _accessor->readAsync();
      }
      
      void write() override {
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
        _accessor->write();
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void doReadTransfer() override {
        _accessor->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        return _accessor->doReadTransferNonBlocking();
      }

      void postRead() override {
        _accessor->postRead();
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void preWrite() override {
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
        _accessor->preWrite();
      }

      void postWrite() override {
        _accessor->postWrite();
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      bool isSameRegister(const boost::shared_ptr<mtca4u::TransferElement const> &other) const override {
        return _accessor->isSameRegister(other);
      }

      bool isReadOnly() const override {
        return _accessor->isReadOnly();
      }

      bool isReadable() const override {
        return _accessor->isReadable();
      }
      
      bool isWriteable() const override {
        return _accessor->isWriteable();
      }
      
      std::vector< boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return _accessor->getHardwareAccessingElements();
      }

      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement> newElement) override {
        _accessor->replaceTransferElement(newElement);
      }

      void setPersistentDataStorage(boost::shared_ptr<ChimeraTK::PersistentDataStorage> storage) override {
        _accessor->setPersistentDataStorage(storage);
      }

    protected:
      
      using mtca4u::NDRegisterAccessor<UserType>::buffer_2D;

      boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> _accessor;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
