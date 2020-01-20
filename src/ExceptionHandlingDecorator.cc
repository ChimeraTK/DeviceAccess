#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), dm(devMod) {}

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod,
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> recoveryAccessor)
    : ExceptionHandlingDecorator<UserType>(accessor, devMod) {

    // Register recoveryAccessor at the DeviceModule
    if(recoveryAccessor != nullptr){
      _recoveryAccessor = recoveryAccessor;
      dm.addRecoveryAccessor(_recoveryAccessor);
    }
  }

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
    /* For writable accessors, copy data to the recoveryAcessor before perfroming the write.
     * Otherwise, the decorated accessor may have swapped the data out of the user buffer already.
     * This obtains a shared lock from the DeviceModule, hence, the regular writing happeniin here
     * can be performed in shared mode of the mutex and accessors are not blocking each other.
     * In case of recovery, the DeviceModule thread will take an exclusive lock so that this thread can not
     * modify the recoveryAcessor's user buffer while data is written to the device.
     */
    {
      auto lock{dm.getRecoverySharedLock()};

      if(_recoveryAccessor != nullptr){
        // Access to _recoveryAccessor is only possible channel-wise
        for(unsigned int ch=0; ch<_recoveryAccessor->getNumberOfChannels(); ++ch){
          _recoveryAccessor->accessChannel(ch) = buffer_2D[ch];
        }
      }
      else{
        throw ChimeraTK::logic_error("ChimeraTK::ExceptionhandlingDecorator: Calling write() on a non-writeable accessor is not supported ");
      }
    }
    // Now delegate call to the generic decorator, which swaps the buffer
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

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::interrupt() {
    // notify the condition variable waiting in reportException of the genericTransfer
    dm.notify();
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::interrupt();
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
