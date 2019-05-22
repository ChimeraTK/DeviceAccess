#include "ExceptionHandlingDecorator.h"
#include "DeviceModule.h"

namespace ChimeraTK {

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransfer(versionNumber);
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doWriteTransferDestructively(versionNumber);
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doReadTransfer() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransfer();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferNonBlocking() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferNonBlocking();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  bool ExceptionHandlingDecorator<UserType>::doReadTransferLatest() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferLatest();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  TransferFuture ExceptionHandlingDecorator<UserType>::doReadTransferAsync() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      return ChimeraTK::NDRegisterAccessorDecorator<UserType>::doReadTransferAsync();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreRead() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreRead();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostRead() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostRead();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPreWrite() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPreWrite();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  template<typename UserType>
  void ExceptionHandlingDecorator<UserType>::doPostWrite() {
  retry:
    try {
      if(!dm.device.isOpened()) {
        Application::getInstance().testableModeUnlock("waitForDeviceOpen");
        usleep(500000);
        Application::getInstance().testableModeLock("waitForDeviceOpen");
        goto retry;
      }
      ChimeraTK::NDRegisterAccessorDecorator<UserType>::doPostWrite();
    }
    catch(ChimeraTK::runtime_error& e) {
      dm.reportException(e.what());
      goto retry;
    }
  }

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ExceptionHandlingDecorator);

} /* namespace ChimeraTK */
