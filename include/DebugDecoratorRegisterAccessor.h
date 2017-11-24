/*
 * DebugDecoratorRegisterAccessor.h
 *
 *  Created on: Nov 21, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR

#include <mtca4u/NDRegisterAccessorDecorator.h>

#include "Application.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the application */
  template<typename UserType>
  class DebugDecoratorRegisterAccessor : public mtca4u::NDRegisterAccessorDecorator<UserType> {
    public:
      DebugDecoratorRegisterAccessor(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> accessor,
                                     const std::string &fullyQualifiedName)
      : mtca4u::NDRegisterAccessorDecorator<UserType>(accessor),
        _fullyQualifiedName(fullyQualifiedName)
      {
        std::cout << "Enable debug output for variable '" << _fullyQualifiedName << "'." << std::endl;
      }

      bool write(ChimeraTK::VersionNumber versionNumber={}) override {
        std::cout << "write() called on '" << _fullyQualifiedName << "'." << std::endl;
        return mtca4u::NDRegisterAccessorDecorator<UserType>::write(versionNumber);
      }

      void doReadTransfer() override {
        std::cout << "doReadTransfer() called on '" << _fullyQualifiedName << "'." << std::endl;
        mtca4u::NDRegisterAccessorDecorator<UserType>::doReadTransfer();
      }

      bool doReadTransferNonBlocking() override {
        std::cout << "doReadTransferNonBlocking() called on '" << _fullyQualifiedName << "'." << std::endl;
        return mtca4u::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking();
      }

      bool doReadTransferLatest() override {
        std::cout << "doReadTransferLatest() called on '" << _fullyQualifiedName << "'." << std::endl;
        return mtca4u::NDRegisterAccessorDecorator<UserType>::doReadTransferLatest();
      }

      TransferFuture readAsync() override {
        if(this->asyncTransferActive()) {
          std::cout << "readAsync() called on '" << _fullyQualifiedName << "' (pre-existing future)." << std::endl;
        }
        else {
          std::cout << "readAsync() called on '" << _fullyQualifiedName << "' (new future)." << std::endl;
        }
        return mtca4u::NDRegisterAccessorDecorator<UserType>::readAsync();
      }

      void postRead() override {
        std::cout << "postRead() called on '" << _fullyQualifiedName << "'." << std::endl;
        mtca4u::NDRegisterAccessorDecorator<UserType>::postRead();
      }

      void preWrite() override {
        std::cout << "preWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
        mtca4u::NDRegisterAccessorDecorator<UserType>::preWrite();
      }

      void postWrite() override {
        std::cout << "postWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
        mtca4u::NDRegisterAccessorDecorator<UserType>::postWrite();
      }

    protected:

      std::string _fullyQualifiedName;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
