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
      std::string dirname, basename;
      if(lastSlash != std::string::npos) {
        dirname = name.substr(0, lastSlash);
        basename = name.substr(lastSlash + 1);
      }
      else {
        dirname = "";
        basename = name;
      }
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
    }
    catch(...) {
      //catch any to notify the waiting thread(s) (exception handling thread), then re-throw
      errorIsReportedCondVar.notify_all();
      throw;
    }
  }

  /*********************************************************************************************************************/

  void DeviceModule::waitForRecovery() {
    //Wait until the error condition has been cleared by the exception handling thread.
    //We must not hold the testable mode mutex while doing so, otherwise the other thread will never run to fulfill the condition
    owner->testableModeUnlock("waitForDeviceOK");
    {
      std::unique_lock<std::mutex> errorLock(errorMutex);
      while(deviceHasError) {
        errorIsResolvedCondVar.wait(errorLock);
        boost::this_thread::interruption_point();
      }
    } // end scope of error lock. We must not hold it when trying to get the testable mode lock.
    owner->testableModeLock("continueAfterException");
  }

  /*********************************************************************************************************************/

  void DeviceModule::handleException() {
    Application::registerThread("DM_" + getName());
    std::string error;
    owner->testableModeLock("Startup");

    // flag whether the deviceError.message has already been initialised with a sensible value
    bool firstAttempt = true;

    // catch-all required around everything, to prevent any uncaught exception (including the boost thread interruption)
    // to terminate this thread before waking up other threads waiting on the condition variable.
    try {
      while(true) {
        // [Spec: 2.3.1] (Re)-open the device.
        owner->testableModeUnlock("Open/recover device");
        do {
          usleep(500000);
          boost::this_thread::interruption_point();
          try {
            device.open();
          }
          catch(ChimeraTK::runtime_error& e) {
            assert(deviceError.status != 0); // any error must already be reported...
            if(firstAttempt) {
              // set proper error message in very first attempt to open the device
              deviceError.message = e.what();
              deviceError.setCurrentVersionNumber({});
              deviceError.message.write();
              firstAttempt = false;
            }
            continue; // should not be necessary because isFunctional() should return false. But no harm in leaving it in.
          }
        } while(!device.isFunctional());
        firstAttempt = false;
        owner->testableModeLock("Initialise device");

        std::unique_lock<std::mutex> errorLock(errorMutex);

        // [Spec: 2.3.2] Run initialisation handlers
        try {
          for(auto& initHandler : initialisationHandlers) {
            initHandler(this);
          }
        }
        catch(ChimeraTK::runtime_error& e) {
          assert(deviceError.status != 0); // any error must already be reported...
          // update error message, since it might have been changed...
          deviceError.message = e.what();
          deviceError.setCurrentVersionNumber({});
          deviceError.message.write();
          // Jump back to re-opening the device
          continue;
        }

        // [Spec: 2.3.3] Empty exception reporting queue.
        while(errorQueue.pop()) {
          if(owner->isTestableModeEnabled()) {
            assert(owner->testableMode_counter > 0);
            --owner->testableMode_counter;
          }
        }

        // [Spec: 2.3.4] Write all recovery accessors
        try {
          boost::unique_lock<boost::shared_mutex> uniqueLock(recoverySharedMutex);
          for(auto& recoveryHelper : recoveryHelpers) {
            if(recoveryHelper->versionNumber != VersionNumber{nullptr}) {
              recoveryHelper->accessor->write(recoveryHelper->versionNumber);
            }
          }
        }
        catch(ChimeraTK::runtime_error& e) {
          // update error message, since it might have been changed...
          deviceError.message = e.what();
          deviceError.setCurrentVersionNumber({});
          deviceError.message.write();
          // Jump back to re-opening the device
          continue;
        }

        // [Spec: 2.3.5] Reset exception state and wait for the next error to be reported.
        deviceError.status = 0;
        deviceError.message = "";
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();

        deviceHasError = false;
        // decrement special testable mode counter, was incremented manually above to make sure initialisation completes
        // within one "application step"
        --owner->testableMode_deviceInitialisationCounter;

        // [Spec: 2.3.6+2.3.7] send the trigger that the device is available again
        deviceBecameFunctional.write();
        errorLock.unlock();
        errorIsResolvedCondVar.notify_all();

        // [Spec: 2.3.8] Wait for an exception being reported by the ExceptionHandlingDecorators
        // release the testable mode mutex for waiting for the exception.
        owner->testableModeUnlock("Wait for exception");
        // Do not modify the queue without holding the testable mode lock, because we also consistently have to modify
        // the counter protected by that mutex.
        // Just check the condition variable.
        errorLock.lock();
        while(!deviceHasError) {
          boost::this_thread::interruption_point(); // Make sure not to start waiting for the condition variable if
                                                    // interruption was requested.
          errorIsReportedCondVar.wait(errorLock);   // this releases the mutex while waiting
          boost::this_thread::interruption_point(); // we need an interruption point in the waiting loop
        }

        errorLock.unlock(); // We must not hold the error lock when getting the testable mode mutex to avoid deadlocks!
        owner->testableModeLock("Process exception");
        // increment special testable mode counter to make sure the initialisation completes within one
        // "application step"
        ++owner->testableMode_deviceInitialisationCounter; // matched above with a decrement

        errorLock.lock(); // we need both locks to modify the queue, so get it again.

        auto popResult = errorQueue.pop(error);
        (void)popResult;
        assert(popResult); // this if should always be true, otherwise the condition variable logic is wrong
        if(owner->isTestableModeEnabled()) {
          assert(owner->testableMode_counter > 0);
          --owner->testableMode_counter;
        }

        // [Spec: 2.6.1] report exception to the control system
        deviceError.status = 1;
        deviceError.message = error;
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();

        // TODO Implementation for Spec 2.6.2 goes here.

      } // while(true)
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

    // Set initial status to error
    deviceError.status = 1;
    deviceError.message = "Attempting to open device...";
    deviceError.setCurrentVersionNumber({});
    deviceError.writeAll();

    // Increment special testable mode counter to make sure the initialisation completes within one
    // "application step". Start with counter increased (device not initialised yet, wait).
    // We can to this here without testable mode lock because the application is still single threaded.
    ++owner->testableMode_deviceInitialisationCounter; // released and increased in handeException loop
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
    deviceBecameFunctional >> cs["Devices"][deviceAliasOrURI_withoutSlashes]("deviceBecameFunctional");
  }

  void DeviceModule::addInitialisationHandler(std::function<void(DeviceModule*)> initialisationHandler) {
    initialisationHandlers.push_back(initialisationHandler);
  }

  void DeviceModule::notify() { errorIsResolvedCondVar.notify_all(); }

  void DeviceModule::addRecoveryAccessor(boost::shared_ptr<RecoveryHelper> recoveryAccessor) {
    recoveryHelpers.push_back(recoveryAccessor);
  }

  boost::shared_lock<boost::shared_mutex> DeviceModule::getRecoverySharedLock() {
    return boost::shared_lock<boost::shared_mutex>(recoverySharedMutex);
  }

} // namespace ChimeraTK
