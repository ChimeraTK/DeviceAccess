#ifndef CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
#define CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
/*
 * ExceptionHandlingDecorator.h
 *
 *  Created on: Mar 18, 2019
 *      Author: Martin Hierholzer
 */

#include <ChimeraTK/NDRegisterAccessorDecorator.h>
#include "RecoveryHelper.h"

#include "Application.h"

namespace ChimeraTK {

  /** Decorator of the NDRegisterAccessor which facilitates tests of the
   * application */
  template<typename UserType>
  class ExceptionHandlingDecorator : public ChimeraTK::NDRegisterAccessorDecorator<UserType> {
   public:
    /**
     * Decorate the accessors which is handed in the constuctor. It needs the device module to implement
     * the exception handling. Accessors which write to the device in addition need a recovery accessor
     * so the variables can be written again after a device has recovered from an error. Accessors
     * which only read don't specify the third parameter.
     */
    ExceptionHandlingDecorator(boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor,
        DeviceModule& devMod, VariableDirection direction,
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> recoveryAccessor = {nullptr});

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override;

    void doReadTransferSynchronously() override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    void setOwner(EntityOwner* owner);

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreRead(TransferType type) override;

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
    DeviceModule& deviceModule;

    bool previousReadFailed{true};

    void setOwnerValidity(bool hasExceptionNow);
    boost::shared_ptr<RecoveryHelper> _recoveryHelper{nullptr};
    boost::shared_ptr<NDRegisterAccessor<UserType>> _recoveryAccessor{nullptr};

    EntityOwner* _owner = {nullptr};
    VariableDirection _direction;

    bool transferAllowed{false};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */

#endif // CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
