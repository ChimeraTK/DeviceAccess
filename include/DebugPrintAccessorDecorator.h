/*
 * DebugPrintAccessorDecorator.h
 *
 *  Created on: Nov 21, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR
#define CHIMERATK_DEBUG_DECORATOR_REGISTER_ACCCESSOR

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

#include "Application.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the
   * application */
  template<typename UserType>
  class DebugPrintAccessorDecorator : public ChimeraTK::NDRegisterAccessorDecorator<UserType> {
   public:
    DebugPrintAccessorDecorator(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, const std::string& fullyQualifiedName)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), _fullyQualifiedName(fullyQualifiedName) {
      std::cout << "Enable debug output for variable '" << _fullyQualifiedName << "'." << std::endl;
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      std::cout << "doWriteTransfer() called on '" << _fullyQualifiedName << "'." << std::endl;
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransfer(versionNumber);
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber = {}) override {
      std::cout << "doWriteTransferDestructively() called on '" << _fullyQualifiedName << "'." << std::endl;
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransferDestructively(versionNumber);
    }

    void doReadTransfer() override {
      std::cout << "doReadTransfer() called on '" << _fullyQualifiedName << "'." << std::endl;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransfer();
    }

    bool doReadTransferNonBlocking() override {
      std::cout << "doReadTransferNonBlocking() called on '" << _fullyQualifiedName << "'." << std::endl;
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking();
    }

    bool doReadTransferLatest() override {
      std::cout << "doReadTransferLatest() called on '" << _fullyQualifiedName << "'." << std::endl;
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferLatest();
    }

    TransferFuture doReadTransferAsync() override {
      std::cout << "doReadTransferAsync() called on '" << _fullyQualifiedName << std::endl;
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferAsync();
    }

    void doPreRead() override {
      std::cout << "preRead() called on '" << _fullyQualifiedName << "'." << std::endl;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead();
    }

    void doPostRead() override {
      std::cout << "postRead() called on '" << _fullyQualifiedName << "'." << std::endl;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostRead();
    }

    void doPreWrite() override {
      std::cout << "preWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite();
    }

    void doPostWrite() override {
      std::cout << "postWrite() called on '" << _fullyQualifiedName << "'." << std::endl;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite();
    }

   protected:
    std::string _fullyQualifiedName;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_DECORATOR_REGISTER_ACCCESSOR */
