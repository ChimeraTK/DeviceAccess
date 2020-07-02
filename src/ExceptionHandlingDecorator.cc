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
  void ExceptionHandlingDecorator<UserType>::doPreWrite(TransferType type, VersionNumber versionNumber) {
    /* For writable accessors, copy data to the recoveryAcessor before perfroming the write.
     * Otherwise, the decorated accessor may have swapped the data out of the user buffer already.
     * This obtains a shared lock from the DeviceModule, hence, the regular writing happeniin here
     * can be performed in shared mode of the mutex and accessors are not blocking each other.
     * In case of recovery, the DeviceModule thread will take an exclusive lock so that this thread can not
     * modify the recoveryAcessor's user buffer while data is written to the device.
     */
    {
      hasThrownToInhibitTransfer = false;
      auto recoverylock{deviceModule.getRecoverySharedLock()};

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

      boost::shared_lock<boost::shared_mutex> errorLock(deviceModule.errorMutex);
      if(deviceModule.deviceHasError) {
        hasThrownToInhibitTransfer = true;
        throw ChimeraTK::runtime_error(
            "ExceptionHandlingDecorator is preventing the write transfer."); // writing will be delayed, done by the recovery accessor
      }

      ++deviceModule.synchronousTransferCounter; // must be under the lock

    } // lock guard goes out of scope

    // Now delegate call to the generic decorator, which swaps the buffer, without adding our exception handling with the generic transfer
    // preWrite and postWrite are only delegated if the transfer is allowed.
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(type, versionNumber);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite(TransferType type, VersionNumber versionNumber) {
    if(!hasThrownToInhibitTransfer) {
      try {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(type, versionNumber);
      }
      catch(ChimeraTK::runtime_error& e) {
        deviceModule.reportException(e.what());
      }
    }
    else {
      // get rid of the active exception do the calling postWrite() does not re-throw
      _activeException = nullptr;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::setOwner(EntityOwner* owner) {
    _owner = owner;
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    bool hasException = false;
    // preRead has not been called when the transfer was not allowed. Don't call postRead in this case.
    if(!hasThrownToInhibitTransfer) {
      try {
        _target->setActiveException(this->_activeException);
        _target->postRead(type, hasNewData);
      }
      catch(ChimeraTK::runtime_error& e) {
        deviceModule.reportException(e.what());
        hasException = true;
      }
    }
    else {
      _activeException = nullptr;
    }

    if(hasException || hasThrownToInhibitTransfer) {
      _dataValidity = DataValidity::faulty;
      _versionNumber = {};
    }
    else {
      _dataValidity = _target->dataValidity();
      _versionNumber = _target->getVersionNumber();
    }

    // only replace the user buffer if there really is new data
    if(hasNewData) {
      for(size_t i = 0; i < buffer_2D.size(); ++i)
        buffer_2D[i].swap(this->_target->accessChannel(static_cast<unsigned int>(i)));
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead(TransferType type) {
    hasThrownToInhibitTransfer = false;

    if(TransferElement::_versionNumber == VersionNumber(nullptr)) {
      Application::testableModeUnlock("ExceptionHandling_doPreRead");
      deviceModule.getInitialValueSharedLock();
      Application::testableModeLock("ExceptionHandling_doPreRead");
      // we don't have to store the shared lock. Once we acquired it the deviceModule will never take it again.
    }

    if(!TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) {
      boost::shared_lock<boost::shared_mutex> recoveryLock(deviceModule.errorMutex);
      if(deviceModule.deviceHasError) {
        hasThrownToInhibitTransfer = true;
        throw ChimeraTK::runtime_error("ExceptionHandlingDecorator has thrown to skip read transfer");
      }

      ++deviceModule.synchronousTransferCounter;
    }

    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
