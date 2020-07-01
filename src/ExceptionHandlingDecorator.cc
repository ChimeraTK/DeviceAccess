#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

namespace ChimeraTK {

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod,
      VariableDirection direction, boost::shared_ptr<NDRegisterAccessor<UserType>> recoveryAccessor)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), deviceModule(devMod),
    _recoveryAccessor(recoveryAccessor), _direction(direction) {
    // Register recoveryAccessor at the DeviceModule
    if(recoveryAccessor != nullptr) {
      // version number is still {nullptr} (i.e. invalid)
      _recoveryHelper = boost::make_shared<RecoveryHelper>(recoveryAccessor, VersionNumber(nullptr));
      deviceModule.addRecoveryAccessor(_recoveryHelper);
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setOwnerValidity(bool hasExceptionNow) {
    if(hasExceptionNow != previousReadFailed) {
      previousReadFailed = hasExceptionNow;
      if(!_owner) return;
      if(hasExceptionNow) {
        _owner->incrementExceptionCounter();
      }
      else {
        _owner->decrementExceptionCounter();
      }
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    /* For writable accessors, copy data to the recoveryAcessor before perfroming the write.
     * Otherwise, the decorated accessor may have swapped the data out of the user buffer already.
     * This obtains a shared lock from the DeviceModule, hence, the regular writing happeniin here
     * can be performed in shared mode of the mutex and accessors are not blocking each other.
     * In case of recovery, the DeviceModule thread will take an exclusive lock so that this thread can not
     * modify the recoveryAcessor's user buffer while data is written to the device.
     */
    {
      auto lock{deviceModule.getRecoverySharedLock()};

      if(_recoveryAccessor != nullptr) {
        // Access to _recoveryAccessor is only possible channel-wise
        for(unsigned int ch = 0; ch < _recoveryAccessor->getNumberOfChannels(); ++ch) {
          _recoveryAccessor->accessChannel(ch) = buffer_2D[ch];
        }
        // FIXME: This should be the version number of the owner, which is not set at the moment
        _recoveryHelper->versionNumber = {};
      }
      else {
        throw ChimeraTK::logic_error(
            "ChimeraTK::ExceptionhandlingDecorator: Calling write() on a non-writeable accessor is not supported ");
      }
    } // lock guard goes out of scope

    // #138 Phase 1. Change for phase 2
    // Starting a transfer is only allowed if the status is running (not in initialisation or shutdown)
    bool transferAllowed = (Application::getInstance().getLifeCycleState() == LifeCycleState::run);
    // the waiting is only necessary as a hack for phase 1 because DeviceModule::startTransfer is not there yet
    if(transferAllowed) deviceModule.waitForRecovery();

    // Now delegate call to the generic decorator, which swaps the buffer, without adding our exception handling with the generic transfer
    // preWrite and postWrite are only delegated if the transfer is allowed.
    if(transferAllowed) {
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(type, versionNumber);
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite(TransferType type, VersionNumber versionNumber) {
    try {
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(type, versionNumber);
    }
    catch(ChimeraTK::runtime_error& e) {
      deviceModule.reportException(e.what());
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setOwner(EntityOwner* owner) {
    _owner = owner;
    if(_direction.dir == VariableDirection::feeding && previousReadFailed) {
      _owner->incrementExceptionCounter(false); // do not write. We are still in the setup phase.
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    bool hasException = false;
    try {
      this->_target->postRead(type, hasNewData);
    }
    catch(ChimeraTK::runtime_error& e) {
      deviceModule.reportException(e.what());
      hasException = true;
      // #138 Phase 1: Remove for phase 2
      // Inform the owner about the failed read (will be informed again after successful recovery
      setOwnerValidity(hasException);
    }
    // #138 Phase 2: change if codition here
    if(hasException) {
      // Try to recover and read until it succeeds.
      // We are already behind the delegated postRead, so the transfer in the target is already complemted.
      // So we have to use a complete blocking preRead, readTransfer, postRead, i.e. _tagret->read()
      while(true) {
        deviceModule.waitForRecovery();
        /* //#138 Phase 2
         * if (!deviceModule->startTransfer()) continue;
         * // end  of #138 Phase 2
         */
        try {
          this->_target->read();
          hasException = false;
          hasNewData = true; // if read() returns there is always new data
          /* //#138 Phase 2
           * deviceModule->stopTransfer());
           * // end  of #138 Phase 2
           */
          break;
        }
        catch(ChimeraTK::runtime_error&) {
          /* //#138 Phase 2
           * deviceModule->stopTransfer());
           * // end  of #138 Phase 2
           */
        }
      }
    }
    setOwnerValidity(hasException);
    // only replace the user buffer if there really is new data
    if(hasNewData) {
      for(size_t i = 0; i < buffer_2D.size(); ++i)
        buffer_2D[i].swap(this->_target->accessChannel(static_cast<unsigned int>(i)));
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead(TransferType type) {
    if(TransferElement::_versionNumber == VersionNumber(nullptr)) {
      Application::testableModeUnlock("ExceptionHandling_doPreRead");
      deviceModule.getInitialValueSharedLock();
      Application::testableModeLock("ExceptionHandling_doPreRead");
      // we don't have to store the shared lock. Once we acquired it the deviceModule will never take it again.
    }
    /* #138 Phase 1. Change this for phase 2 */
    /* Hack for phase 1 because DeviceModule::startTransfer is not there yet. */
    deviceModule.waitForRecovery();
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
