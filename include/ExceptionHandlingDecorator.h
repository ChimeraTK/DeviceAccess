#ifndef CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
#define CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
/*
 * ExceptionHandlingDecorator.h
 *
 *  Created on: Mar 18, 2019
 *      Author: Martin Hierholzer
 */

#include <ChimeraTK/NDRegisterAccessorDecorator.h>

#include "Application.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the
   * application */
  template<typename UserType>
  class ExceptionHandlingDecorator : public ChimeraTK::NDRegisterAccessorDecorator<UserType> {
   public:

    /// Basic constructor, inititalizes the base decorator and the reference to the DeviceModule
    ExceptionHandlingDecorator(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod);

    /**
     * Constructor for writeable registers, allows also setting the recoveryAccessor.
     * Deletgates to the basic contructor for member initialization.
     */
    ExceptionHandlingDecorator(
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod,
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> recoveryAccessor);

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override;

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber = {}) override;

    void doReadTransfer() override;

    bool doReadTransferNonBlocking() override;

    bool doReadTransferLatest() override;

    TransferFuture doReadTransferAsync() override;

    void doPreRead() override;
    void doPostRead() override;

    void doPreWrite() override;

    void doPostWrite() override;

    DataValidity dataValidity() const override;

    void setDataValidity(DataValidity validity = DataValidity::ok) override;

    void interrupt() override;

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
    DeviceModule& dm;
    DataValidity validity{DataValidity::ok};
    bool genericTransfer(std::function<bool(void)> callable);
    boost::shared_ptr<NDRegisterAccessor<UserType>> _recoveryAccessor{nullptr};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */

#endif // CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
