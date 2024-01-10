// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DummyInterruptTriggerAccessor.h"

#include "DummyBackendBase.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType>
  DummyInterruptTriggerAccessor<UserType>::DummyInterruptTriggerAccessor(boost::shared_ptr<DeviceBackend> backend,
      std::function<VersionNumber(void)> interruptTrigger, const RegisterPath& registerPathName,
      size_t numberOfElements, size_t elementsOffset, const AccessModeFlags& flags)
  : NDRegisterAccessor<UserType>(registerPathName, {}),
    _backend(boost::dynamic_pointer_cast<DummyBackendBase>(backend)), _interruptTrigger(std::move(interruptTrigger)) {
    assert(_backend); // Accessor created with wrong backend type?

    if(numberOfElements > 1) {
      throw ChimeraTK::logic_error("DUMMY_INTERRUPT accessor register can have at most one element");
    }

    if(elementsOffset != 0) {
      throw ChimeraTK::logic_error("DUMMY_INTERRUPT accessor register cannot have any offset");
    }

    flags.checkForUnknownFlags({});

    NDRegisterAccessor<UserType>::buffer_2D.resize(1);
    NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
    NDRegisterAccessor<UserType>::buffer_2D[0][0] = numericToUserType<UserType>(1);
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool DummyInterruptTriggerAccessor<UserType>::doWriteTransfer(ChimeraTK::VersionNumber) {
    _interruptTrigger();

    return false;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DummyInterruptTriggerAccessor<UserType>::doPreRead(TransferType) {
    if(not _backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(_backend->throwExceptionRead) {
      _backend->throwExceptionCounter++;
      throw ChimeraTK::runtime_error("DummyBackend: exception on read requested by user");
    }
    _backend->checkActiveException();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DummyInterruptTriggerAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(not hasNewData) return;
    NDRegisterAccessor<UserType>::buffer_2D[0][0] = numericToUserType<UserType>(1);
    TransferElement::_versionNumber = {};
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void DummyInterruptTriggerAccessor<UserType>::doPreWrite(TransferType, VersionNumber) {
    //
    if(not _backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(_backend->throwExceptionWrite) {
      _backend->throwExceptionCounter++;
      throw ChimeraTK::runtime_error("DummyBackend: exception on write requested by user");
    }

    _backend->checkActiveException();
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(DummyInterruptTriggerAccessor);
} // namespace ChimeraTK
