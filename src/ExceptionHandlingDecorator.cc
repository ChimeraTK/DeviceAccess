#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setOwnerValidity(DataValidity newValidity) {
    if (newValidity != localValidity) {
      localValidity = newValidity;
      if (!_owner) return;
      if (newValidity == DataValidity::faulty) {
        _owner->incrementDataFaultCounter(true);
      } else {
        _owner->decrementDataFaultCounter();
      }
    }
  }
  
  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::genericTransfer(
    std::function<bool(void)> callable, bool updateOwnerValidityFlag) {
    
    std::function<void(DataValidity)> setOwnerValidityFunction{};
    if(updateOwnerValidityFlag) {
      setOwnerValidityFunction = std::bind(&ExceptionHandlingDecorator<UserType>::setOwnerValidity, this, std::placeholders::_1);
    } 
    else {
      setOwnerValidityFunction = [](DataValidity) {}; // do nothing if user does                                               // not want to invalidate data.
    }
    
    while(true) {
      try {
        if(!deviceModule.device.isOpened()) {
          setOwnerValidityFunction(DataValidity::faulty);
          Application::getInstance().testableModeUnlock("waitForDeviceOpen");
          boost::this_thread::sleep(boost::posix_time::millisec(DeviceOpenTimeout));
          Application::getInstance().testableModeLock("waitForDeviceOpen");
          continue;
        }
        auto retval = callable();
        auto delegatedValidity = ChimeraTK::NDRegisterAccessorDecorator<UserType>::dataValidity();
        setOwnerValidityFunction(delegatedValidity);
        return retval;
      }
      catch(ChimeraTK::runtime_error& e) {
        setOwnerValidityFunction(DataValidity::faulty);
        deviceModule.reportException(e.what());
      }
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    return genericTransfer(
        [this, versionNumber]() {
          return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransfer(versionNumber);
        },
        false);
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
    return genericTransfer(
        [this, versionNumber]() {
          return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransferDestructively(versionNumber);
        },
        false);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doReadTransfer() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransfer(), true; });
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferNonBlocking() {
    return genericTransfer( [this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking(); });
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferLatest() {
    return genericTransfer(
        [this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferLatest(); });
  }

  template<typename UserType>
  TransferFuture ExceptionHandlingDecorator<UserType>::doReadTransferAsync() {
    TransferFuture future;
    genericTransfer([this, &future]() {
      future = ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferAsync();
      return true;
    });

    return future;
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(), true; });
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostRead(), true; });
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreWrite() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(), true; }, false);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(), true; }, false);
  }

  template<typename UserType>
  DataValidity ExceptionHandlingDecorator<UserType>::dataValidity() const {
    // faulty Validity from the decorated class takes precedence over our own
    auto delegatedValidity = ChimeraTK::NDRegisterAccessorDecorator<UserType>::dataValidity();
    if(delegatedValidity == DataValidity::faulty) {
      return delegatedValidity;
    }

    return localValidity;
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
  }
  
  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
