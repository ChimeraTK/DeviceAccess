#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

#include <functional>

namespace ChimeraTK {

  template<typename UserType>
  ExceptionHandlingDecorator<UserType>::ExceptionHandlingDecorator(
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> accessor, VariableNetworkNode networkNode)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(accessor), _direction(networkNode.getDirection()) {
    auto deviceAlias = networkNode.getDeviceAlias();
    auto registerName = networkNode.getRegisterName();

    assert(Application::getInstance().deviceModuleMap.count(deviceAlias) != 0);
    _deviceModule = Application::getInstance().deviceModuleMap[deviceAlias];

    // Consuming from the network means writing to the device what you consumed.
    if(_direction.dir == VariableDirection::consuming) {
      _deviceModule->writeRegisterPaths.push_back(registerName);

      // writable registers get a recoveryAccessor
      // Notice: There will be write-accessors without recovery accessors in future (intentionally turned off by the application programmer)
      // When this feature is added the VariableNetworkNode will get a new data member to indicate this.
      auto nElements = networkNode.getNumberOfElements();

      // The device in the deviceModule does not have a valid backend yet. It is set when open() is called, which has not happened yet.
      // We have to get the backend from the application.
      assert(Application::getInstance().deviceMap.count(deviceAlias) != 0);
      auto deviceBackend = Application::getInstance().deviceMap[deviceAlias];

      _recoveryAccessor = deviceBackend->getRegisterAccessor<UserType>(
          registerName, nElements, 0, {}); // recovery accessors don't have wait_for_new_data
      // version number and write order are still {nullptr} and 0 (i.e. invalid)
      _recoveryHelper = boost::make_shared<RecoveryHelper>(_recoveryAccessor, VersionNumber(nullptr), 0);
      _deviceModule->addRecoveryAccessor(_recoveryHelper);
    }
    else if(_direction.dir == VariableDirection::feeding) {
      _deviceModule->readRegisterPaths.push_back(registerName);
    }
    else {
      throw ChimeraTK::logic_error("Invalid variable direction in " + networkNode.getRegisterName());
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
      _hasThrownToInhibitTransfer = false;
      _hasThrownLogicError = false;
      _dataLostInPreviousWrite = false;
      auto recoverylock{_deviceModule->getRecoverySharedLock()};

      if(_recoveryAccessor != nullptr) {
        if(!_recoveryHelper->wasWritten && (_recoveryHelper->writeOrder != 0)) {
          _dataLostInPreviousWrite = true;
        }

        // Access to _recoveryAccessor is only possible channel-wise
        for(unsigned int ch = 0; ch < _recoveryAccessor->getNumberOfChannels(); ++ch) {
          _recoveryAccessor->accessChannel(ch) = buffer_2D[ch];
        }
        _recoveryHelper->versionNumber = versionNumber;
        _recoveryHelper->writeOrder = _deviceModule->writeOrder();
        _recoveryHelper->wasWritten = false;
      }
      else {
        _hasThrownLogicError = true;
        throw ChimeraTK::logic_error(
            "ChimeraTK::ExceptionhandlingDecorator: Calling write() on a non-writeable accessor is not supported ");
      }

      boost::shared_lock<boost::shared_mutex> errorLock(_deviceModule->errorMutex);
      if(_deviceModule->deviceHasError) {
        _hasThrownToInhibitTransfer = true;
        throw ChimeraTK::runtime_error(
            "ExceptionHandlingDecorator is preventing the write transfer."); // writing will be delayed, done by the recovery accessor
      }

      ++_deviceModule->synchronousTransferCounter; // must be under the lock

    } // lock guard goes out of scope

    // Now delegate call to the generic decorator, which swaps the buffer, without adding our exception handling with the generic transfer
    // preWrite and postWrite are only delegated if the transfer is allowed.
    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite(type, versionNumber);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite(TransferType type, VersionNumber versionNumber) {
    if(_hasThrownLogicError) {
      // preWrite has not been delegated, so there is nothing to do here. Let
      // postWrite() throw the active exception we have. Don't clear logic erros here.
      return;
    }
    if(!_hasThrownToInhibitTransfer) {
      {
        auto recoverylock{_deviceModule->getRecoverySharedLock()};
        _recoveryHelper->wasWritten = true; // the transfer was successful, so this data was written
      }                                     // end scope for recovery lock

      --_deviceModule->synchronousTransferCounter;
      try {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(type, versionNumber);
      }
      catch(ChimeraTK::runtime_error& e) {
        _deviceModule->reportException(e.what());
      }
    }
    else {
      // get rid of the active exception do the calling postWrite() does not re-throw
      _activeException = nullptr;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    bool hasException = false;
    // preRead has not been called when the transfer was not allowed. Don't call postRead in this case.
    if(!_hasThrownToInhibitTransfer) {
      try {
        if(!TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) { // was as synchronous transfer
          --_deviceModule->synchronousTransferCounter;
        }
        _target->setActiveException(this->_activeException);
        _target->postRead(type, hasNewData);
      }
      catch(ChimeraTK::runtime_error& e) {
        _deviceModule->reportException(e.what());
        hasException = true;
      }
    }
    else {
      _activeException = nullptr;
    }

    if(hasException || _hasThrownToInhibitTransfer) {
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
    _hasThrownToInhibitTransfer = false;

    if(TransferElement::_versionNumber == VersionNumber(nullptr)) {
      Application::testableModeUnlock("ExceptionHandling_doPreRead");
      _deviceModule->getInitialValueSharedLock();
      Application::testableModeLock("ExceptionHandling_doPreRead");
      // we don't have to store the shared lock. Once we acquired it the deviceModule will never take it again.
    }

    if(!TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) {
      boost::shared_lock<boost::shared_mutex> recoveryLock(_deviceModule->errorMutex);
      if(_deviceModule->deviceHasError) {
        _hasThrownToInhibitTransfer = true;
        throw ChimeraTK::runtime_error("ExceptionHandlingDecorator has thrown to skip read transfer");
      }

      ++_deviceModule->synchronousTransferCounter;
    }

    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(VersionNumber versionNumber) {
    bool transferLostData = _target->writeTransfer(versionNumber);
    return transferLostData || _dataLostInPreviousWrite;
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(VersionNumber versionNumber) {
    bool transferLostData = _target->writeTransferDestructively(versionNumber);
    return transferLostData || _dataLostInPreviousWrite;
  }

  template<typename Callable>
  bool genericWriteWrapper(Callable writeFunction, VersionNumber versionNumber);

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
