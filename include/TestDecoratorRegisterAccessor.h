/*
 * TestDecoratorRegisterAccessor.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR

#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>
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

      TestDecoratorTransferFuture(TransferFuture &originalFuture, TestDecoratorRegisterAccessor<UserType> *accessor)
      : _originalFuture(&originalFuture), _accessor(accessor)
      {
        TransferFuture::_theFuture = _originalFuture->getBoostFuture();
        TransferFuture::_transferElement = accessor;
      }
      
      virtual ~TestDecoratorTransferFuture() {}
      
      void wait() override {
        _accessor->releaseLock();
        boost::this_thread::interruption_point();
        _originalFuture->wait();
        _accessor->obtainLockAndDecrementCounter();
        _accessor->postRead();
        _accessor->hasActiveFuture = false;
      }

      TestDecoratorTransferFuture& operator=(const TestDecoratorTransferFuture &&other) {
        TransferFuture::_theFuture = other._theFuture;
        TransferFuture::_transferElement = other._transferElement;
        _originalFuture = other._originalFuture;
        _accessor = other._accessor;
        return *this;
      }

      TestDecoratorTransferFuture(const TestDecoratorTransferFuture &&other)
      : TransferFuture(other._theFuture, other._transferElement),
        _originalFuture(other._originalFuture),
        _accessor(other._accessor)
      {}

      TestDecoratorTransferFuture(const TestDecoratorTransferFuture &other) = delete;
      TestDecoratorTransferFuture& operator=(const TestDecoratorTransferFuture &other) = delete;

    protected:
      
      // plain pointers are ok, since this class is non-copyable and always owned by the TestDecoratorRegisterAccessor
      // (which also holds a shared pointer on the actual accessor, which in turn owns the originalFuture).
      TransferFuture *_originalFuture;
      TestDecoratorRegisterAccessor<UserType> *_accessor;
  };

  /*********************************************************getTwoDRegisterAccessor**********************************************************/
  
  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class TestDecoratorRegisterAccessor : public mtca4u::NDRegisterAccessor<UserType> {
    public:
      TestDecoratorRegisterAccessor(boost::shared_ptr<ChimeraTK::ProcessArray<UserType>> accessor)
      : mtca4u::NDRegisterAccessor<UserType>(accessor->getName(), accessor->getUnit(), accessor->getDescription()),
        _accessor(accessor)
      {
        try {
          // initialise buffers
          buffer_2D.resize(_accessor->getNumberOfChannels());
          for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i] = _accessor->accessChannel(i);
          
          // if receiving end, register for testable mode (stall detection)
          if(isReadable()) {
            Application::getInstance().testableMode_processVars[getUniqueId()] = accessor;
          }
        }
        catch(...) {
          this->shutdown();
          throw;
        }
      }
      
      virtual ~TestDecoratorRegisterAccessor() { this->shutdown(); }
      
      
      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        bool dataLost = false;
        preWrite();
        if(!Application::testableModeTestLock()) {
          // may happen if first write in thread is done before first blocking read
          Application::testableModeLock("write "+this->getName());
        }
        dataLost = _accessor->write(versionNumber);
        if(!dataLost) {
          ++Application::getInstance().testableMode_counter;
          ++Application::getInstance().testableMode_perVarCounter[_accessor->getUniqueId()];
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestDecoratorRegisterAccessor::write[name='"<<this->getName()<<"']: testableMode_counter "
                         "increased, now at value " << Application::getInstance().testableMode_counter << std::endl; 
          }
        }
        else {
          if(Application::getInstance().enableDebugTestableMode) {
            std::cout << "TestDecoratorRegisterAccessor::write[name='"<<this->getName()<<"']: testableMode_counter not "
                        "increased due to lost data" << std::endl; 
          }
        }
        postWrite();
        return dataLost;
      }

      void doReadTransfer() override {
        releaseLock();
        _accessor->doReadTransfer();
        obtainLockAndDecrementCounter();
      }
      
      /** Release the testableModeLock */
      void releaseLock() {
        try {
          Application::testableModeUnlock("doReadTransfer "+this->getName());
        }
        catch(std::system_error &e) {   // ignore operation not permitted errors, since they happen the first time (lock not yet owned)
          if(e.code() != std::errc::operation_not_permitted) throw e;
        }
      }
      
      /** Obtain the testableModeLock and decrement the counter. */
      void obtainLockAndDecrementCounter() {
        Application::testableModeLock("doReadTransfer "+this->getName());
        assert(Application::getInstance().testableMode_perVarCounter[_accessor->getUniqueId()] > 0);
        assert(Application::getInstance().testableMode_counter > 0);
        --Application::getInstance().testableMode_counter;
        --Application::getInstance().testableMode_perVarCounter[_accessor->getUniqueId()];
        if(Application::getInstance().enableDebugTestableMode) {
          std::cout << "TestDecoratorRegisterAccessor[name='"<<this->getName()<<"']: testableMode_counter "
                       "decreased, now at value " << Application::getInstance().testableMode_counter << " / " << Application::getInstance().testableMode_perVarCounter[_accessor->getUniqueId()] << std::endl; 
        }
      }

      bool doReadTransferNonBlocking() override {
        return _accessor->doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        bool retval = _accessor->doReadTransferLatest();

        // the queue has been emptied, so make sure that the testableMode_counter reflects this
        assert(Application::testableModeTestLock());
        auto &app = Application::getInstance();
        app.testableMode_counter -= app.testableMode_perVarCounter[_accessor->getUniqueId()];
        app.testableMode_perVarCounter[_accessor->getUniqueId()] = 0;
        return retval;
      }

      TransferFuture& readAsync() override {
        if(TransferElement::hasActiveFuture) {
          return activeTestDecoratorFuture;
        }
        auto &future = _accessor->readAsync();
        TransferElement::hasActiveFuture = true;
        activeTestDecoratorFuture = TestDecoratorTransferFuture<UserType>(future, this);
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
      
      size_t getUniqueId() const {
        return _accessor->getUniqueId();
      }

    protected:
      
      using mtca4u::NDRegisterAccessor<UserType>::buffer_2D;

      boost::shared_ptr<ChimeraTK::ProcessArray<UserType>> _accessor;
      
      friend class TestDecoratorTransferFuture<UserType>;
      
      TestDecoratorTransferFuture<UserType> activeTestDecoratorFuture;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
