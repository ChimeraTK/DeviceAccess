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

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreRead(TransferType type) override;

    bool doWriteTransfer(VersionNumber versionNumber) override;
    bool doWriteTransferDestructively(VersionNumber versionNumber) override;

   protected:
    using ChimeraTK::NDRegisterAccessor<UserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::_target;
    using ChimeraTK::TransferElement::_versionNumber;
    using ChimeraTK::TransferElement::_dataValidity;
    using ChimeraTK::TransferElement::_activeException;

    DeviceModule& _deviceModule;

    bool previousReadFailed{true};

    boost::shared_ptr<RecoveryHelper> _recoveryHelper{nullptr};
    boost::shared_ptr<NDRegisterAccessor<UserType>> _recoveryAccessor{nullptr};

    VariableDirection _direction;

    bool _hasThrownToInhibitTransfer{false};
    bool _dataLostInPreviousWrite{false};
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */

#endif // CHIMERATK_EXCEPTION_HANDLING_DECORATOR_H
