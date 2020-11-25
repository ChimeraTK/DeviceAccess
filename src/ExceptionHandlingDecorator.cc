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
      _inhibitWriteTransfer = false;
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
        _inhibitWriteTransfer = true;
        return;
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
    if(!_inhibitWriteTransfer) {
      --_deviceModule->synchronousTransferCounter;
      try {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite(type, versionNumber);
        {
          auto recoverylock{_deviceModule->getRecoverySharedLock()};
          // the transfer was successful or doPostRead did not throw and we reach this point,
          // so we matk these data as written
          _recoveryHelper->wasWritten = true;
        } // end scope for recovery lock
      }
      catch(ChimeraTK::runtime_error& e) {
        // Report exception to the exception backend. This would be done by the TransferElement base class only if we
        // would let the exception through, hence we have to take care of this here.
        this->_exceptionBackend->setException();
        // Report exception to the DeviceModule
        _deviceModule->reportException(std::string(e.what()) + " (seen by '" + _target->getName() + "')");
      }
    }
    assert(_activeException == nullptr);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    // preRead has not been called when the transfer was not allowed. Don't call postRead in this case.
    if(!_hasThrownToInhibitTransfer) {
      try {
        if(!TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) { // was as synchronous transfer
          --_deviceModule->synchronousTransferCounter;
        }
        _target->setActiveException(this->_activeException);
        _target->postRead(type, hasNewData);
        if(hasNewData) {
          // Reset the flag after a successful read.
          // It is only reset if there was new data. A readNonBlocking on a faulty device is not different to a readNonBlocking on working device: There just
          // is no new data. We only reset it on the next successful read with the initial value, otherwise keep the exception flag.
          _hasReportedException = false;
        }
      }
      catch(ChimeraTK::runtime_error& e) {
        // Report exception to the exception backend. This would be done by the TransferElement base class only if we
        // would let the exception through, hence we have to take care of this here.
        this->_exceptionBackend->setException();
        // Report exception to the DeviceModule
        _deviceModule->reportException(std::string(e.what()) + " (seen by '" + _target->getName() + "')");
        _hasReportedException = true;
      }
    }
    else {
      _activeException = nullptr;
    }

    if(_hasReportedException || _hasThrownToInhibitTransfer) {
      _dataValidity = DataValidity::faulty;
      // Note: This assertion does not hold
      // See discussion in https://github.com/ChimeraTK/DeviceAccess/pull/178
      // assert(_deviceModule->getExceptionVersionNumber() > _versionNumber);
      if(_deviceModule->getExceptionVersionNumber() > _versionNumber) {
        _versionNumber = _deviceModule->getExceptionVersionNumber();
      }
    }
    else {
      _dataValidity = _target->dataValidity();
      // Note: This assertion does not hold
      // See discussion in https://github.com/ChimeraTK/DeviceAccess/pull/178
      //assert(_target->getVersionNumber() >= _versionNumber);
      if(_target->getVersionNumber() > _versionNumber) {
        _versionNumber = _target->getVersionNumber();
      }
    }

    // only replace the user buffer if there really is new data
    if(hasNewData) {
      for(size_t i = 0; i < buffer_2D.size(); ++i)
        buffer_2D[i].swap(this->_target->accessChannel(static_cast<unsigned int>(i)));
    }
    assert(_activeException == nullptr);
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead(TransferType type) {
    _hasThrownToInhibitTransfer = false;

    if(TransferElement::_versionNumber == VersionNumber(nullptr)) {
      _deviceModule->getInitialValueSharedLock();
      // we don't have to store the shared lock. Once we acquired it the deviceModule will never take it again.
    }

    if(!TransferElement::_accessModeFlags.has(AccessMode::wait_for_new_data)) {
      boost::shared_lock<boost::shared_mutex> errorLock(_deviceModule->errorMutex);
      if(_deviceModule->deviceHasError) {
        _hasThrownToInhibitTransfer = true;
        throw ChimeraTK::runtime_error("ExceptionHandlingDecorator has thrown to skip read transfer");
      }
      // must hold the errorMutex while modifying the counter, and it must not be given up
      // after the decision to do the transfer until here
      ++_deviceModule->synchronousTransferCounter;
    }

    ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead(type);
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(VersionNumber versionNumber) {
    return genericWriteWrapper([&] { return _target->writeTransferDestructively(versionNumber); });
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(VersionNumber versionNumber) {
    return genericWriteWrapper([&] { return _target->writeTransferDestructively(versionNumber); });
  }

  template<typename UserType>
  template<typename Callable>
  bool ExceptionHandlingDecorator<UserType>::genericWriteWrapper(Callable writeFunction) {
    if(_inhibitWriteTransfer) {
      return _dataLostInPreviousWrite;
    }
    bool transferReportsPreviousDataLost = false;
    try {
      transferReportsPreviousDataLost = writeFunction();
    }
    catch(ChimeraTK::runtime_error&) {
      _activeException = std::current_exception();
    }
    return (transferReportsPreviousDataLost || _dataLostInPreviousWrite);
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
