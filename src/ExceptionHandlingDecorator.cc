#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

constexpr useconds_t DeviceOpenTimeout = 500;

namespace ChimeraTK {

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, DeviceModule& devMod,
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> recoveryAccessor)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), deviceModule(devMod) {
    // Register recoveryAccessor at the DeviceModule
    if(recoveryAccessor != nullptr) {
      _recoveryAccessor = recoveryAccessor;
      deviceModule.addRecoveryAccessor(_recoveryAccessor);
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setOwnerValidity(DataValidity newValidity) {
    if(newValidity != localValidity) {
      localValidity = newValidity;
      if(!_owner) return;
      if(newValidity == DataValidity::faulty) {
        _owner->incrementExceptionCounter();
      }
      else {
        _owner->decrementExceptionCounter();
      }
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::genericTransfer(
      std::function<bool(void)> callable, bool updateOwnerValidityFlag) {
    std::function<void(DataValidity)> setOwnerValidityFunction{};
    if(updateOwnerValidityFlag) {
      setOwnerValidityFunction =
          std::bind(&ExceptionHandlingDecorator<UserType>::setOwnerValidity, this, std::placeholders::_1);
    }
    else {
      setOwnerValidityFunction = [](DataValidity) {
      }; // do nothing if user does                                               // not want to invalidate data.
    }

    while(true) {
      try {
        if(!deviceModule.device.isOpened()) {
          if(Application::getInstance().getLifeCycleState() != LifeCycleState::run) {
            // If the application has not yet fully started, we cannot wait for the device to open. Instead register
            // the variable in the DeviceMoule, so the transfer will be performed after the device is opened.
            assert(_recoveryAccessor != nullptr); // should always be true for writeable registers with this decorator
            // Note: it's ok to use the recoveryAccessor here as well, since device opening and recovery happens in the
            // same thread in the DeviceModule.
            deviceModule.writeAfterOpen.push_back(this->_recoveryAccessor);
            return false;
          }
          // We artificially increase the testabel mode counter so the test does not slip out of testable mode here in case
          // the queues are empty. At the moment, the exception handling has to be done before you get the lock back in your test.
          // Exception handling cannot be tested in testable mode at the moment.
          if(Application::getInstance().isTestableModeEnabled()) {
            ++Application::getInstance().testableMode_counter;
            Application::getInstance().testableModeUnlock("waitForDeviceOpen");
          }
          boost::this_thread::sleep(boost::posix_time::millisec(DeviceOpenTimeout));
          if(Application::getInstance().isTestableModeEnabled()) {
            Application::getInstance().testableModeLock("waitForDeviceOpen");
            --Application::getInstance().testableMode_counter;
          }
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
  void ExceptionHandlingDecorator<UserType>::doPreWrite() {
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
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite();
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
