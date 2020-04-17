#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod,
      VariableDirection direction, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> recoveryAccessor)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), deviceModule(devMod), _direction(direction) {
    // Register recoveryAccessor at the DeviceModule
    if(recoveryAccessor != nullptr) {
      _recoveryAccessor = recoveryAccessor;
      deviceModule.addRecoveryAccessor(_recoveryAccessor);
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
  void ExceptionHandlingDecorator<UserType>::doGenericPostAction(
      std::function<void(void)> callable, bool updateOwnerValidityFlag) {
    std::function<void(bool)> setOwnerValidityFunction{};
    if(updateOwnerValidityFlag) {
      setOwnerValidityFunction =
          std::bind(&ExceptionHandlingDecorator<UserType>::setOwnerValidity, this, std::placeholders::_1);
    }
    else {
      setOwnerValidityFunction = [](bool) {}; // do nothing if calling code does not want to invalidate data.
    }

    try {
      callable();
      // We do not have to relay the target's data validity. The MetaDataPropagatingDecorator already takes care of it.
      // The ExceptionHandling decorator is used in addition, not instead of it.
      setOwnerValidityFunction(/*hasExceptionNow = */ false);
    }
    catch(ChimeraTK::runtime_error& e) {
      setOwnerValidityFunction(/*hasExceptionNow = */ true);
      deviceModule.reportException(e.what());
      deviceModule.waitForRecovery();
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    if(transferAllowed) {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransfer(versionNumber);
    }
    else {
      return true; /* data loss */
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
    if(transferAllowed) {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransferDestructively(versionNumber);
    }
    else {
      return true; /* data loss */
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doReadTransfer() {
    if(transferAllowed) {
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransfer();
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferNonBlocking() {
    if(transferAllowed) {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking();
    }
    else {
      return false; //hasNewData
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferLatest() {
    if(transferAllowed) {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferLatest();
    }
    else {
      return false; //hasNewData
    }
  }

  template<typename UserType>
  TransferFuture ExceptionHandlingDecorator<UserType>::doReadTransferAsync() {
    assert(transferAllowed);
    if(!transferAllowed) {
      throw(ChimeraTK::logic_error("You have to implement ApplicationCore #157"));
    }
    return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferAsync();
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreWrite(TransferType type) {
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
      }
      else {
        throw ChimeraTK::logic_error(
            "ChimeraTK::ExceptionhandlingDecorator: Calling write() on a non-writeable accessor is not supported ");
      }
    } // lock guard goes out of scope

    // Now delegate call to the generic decorator, which swaps the buffer, without adding our exception handling with the generic transfer
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(type);

    // #138 Phase 1. Change for phase 2
    transferAllowed = (Application::getInstance().getLifeCycleState() == LifeCycleState::run);
    // the waiting is only necessary as a hack for phase 1 because DeviceModule::startTransfer is not there yet
    if(transferAllowed) deviceModule.waitForRecovery();
  }

  template<typename UserType>
  DataValidity ExceptionHandlingDecorator<UserType>::dataValidity() const {
    // If there has been an exception  the data cannot be OK.
    // This is only considered in read mode (=feeding to the connected variable network).
    if(_direction.dir == VariableDirection::feeding && previousReadFailed) {
      return DataValidity::faulty;
    }
    // no exception, return the data validity of the accessor we are decorating
    auto delegatedValidity = ChimeraTK::NDRegisterAccessorDecorator<UserType>::dataValidity();
    return delegatedValidity;
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::interrupt() {
    // notify the condition variable waiting in reportException of the genericTransfer
    deviceModule.notify();
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::interrupt();
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
  void ExceptionHandlingDecorator<UserType>::doPostWrite(TransferType type, bool dataLost) {
    doGenericPostAction(
        [this, type, dataLost]() { ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(type, dataLost); },
        false);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead(TransferType type) {
    /* #138 Phase 1. Change this for phase 2 */
    transferAllowed = (Application::getInstance().getLifeCycleState() == LifeCycleState::run);
    /* Hack for phase 1 because DeviceModule::startTransfer is not there yet. Remove for phase 2 */
    assert(transferAllowed); // not true in phase 2 any more
    deviceModule.waitForRecovery();
    /* End of remove for phase 2 */

    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
