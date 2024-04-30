// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ExceptionDummyBackend.h"

namespace ChimeraTK {

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

  void ExceptionDummy::setExceptionImpl() noexcept {
    DummyBackend::setExceptionImpl();

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

  /********************************************************************************************************************/

  ExceptionDummy::ExceptionDummy(const std::string& mapFileName) : DummyBackend(mapFileName) {
    OVERRIDE_VIRTUAL_FUNCTION_TEMPLATE(getRegisterAccessor_impl);
  }

  /********************************************************************************************************************/

  boost::shared_ptr<DeviceBackend> ExceptionDummy::createInstance(
      // FIXME #11279 Implement API breaking changes from linter warnings
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      [[maybe_unused]] std::string address, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("No map file name given.");
    }
    return boost::shared_ptr<DeviceBackend>(new ExceptionDummy(parameters["map"]));
  }

  /********************************************************************************************************************/

  void ExceptionDummy::open() {
    if(throwExceptionOpen) {
      throwExceptionCounter++;
      setException("DummyException: open throws by request");
      throw ChimeraTK::runtime_error("DummyException: open throws by request");
    }
    ChimeraTK::DummyBackend::open();
  }

  /********************************************************************************************************************/

  void ExceptionDummy::closeImpl() {
    setException("Close ExceptionDummy");
    DummyBackend::closeImpl();
  }

  /********************************************************************************************************************/

  void ExceptionDummy::read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) {
    if(throwExceptionRead) {
      throwExceptionCounter++;
      throw ChimeraTK::runtime_error("DummyException: read throws by request");
    }
    ChimeraTK::DummyBackend::read(bar, address, data, sizeInBytes);
  }

  /********************************************************************************************************************/

  void ExceptionDummy::write(uint64_t bar, uint64_t address, int32_t const* data, size_t sizeInBytes) {
    if(throwExceptionWrite) {
      throwExceptionCounter++;
      throw ChimeraTK::runtime_error("DummyException: write throws by request");
    }
    ChimeraTK::DummyBackend::write(bar, address, data, sizeInBytes);

    // increment write counter and update write order (only if address points to beginning of a register!)
    auto itWriteOrder = _writeOrderMap.find(std::make_pair(bar, address));
    if(itWriteOrder != _writeOrderMap.end()) {
      // update write order
      auto generatedOrderNumber = ++_writeOrderCounter;
      auto& orderNumberInMap = itWriteOrder->second;
      // atomically update order number in the map only if the generated order number is bigger. This will be always
      // the case, unless there is a concurrent write operation updating the order number in between.
      size_t current;
      while((current = orderNumberInMap.load()) < generatedOrderNumber) {
        orderNumberInMap.compare_exchange_weak(current, generatedOrderNumber);
      }

      // increment write counter
      auto itWriteCounter = _writeCounterMap.find(std::make_pair(bar, address));
      assert(itWriteCounter != _writeCounterMap.end()); // always inserted together
      itWriteCounter->second++;
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> ExceptionDummy::getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    auto path = registerPathName;
    path.setAltSeparator(".");
    auto pathComponents = path.getComponents();
    bool pushRead = false;
    if(pathComponents[pathComponents.size() - 1] == "PUSH_READ") {
      if(flags.has(AccessMode::wait_for_new_data)) {
        pushRead = true;
        flags.remove(AccessMode::wait_for_new_data);
      }
      path--; // remove last component
    }

    auto acc = CALL_BASE_FUNCTION_TEMPLATE(
        getRegisterAccessor_impl, UserType, path, numberOfWords, wordOffsetInRegister, flags);
    if(pushRead) {
      std::unique_lock<std::mutex> lk(_pushDecoratorsMutex);

      auto decorator = boost::make_shared<ExceptionDummyPushDecorator<UserType>>(
          acc, boost::dynamic_pointer_cast<ExceptionDummy>(this->shared_from_this()));

      _pushDecorators[registerPathName].push_back(decorator);

      if(_activateNewPushAccessors) {
        decorator->_isActive = true;
        decorator->trigger(); // initial value
      }

      acc = decorator;
    }
    else if(acc->isReadable()) {
      // decorate all poll-type variable so returned validity of the data can be controlled
      auto decorator = boost::make_shared<ExceptionDummyPollDecorator<UserType>>(
          acc, boost::dynamic_pointer_cast<ExceptionDummy>(this->shared_from_this()));
      acc = decorator;
    }

    // create entry in _writeOrderMap and _writeCounterMap if necessary
    if(pathComponents[pathComponents.size() - 1] != "DUMMY_WRITEABLE" &&
        (pathComponents[0].find("DUMMY_INTERRUPT_") != 0)) {
      auto info = getRegisterInfo(path);
      auto adrPair = std::make_pair(info.bar, info.address);
      if(_writeOrderMap.find(adrPair) == _writeOrderMap.end()) {
        _writeOrderMap[adrPair] = 0;
        _writeCounterMap[adrPair] = 0;
      }
    }

    acc->setExceptionBackend(shared_from_this());

    return acc;
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
