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
    initialValueMutex.lock();
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
    if(owner->isTestableModeEnabled()) {
      assert(owner->testableModeTestLock());
    }

    // The error queue must only be modified when holding both mutexes (error mutex and testable mode mutex), because
    // the testable mode counter must always be consistent with the content of the queue.
    // To avoid deadlocks you must always first aquire the testable mode mutex if you need both.
    // You can hold the error mutex without holding the testable mode mutex (for instance for checking the error predicate),
    // but then you must not try to aquire the testable mode mutex!
    boost::unique_lock<boost::shared_mutex> errorLock(errorMutex);

    if(!deviceHasError) { // only report new errors if the device does not have reported errors already
      if(errorQueue.push(errMsg)) {
        if(owner->isTestableModeEnabled()) ++owner->testableMode_counter;
      } // else do nothing. There are plenty of errors reported already: The queue is full.
      // set the error flag and notify the other threads
      deviceHasError = true;
      exceptionVersionNumber = {}; // generate a new exception version number
      errorLock.unlock();
    }
    else {
      errorLock.unlock();
    }
  }

  /*********************************************************************************************************************/

  VersionNumber DeviceModule::getExceptionVersionNumber() {
    boost::shared_lock<boost::shared_mutex> errorLock(errorMutex);
    return exceptionVersionNumber;
  }

  /*********************************************************************************************************************/

  void DeviceModule::handleException() {
    Application::registerThread("DM_" + getName());
    std::string error;
    owner->testableModeLock("Startup");

    // We have the testable mode lock. The device has not been initialised yet, but from now on the testableMode_deviceInitialisationCounter will take care or it
    testableModeReached = true;

    // flag whether the devices was opened+initialised for the first time
    bool firstSuccess = true;

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
          if(std::string(deviceError.message) != e.what()) {
            std::cerr << "Device " << deviceAliasOrURI << " reports error: " << e.what() << std::endl;
            // set proper error message in very first attempt to open the device
            deviceError.message = e.what();
            deviceError.setCurrentVersionNumber({});
            deviceError.message.write();
          }
          continue; // should not be necessary because isFunctional() should return false. But no harm in leaving it in.
        }
      } while(!device.isFunctional());
      owner->testableModeLock("Initialise device");

      boost::unique_lock<boost::shared_mutex> errorLock(errorMutex);

      // [Spec: 2.3.3] Empty exception reporting queue.
      while(errorQueue.pop()) {
        if(owner->isTestableModeEnabled()) {
          assert(owner->testableMode_counter > 0);
          --owner->testableMode_counter;
        }
      }
      errorLock.unlock(); // we don't need to hold the lock for now, but we will need it later

      for(auto& writeMe : writeRegisterPaths) {
        auto reg = device.getOneDRegisterAccessor<std::string>(writeMe); // the user data type does not matter here.
        if(!reg.isWriteable()) {
          throw ChimeraTK::logic_error(std::string(writeMe) + " is not writeable!");
        }
      }

      for(auto& readMe : readRegisterPaths) {
        auto reg = device.getOneDRegisterAccessor<std::string>(readMe); // the user data type does not matter here.
        if(!reg.isReadable()) {
          throw ChimeraTK::logic_error(std::string(readMe) + " is not readable!");
        }
      }

      // [Spec: 2.3.2] Run initialisation handlers
      try {
        for(auto& initHandler : initialisationHandlers) {
          initHandler(this);
        }
      }
      catch(ChimeraTK::runtime_error& e) {
        assert(deviceError.status != 0); // any error must already be reported...
        // update error message, since it might have been changed...
        if(std::string(deviceError.message) != e.what()) {
          std::cerr << "Device " << deviceAliasOrURI << " reports error: " << e.what() << std::endl;
          deviceError.message = e.what();
          deviceError.setCurrentVersionNumber({});
          deviceError.message.write();
        }
        // Jump back to re-opening the device
        continue;
      }

      // Write all recovery accessors
      // We are now entering the critical recovery section. It is protected by the recovery mutex until the deviceHasError flag has been cleared.
      boost::unique_lock<boost::shared_mutex> recoveryLock(recoveryMutex);
      try {
        // sort recovery helpers according to write order
        recoveryHelpers.sort([](boost::shared_ptr<RecoveryHelper>& a, boost::shared_ptr<RecoveryHelper>& b) {
          return a->writeOrder < b->writeOrder;
        });
        for(auto& recoveryHelper : recoveryHelpers) {
          if(recoveryHelper->versionNumber != VersionNumber{nullptr}) {
            recoveryHelper->accessor->write(recoveryHelper->versionNumber);
            recoveryHelper->wasWritten = true;
          }
        }
      }
      catch(ChimeraTK::runtime_error& e) {
        // update error message, since it might have been changed...
        if(std::string(deviceError.message) != e.what()) {
          std::cerr << "Device " << deviceAliasOrURI << " reports error: " << e.what() << std::endl;
          deviceError.message = e.what();
          deviceError.setCurrentVersionNumber({});
          deviceError.message.write();
        }
        // Jump back to re-opening the device
        continue;
      }

      errorLock.lock();
      deviceHasError = false;
      errorLock.unlock();

      recoveryLock.unlock();

      // send the trigger that the device is available again
      device.activateAsyncRead();
      if(isHoldingInitialValueMutex) {
        isHoldingInitialValueMutex = false;
        initialValueMutex.unlock();
      }

      // [Spec: 2.3.5] Reset exception state and wait for the next error to be reported.
      deviceError.status = 0;
      deviceError.message = "";
      deviceError.setCurrentVersionNumber({});
      deviceError.writeAll();
      deviceBecameFunctional.write();

      if(!firstSuccess) {
        std::cerr << "Device " << deviceAliasOrURI << " error cleared." << std::endl;
      }
      firstSuccess = false;

      // decrement special testable mode counter, was incremented manually above to make sure initialisation completes
      // within one "application step"
      if(Application::getInstance().testableMode) --owner->testableMode_deviceInitialisationCounter;

      // [Spec: 2.3.8] Wait for an exception being reported by the ExceptionHandlingDecorators
      // release the testable mode mutex for waiting for the exception.
      owner->testableModeUnlock("Wait for exception");

      // Do not modify the queue without holding the testable mode lock, because we also consistently have to modify
      // the counter protected by that mutex.
      // Just call wait(), not pop_wait().
      boost::this_thread::interruption_point();
      errorQueue.wait();
      boost::this_thread::interruption_point();

      owner->testableModeLock("Process exception");
      // increment special testable mode counter to make sure the initialisation completes within one
      // "application step"
      if(Application::getInstance().testableMode) {
        ++owner->testableMode_deviceInitialisationCounter; // matched above with a decrement
      }

      errorLock.lock(); // we need both locks to modify the queue

      auto popResult = errorQueue.pop(error);
      assert(popResult); // this if should always be true, otherwise the waiting did not work.
      (void)popResult;   // avoid warning in production build. g++5.4 does not support [[maybe_unused]] yet.
      if(owner->isTestableModeEnabled()) {
        assert(owner->testableMode_counter > 0);
        --owner->testableMode_counter;
      }

      errorLock.unlock(); // we must not hold the lock while waiting for the synchronousTransferCounter to go back to 0

      // [ExceptionHandling Spec: C.3.3.14] report exception to the control system
      std::cerr << "Device " << deviceAliasOrURI << " reports error: " << error << std::endl;
      deviceError.status = 1;
      deviceError.message = error;
      deviceError.setCurrentVersionNumber({});
      deviceError.writeAll();

      // [ExceptionHandling Spec: C.3.3.15] Wait for all synchronous transfers to finish before starting recovery.
      while(synchronousTransferCounter > 0) {
        usleep(1000);
      }

    } // while(true)
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
    if(Application::getInstance().testableMode) {
      ++owner->testableMode_deviceInitialisationCounter; // released and increased in handeException loop
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
      // put an exception into the waiting queue
      try {
        throw boost::thread_interrupted();
      }
      catch(boost::thread_interrupted&) {
        errorQueue.push_exception(std::current_exception());
      }
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

  void DeviceModule::addRecoveryAccessor(boost::shared_ptr<RecoveryHelper> recoveryAccessor) {
    recoveryHelpers.push_back(recoveryAccessor);
  }

  uint64_t DeviceModule::writeOrder() { return ++writeOrderCounter; }

  boost::shared_lock<boost::shared_mutex> DeviceModule::getRecoverySharedLock() {
    return boost::shared_lock<boost::shared_mutex>(recoveryMutex);
  }

  boost::shared_lock<boost::shared_mutex> DeviceModule::getInitialValueSharedLock() {
    return boost::shared_lock<boost::shared_mutex>(initialValueMutex);
  }

} // namespace ChimeraTK
