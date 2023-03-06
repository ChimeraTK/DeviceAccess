// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ExceptionDummyBackend.h"

#include "BackendFactory.h"

namespace ChimeraTK {

  ExceptionDummy::BackendRegisterer ExceptionDummy::backendRegisterer;

  /********************************************************************************************************************/

  ExceptionDummy::BackendRegisterer::BackendRegisterer() {
    std::cout << "ExceptionDummy::BackendRegisterer: registering backend type ExceptionDummy" << std::endl;
    ChimeraTK::BackendFactory::getInstance().registerBackendType("ExceptionDummy", &ExceptionDummy::createInstance);
  }

  /********************************************************************************************************************/

  /// Function to trigger sending values for push-type variables
  void ExceptionDummy::triggerPush(RegisterPath path, VersionNumber v) {
    path.setAltSeparator(".");
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    _pushVersions[path] = v;
    for(auto& acc_weak : _pushDecorators[path]) {
      auto acc = acc_weak.lock();
      if(!acc->_isActive) continue;
      lk.unlock(); // next line might call setException()...
      acc->trigger();
      lk.lock();
    }
  }

  /********************************************************************************************************************/

  void ExceptionDummy::activateAsyncRead() noexcept {
    DummyBackend::activateAsyncRead();

    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    for(auto& pair : _pushDecorators) {
      _pushVersions[pair.first] = {};
      for(auto& acc_weak : pair.second) {
        auto acc = acc_weak.lock();
        if(acc->_isActive) continue;
        lk.unlock();    // next line might call setException()...
        acc->trigger(); // initial value
        lk.lock();
        acc->_isActive = true;
        acc->_hasException = false;
      }
    }
    _activateNewPushAccessors = true;
  }

  /********************************************************************************************************************/

  void ExceptionDummy::setException() {
    DummyBackend::setException();

    // deactivate async transfers
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    for(auto& pair : _pushDecorators) {
      _pushVersions[pair.first] = {};
      for(auto& acc_weak : pair.second) {
        auto acc = acc_weak.lock();
        if(!acc->_isActive) continue;
        acc->_isActive = false;
        if(acc->_hasException) continue;
        acc->_hasException = true;
        lk.unlock(); // next line might call setException()...
        acc->trigger();
        lk.lock();
      }
    }
    _activateNewPushAccessors = false;
  }

  /********************************************************************************************************************/

  size_t ExceptionDummy::getWriteOrder(const RegisterPath& path) {
    auto info = getRegisterInfo(path);
    auto adrPair = std::make_pair(info.bar, info.address);
    return _writeOrderMap.at(adrPair);
  }

  /********************************************************************************************************************/

  size_t ExceptionDummy::getWriteCount(const RegisterPath& path) {
    auto info = getRegisterInfo(path);
    auto adrPair = std::make_pair(info.bar, info.address);
    return _writeCounterMap.at(adrPair);
  }

  /********************************************************************************************************************/

  bool ExceptionDummy::asyncReadActivated() {
    std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);
    return _activateNewPushAccessors;
  }

} // namespace ChimeraTK
