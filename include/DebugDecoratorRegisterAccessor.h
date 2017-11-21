/*
 * DebugDecoratorRegisterAccessor.h
 *
 *  Created on: Nov 21, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR

#include <mtca4u/NDRegisterAccessor.h>

#include "Application.h"

namespace ChimeraTK {

  template<typename UserType>
  class DebugDecoratorRegisterAccessor;

  /** Altered version of the TransferFuture. */
  template<typename UserType>
  class DebugDecoratorTransferFuture : public TransferFuture {
    public:

      DebugDecoratorTransferFuture() : _originalFuture{nullptr}, _accessor{nullptr} {}

      virtual ~DebugDecoratorTransferFuture() {}

      void wait() override {
        _originalFuture->wait();
        _accessor->postRead();
        _accessor->hasActiveFuture = false;
      }

      void reset(PlainFutureType plainFuture, mtca4u::TransferElement *transferElement) = delete;

      void reset(TransferFuture &originalFuture, DebugDecoratorRegisterAccessor<UserType> *accessor) {
        _originalFuture = &originalFuture;
        _accessor = accessor;
        TransferFuture::_theFuture = _originalFuture->getBoostFuture();
        TransferFuture::_transferElement = accessor;
      }

    protected:

      // plain pointers are ok, since this class is non-copyable and always owned by the DebugDecoratorRegisterAccessor
      // (which also holds a shared pointer on the actual accessor, which in turn owns the originalFuture).
      TransferFuture *_originalFuture;
      DebugDecoratorRegisterAccessor<UserType> *_accessor;
  };

  /*********************************************************getTwoDRegisterAccessor**********************************************************/

  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class DebugDecoratorRegisterAccessor : public mtca4u::NDRegisterAccessor<UserType> {
    public:
      DebugDecoratorRegisterAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> accessor,
                                     const std::string &fullyQualifiedName)
      : mtca4u::NDRegisterAccessor<UserType>(accessor->getName(), accessor->getUnit(), accessor->getDescription()),
        _accessor(accessor), _fullyQualifiedName(fullyQualifiedName)
      {
        std::cout << "Enable debug output for variable '" << _fullyQualifiedName << "'." << std::endl;

        // set ID to match the decorated accessor
        this->_id = _accessor->getId();
        variableId = Application::getInstance().idMap[this->_id];
        assert(variableId != 0);

        try {
          // initialise buffers
          buffer_2D.resize(_accessor->getNumberOfChannels());
          for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i] = _accessor->accessChannel(i);
        }
        catch(...) {
          this->shutdown();
          throw;
        }
      }

      virtual ~DebugDecoratorRegisterAccessor() { this->shutdown(); }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        std::cout << "write() called on '" << _fullyQualifiedName << "'." << std::endl;
        return _accessor->write(versionNumber);
      }

      void doReadTransfer() override {
        std::cout << "doReadTransfer() called on '" << _fullyQualifiedName << "'." << std::endl;
        _accessor->doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        std::cout << "doReadTransferNonBlocking() called on '" << _fullyQualifiedName << "'." << std::endl;
        return _accessor->doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        std::cout << "doReadTransferLatest() called on '" << _fullyQualifiedName << "'." << std::endl;
        return _accessor->doReadTransferLatest();
      }

      TransferFuture& readAsync() override {
        if(TransferElement::hasActiveFuture) {
          std::cout << "readAsync() called on '" << _fullyQualifiedName << "' (pre-existing future)." << std::endl;
          return activeDebugDecoratorFuture;
        }
        std::cout << "readAsync() called on '" << _fullyQualifiedName << "' (new future)." << std::endl;
        auto &future = _accessor->readAsync();
        TransferElement::hasActiveFuture = true;
        activeDebugDecoratorFuture.reset(future, this);
        return activeDebugDecoratorFuture;
      }

      void postRead() override {
        std::cout << "postRead() called on '" << _fullyQualifiedName << "'." << std::endl;
        if(!TransferElement::hasActiveFuture) _accessor->postRead();
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void preWrite() override {
        std::cout << "preWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
        for(size_t i=0; i<_accessor->getNumberOfChannels(); ++i) buffer_2D[i].swap(_accessor->accessChannel(i));
      }

      void postWrite() override {
        std::cout << "postWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
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

      friend class DebugDecoratorTransferFuture<UserType>;

      DebugDecoratorTransferFuture<UserType> activeDebugDecoratorFuture;

      size_t variableId;

      std::string _fullyQualifiedName;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
