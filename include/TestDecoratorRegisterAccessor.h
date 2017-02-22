/*
 * TestDecoratorRegisterAccessor.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR

#include <mtca4u/NDRegisterAccessor.h>

#include "Application.h"

namespace ChimeraTK {

  template<typename UserType>
  class TestDecoratorRegisterAccessor;

  /** Altered version of the TransferFuture since we need to deal with the lock in the wait() function. */
  template<typename UserType>
  class TestDecoratorTransferFuture : public TransferFuture {
    public:
      
      TestDecoratorTransferFuture() : _originalFuture{nullptr} {}

      TestDecoratorTransferFuture(TransferFuture &originalFuture,
                                  boost::shared_ptr<TestDecoratorRegisterAccessor<UserType>> accessor)
      : _originalFuture(&originalFuture), _accessor(accessor)
      {
        TransferFuture::_theFuture = _originalFuture->getBoostFuture();
        TransferFuture::_transferElement = &(_originalFuture->getTransferElement());
      }
      
      void wait() override {
        try {
          Application::getTestableModeLockObject().unlock();
        }
        catch(std::system_error &e) {   // ignore operation not permitted errors, since they happen the first time (lock not yet owned)
          if(e.code() != std::errc::operation_not_permitted) throw e;
        }
        _originalFuture->wait();
        _accessor->postRead();
        _accessor->hasActiveFuture = false;
        Application::getTestableModeLockObject().lock();
        --Application::getInstance().testableMode_counter;
      }

    protected:
      
      TransferFuture *_originalFuture;
      
      boost::shared_ptr<TestDecoratorRegisterAccessor<UserType>> _accessor;
  };

  /*******************************************************************************************************************/
  
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
      
      void write() override {
        preWrite();
        auto &myLock = Application::getTestableModeLockObject();
        if(!myLock.owns_lock()) myLock.lock();   // may happen if first write in thread is done before first blocking read
        ++Application::getInstance().testableMode_counter;
        _accessor->write();
        postWrite();
      }

      void doReadTransfer() override {
        try {
          Application::getTestableModeLockObject().unlock();
        }
        catch(std::system_error &e) {   // ignore operation not permitted errors, since they happen the first time (lock not yet owned)
          if(e.code() != std::errc::operation_not_permitted) throw e;
        }
        _accessor->doReadTransfer();
        Application::getTestableModeLockObject().lock();
        --Application::getInstance().testableMode_counter;
      }

      bool doReadTransferNonBlocking() override {
        return _accessor->doReadTransferNonBlocking();
      }

      TransferFuture& readAsync() override {
        auto &future = _accessor->readAsync();
        auto sharedThis = boost::static_pointer_cast<TestDecoratorRegisterAccessor<UserType>>(this->shared_from_this());
        TransferElement::hasActiveFuture = true;
        activeTestDecoratorFuture = TestDecoratorTransferFuture<UserType>(future, sharedThis);
        return activeTestDecoratorFuture;
      }

      void postRead() override {
        if(!TransferElement::hasActiveFuture) _accessor->postRead();
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void preWrite() override {
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void postWrite() override {
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
      
      friend class TestDecoratorTransferFuture<UserType>;
      
      TestDecoratorTransferFuture<UserType> activeTestDecoratorFuture;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
