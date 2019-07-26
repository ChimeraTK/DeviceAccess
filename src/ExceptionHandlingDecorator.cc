#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::genericTransfer(std::function<bool(void)> callable) {
    while(true) {
      try {
        if(!dm.device.isOpened()) {
          setDataValidity(DataValidity::faulty);
          Application::getInstance().testableModeUnlock("waitForDeviceOpen");
          boost::this_thread::sleep(boost::posix_time::millisec(DeviceOpenTimeout));
          Application::getInstance().testableModeLock("waitForDeviceOpen");
          continue;
        }
        auto retval = callable();
        setDataValidity(DataValidity::ok);
        return retval;
      }
      catch(ChimeraTK::runtime_error& e) {
        setDataValidity(DataValidity::faulty);
        dm.reportException(e.what());
      }
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    return genericTransfer([this, versionNumber]() {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransfer(versionNumber);
    });
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
    return genericTransfer([this, versionNumber]() {
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransferDestructively(versionNumber);
    });
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doReadTransfer() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransfer(), true; });
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferNonBlocking() {
    return genericTransfer(
        [this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking(); });
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
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(), true; });
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite() {
    genericTransfer([this]() { return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(), true; });
  }

  template<typename UserType>
  DataValidity ExceptionHandlingDecorator<UserType>::dataValidity() const {
    // faulty Validity from the decorated class takes precedence over our own
    auto delegatedValidity = ChimeraTK::NDRegisterAccessorDecorator<UserType>::dataValidity();
    if(delegatedValidity == DataValidity::faulty) {
      return delegatedValidity;
    }

    return validity;
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setDataValidity(DataValidity newValidity) {
    // Remember ourselves, but also pass down the line
    if(newValidity != validity) {
      validity = newValidity;
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::setDataValidity(validity);
    }
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
