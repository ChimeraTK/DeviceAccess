/*
 * DeviceModule.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

//#include <ChimeraTK/Device.h>
#include <ChimeraTK/DeviceBackend.h>

#include "Application.h"
#include "DeviceModule.h"
//#include "ControlSystemModule.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  namespace detail {
    DeviceModuleProxy::DeviceModuleProxy(const DeviceModule& owner, const std::string& registerNamePrefix)
    : Module(nullptr, registerNamePrefix.substr(registerNamePrefix.find_last_of("/") + 1), ""), _myowner(&owner),
      _registerNamePrefix(registerNamePrefix) {}

    DeviceModuleProxy::DeviceModuleProxy(DeviceModuleProxy&& other)
    : Module(std::move(other)), _myowner(std::move(other._myowner)),
      _registerNamePrefix(std::move(other._registerNamePrefix)) {}

    VariableNetworkNode DeviceModuleProxy::operator()(
        const std::string& registerName, UpdateMode mode, const std::type_info& valueType, size_t nElements) const {
      return (*_myowner)(_registerNamePrefix + "/" + registerName, mode, valueType, nElements);
    }

    VariableNetworkNode DeviceModuleProxy::operator()(
        const std::string& registerName, const std::type_info& valueType, size_t nElements, UpdateMode mode) const {
      return (*_myowner)(_registerNamePrefix + "/" + registerName, valueType, nElements, mode);
    }
    VariableNetworkNode DeviceModuleProxy::operator()(const std::string& variableName) const {
      return (*_myowner)(_registerNamePrefix + "/" + variableName);
    }

    Module& DeviceModuleProxy::operator[](const std::string& moduleName) const {
      return _myowner->getProxy(_registerNamePrefix + "/" + moduleName);
    }

    const Module& DeviceModuleProxy::virtualise() const {
      return _myowner->virtualise().submodule(_registerNamePrefix);
    }

    void DeviceModuleProxy::connectTo(const Module& target, VariableNetworkNode trigger) const {
      _myowner->virtualiseFromCatalog().submodule(_registerNamePrefix).connectTo(target, trigger);
    }

    DeviceModuleProxy& DeviceModuleProxy::operator=(DeviceModuleProxy&& other) {
      _name = std::move(other._name);
      _myowner = std::move(other._myowner);
      _registerNamePrefix = std::move(other._registerNamePrefix);
      return *this;
    }

  } // namespace detail

  /*********************************************************************************************************************/

  DeviceModule::DeviceModule(Application* application, const std::string& _deviceAliasOrURI,
      std::function<void(DeviceModule*)> initialisationHandler)
  : Module(nullptr, "<Device:" + _deviceAliasOrURI + ">", ""), deviceAliasOrURI(_deviceAliasOrURI),
    registerNamePrefix(""), owner(application) {
    application->registerDeviceModule(this);
    if(initialisationHandler) {
      initialisationHandlers.push_back(initialisationHandler);
    }
  }

  /*********************************************************************************************************************/

  DeviceModule::~DeviceModule() {
    assert(!moduleThread.joinable());
    owner->unregisterDeviceModule(this);
  }

  /*********************************************************************************************************************/

  VariableNetworkNode DeviceModule::operator()(
      const std::string& registerName, UpdateMode mode, const std::type_info& valueType, size_t nElements) const {
    return {registerName, deviceAliasOrURI, registerNamePrefix / registerName, mode,
        {VariableDirection::invalid, false}, valueType, nElements};
  }

  /*********************************************************************************************************************/

  Module& DeviceModule::operator[](const std::string& moduleName) const {
    assert(moduleName.find_first_of("/") == std::string::npos);
    return getProxy(moduleName);
  }

  /*********************************************************************************************************************/

  detail::DeviceModuleProxy& DeviceModule::getProxy(const std::string& fullName) const {
    if(proxies.find(fullName) == proxies.end()) {
      proxies[fullName] = {*this, fullName};
    }
    return proxies[fullName];
  }

  /*********************************************************************************************************************/

  const Module& DeviceModule::virtualise() const { return *this; }

  /*********************************************************************************************************************/

  void DeviceModule::connectTo(const Module& target, VariableNetworkNode trigger) const {
    auto& cat = virtualiseFromCatalog();
    cat.connectTo(target, trigger);
  }

  /*********************************************************************************************************************/

  VirtualModule& DeviceModule::virtualiseFromCatalog() const {
    if(virtualisedModuleFromCatalog_isValid) return virtualisedModuleFromCatalog;

    virtualisedModuleFromCatalog = VirtualModule(deviceAliasOrURI, "Device module", ModuleType::Device);

    if(!deviceIsInitialized) {
      device = Device(deviceAliasOrURI);
      deviceIsInitialized = true;
    }
    // obtain register catalogue
    auto catalog = device.getRegisterCatalogue();
    // iterate catalogue, create VariableNetworkNode for all registers starting
    // with the registerNamePrefix
    size_t prefixLength = registerNamePrefix.length();
    for(auto& reg : catalog) {
      if(std::string(reg.getRegisterName()).substr(0, prefixLength) != std::string(registerNamePrefix)) continue;

      // ignore 2D registers
      if(reg.getNumberOfDimensions() > 1) continue;

      // guess direction and determine update mode
      VariableDirection direction;
      UpdateMode updateMode;
      if(reg.isWriteable()) {
        direction = {VariableDirection::consuming, false};
        updateMode = UpdateMode::push;
      }
      else {
        direction = {VariableDirection::feeding, false};
        if(reg.getSupportedAccessModes().has(AccessMode::wait_for_new_data)) {
          updateMode = UpdateMode::push;
        }
        else {
          updateMode = UpdateMode::poll;
        }
      }

      // guess type
      const std::type_info* valTyp{&typeid(AnyType)};
      auto& dd = reg.getDataDescriptor(); // numeric, string, boolean, nodata, undefined
      if(dd.fundamentalType() == RegisterInfo::FundamentalType::numeric) {
        if(dd.isIntegral()) {
          if(dd.isSigned()) {
            if(dd.nDigits() > 11) {
              valTyp = &typeid(int64_t);
            }
            else if(dd.nDigits() > 6) {
              valTyp = &typeid(int32_t);
            }
            else if(dd.nDigits() > 4) {
              valTyp = &typeid(int16_t);
            }
            else {
              valTyp = &typeid(int8_t);
            }
          }
          else {
            if(dd.nDigits() > 10) {
              valTyp = &typeid(uint64_t);
            }
            else if(dd.nDigits() > 5) {
              valTyp = &typeid(uint32_t);
            }
            else if(dd.nDigits() > 3) {
              valTyp = &typeid(uint16_t);
            }
            else {
              valTyp = &typeid(uint8_t);
            }
          }
        }
        else { // fractional
          valTyp = &typeid(double);
        }
      }
      else if(dd.fundamentalType() == RegisterInfo::FundamentalType::boolean) {
        valTyp = &typeid(int32_t);
      }
      else if(dd.fundamentalType() == RegisterInfo::FundamentalType::string) {
        valTyp = &typeid(std::string);
      }
      else if(dd.fundamentalType() == RegisterInfo::FundamentalType::nodata) {
        valTyp = &typeid(int32_t);
      }

      auto name = std::string(reg.getRegisterName()).substr(prefixLength);
      auto lastSlash = name.find_last_of("/");
      auto dirname = name.substr(0, lastSlash);
      auto basename = name.substr(lastSlash + 1);
      VariableNetworkNode node(
          basename, deviceAliasOrURI, reg.getRegisterName(), updateMode, direction, *valTyp, reg.getNumberOfElements());
      virtualisedModuleFromCatalog.createAndGetSubmoduleRecursive(dirname).addAccessor(node);
    }

    virtualisedModuleFromCatalog_isValid = true;
    return virtualisedModuleFromCatalog;
  }

  /*********************************************************************************************************************/

  void DeviceModule::reportException(std::string errMsg) {
    try {
      if(owner->isTestableModeEnabled()) {
        assert(owner->testableModeTestLock());
      }

      // The error queue must only be modified when holding both mutexes (error mutex and testable mode mutex), because
      // the testable mode counter must always be consistent with the content of the queue.
      // To avoid deadlocks you must always first aquire the testable mode mutex if you need both.
      // You can hold the error mutex without holding the testable mode mutex (for instance for checking the error predicate),
      // but then you must not try to aquire the testable mode mutex!
      std::unique_lock<std::mutex> errorLock(errorMutex);

      if(!deviceHasError) { // only report new errors if the device does not have reported errors already
        if(errorQueue.push(errMsg)) {
          if(owner->isTestableModeEnabled()) ++owner->testableMode_counter;
        } // else do nothing. There are plenty of errors reported already: The queue is full.
        // set the error flag and notify the other threads
        deviceHasError = true;
        errorLock.unlock();
        errorIsReportedCondVar.notify_all();
      }
      else {
        errorLock.unlock();
      }

      //Wait until the error condition has been cleared by the exception handling thread.
      //We must not hold the testable mode mutex while doing so, otherwise the other thread will never run to fulfill the condition
      owner->testableModeUnlock("waitForDeviceOK");
      errorLock.lock();
      while(deviceHasError) {
        errorIsResolvedCondVar.wait(errorLock);
        boost::this_thread::interruption_point();
      }
      errorLock.unlock();
      owner->testableModeLock("continueAfterException");
    }
    catch(...) {
      //catch any to notify the waiting thread(s) (exception handling thread), then re-throw
      errorIsReportedCondVar.notify_all();
      throw;
    }
  }

  /*********************************************************************************************************************/

  void DeviceModule::handleException() {
    Application::registerThread("DM_" + getName());
    std::string error;
    owner->testableModeLock("Startup");
    try {
      while(!device.isOpened()) {
        try {
          boost::this_thread::interruption_point();
          usleep(500000);
          device.open();
          if(deviceError.status != 0) {
            deviceError.status = 0;
            deviceError.message = "";
            deviceError.setCurrentVersionNumber({});
            deviceError.writeAll();
          }
        }
        catch(ChimeraTK::runtime_error& e) {
          if(deviceError.status != 1) {
            deviceError.status = 1;
            deviceError.message = e.what();
            deviceError.setCurrentVersionNumber({});
            deviceError.writeAll();
          }
        }
      }
      // The device was successfully opened, try to initialise it
      try {
        for(auto& initHandler : initialisationHandlers) {
          initHandler(this);
        }
        for(auto& te : writeAfterOpen) {
          te->write();
        }
      }
      catch(ChimeraTK::runtime_error& e) {
        // we just report the exception and let the exception handling loop do the rest
        std::lock_guard<std::mutex> locakGuard(errorMutex);
        deviceHasError = true;
        if(errorQueue.push(e.what())) {
          if(owner->isTestableModeEnabled()) ++owner->testableMode_counter;
        }
      }

      while(true) {
        // We need one interruption point at the beginning of the loop. It is always executed and is not blocked by a lock (except the testable mode lock)
        boost::this_thread::interruption_point();
        // release the testable mode mutex for waiting for the exception.
        owner->testableModeUnlock("Wait for exception");
        // Do not modify the queue without holding the testable mode lock, because we also consitenly have to modify the counter protected by that mutex.
        // Just check the condition variable.
        std::unique_lock<std::mutex> errorLock(errorMutex);
        while(!deviceHasError) {
          boost::this_thread::interruption_point(); // Make sure not to start waiting for the condition variable if
                                                    // interruption was requested.
          errorIsReportedCondVar.wait(errorLock);   // this releases the mutex while waiting
          boost::this_thread::interruption_point(); // we need an interruption point in the waiting loop
        }

        errorLock.unlock(); // We must not hold the error lock when getting the testable mode mutex to avoid deadlocks!
        owner->testableModeLock("Process exception");
        errorLock.lock(); // we need both locks to modify the queue, so get it again.

        auto popResult = errorQueue.pop(error);
        (void)popResult;
        assert(popResult); // this if should always be true, otherwise the condition variable logic is wrong
        if(owner->isTestableModeEnabled()) {
          assert(owner->testableMode_counter > 0);
          --owner->testableMode_counter;
        }

        // report exception to the control system
        deviceError.status = 1;
        deviceError.message = error;
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();

        // wait some time until retrying.
        // Never sleep when holding a lock
        errorLock.unlock(); // we must not hold the error lock when not having the testable mode mutex
        owner->testableModeUnlock("Wait for recovery");

        do {
          usleep(500000);
          boost::this_thread::interruption_point();
          try {
            // This triggers a recovery (device is already opened).
            device.open();
          }
          catch(ChimeraTK::runtime_error&) {
            // Recovery failed. Just catch the exception and retry.
            continue; // should not be necessary because isFunctional() should return false. But no harm in leaving it in.
          }
        } while(!device.isFunctional());

        owner->testableModeLock("Try recovery");
        errorLock.lock();

        // FIXME: we have to wait here until the device reports that it is functional again.
        // Otherwise we spam the error reporting with 2 Hz.

        try {
          // re-initialise the device before continuing
          for(auto& initHandler : initialisationHandlers) {
            initHandler(this);
          }
          for(auto& te : writeAfterOpen) {
            te->write();
          }
        }
        catch(ChimeraTK::runtime_error& e) {
          // Report the error. This puts the exception to the queue and we can continue with waiting for the queue.
          // It is sure we will enter it again because we just pushed to it, so if  the device recovers we will notice.
          // Do not use reportError, which would just do the mutext business in addition to pushing to the queue, but we already have the mutex.
          if(errorQueue.push(e.what())) {
            if(owner->isTestableModeEnabled()) ++owner->testableMode_counter;
          }
          continue;
        }

        // We survived the initialisation (if any). It seems the device is working again.
        // Empty exception reporting queue.
        while(errorQueue.pop()) {
          if(owner->isTestableModeEnabled()) {
            assert(owner->testableMode_counter > 0);
            --owner->testableMode_counter;
          }
        }
        // Reset exception state and wait for the next error to be reported.
        deviceError.status = 0;
        deviceError.message = "";
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();

        deviceHasError = false;

        errorLock.unlock();
        errorIsResolvedCondVar.notify_all();
      }
    }
    catch(...) {
      // before we leave this thread, we might need to notify other waiting
      // threads. boost::this_thread::interruption_point() throws an exception
      // when the thread should be interrupted, so we will end up here
      errorIsResolvedCondVar.notify_all();
      throw;
    }
  }

  /*********************************************************************************************************************/

  void DeviceModule::prepare() {
    if(!deviceIsInitialized) {
      device = Device(deviceAliasOrURI);
      deviceIsInitialized = true;
    }
  }

  /*********************************************************************************************************************/

  /*********************************************************************************************************************/

  void DeviceModule::run() {
    // start the module thread
    assert(!moduleThread.joinable());
    moduleThread = boost::thread(&DeviceModule::handleException, this);
  }

  /*********************************************************************************************************************/

  void DeviceModule::terminate() {
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      errorIsReportedCondVar
          .notify_all(); // the terminating thread will notify the threads waiting for errorIsResolvedCondVar
      moduleThread.join();
    }
    assert(!moduleThread.joinable());
  }

  /*********************************************************************************************************************/

  void DeviceModule::defineConnections() {
    // replace all slashes in the deviceAliasOrURI, because URIs might contain slashes and they are not allowed in
    // module names
    std::string deviceAliasOrURI_withoutSlashes = deviceAliasOrURI;
    size_t i = 0;
    while((i = deviceAliasOrURI_withoutSlashes.find_first_of('/', i)) != std::string::npos) {
      deviceAliasOrURI_withoutSlashes[i] = '_';
    }

    // Connect deviceError module to the control system
    ControlSystemModule cs;
    deviceError.connectTo(cs["Devices"][deviceAliasOrURI_withoutSlashes]);
  }

  void DeviceModule::addInitialisationHandler(std::function<void(DeviceModule*)> initialisationHandler) {
    initialisationHandlers.push_back(initialisationHandler);
  }

  void DeviceModule::notify() { errorIsResolvedCondVar.notify_all(); }

} // namespace ChimeraTK
