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

  }; // namespace detail

  /*********************************************************************************************************************/

  DeviceModule::DeviceModule(Application* application, const std::string& _deviceAliasOrURI)
  : Module(nullptr, "<Device:" + _deviceAliasOrURI + ">", ""), device(_deviceAliasOrURI),
    deviceAliasOrURI(_deviceAliasOrURI),registerNamePrefix(""), owner(application) {
    application->registerDeviceModule(this);
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
    // In testable mode, we need to increase the testableMode_counter before releasing the testableModeLock, but we have
    // to release the testableModeLock lock before obtaining the errorMutex, which in turn has to be obtained before
    // writing to the errorQueue. Otherwise deadlocks might occur (with race conditions).
    if(owner->isTestableModeEnabled()) {
      assert(owner->testableModeTestLock());
      ++owner->testableMode_counter;
    }
    owner->testableModeUnlock("reportException");
    std::unique_lock<std::mutex> lk(errorMutex);
  retry:
    bool success = errorQueue.push(errMsg);
    if(!success) {
      // retry error reporting until the message has been sent. It has to be successful since testableMode_counter
      // has been increased already
      goto retry;
    }
    errorCondVar.wait(lk);
    // release errorMutex before obtaining testableModeLock to prevent deadlocks
    lk.unlock();
    owner->testableModeLock("reportException");
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
          if (deviceError.status != 0){
            deviceError.status = 0;
            deviceError.message = "";
            deviceError.setCurrentVersionNumber({});
            deviceError.writeAll();
          }
        }
        catch(ChimeraTK::runtime_error& e) {
          if (deviceError.status != 1){
            deviceError.status = 1;
            deviceError.message = error;
            deviceError.setCurrentVersionNumber({});
            deviceError.writeAll();
          }
        }
      }
      while(true) {
        owner->testableModeUnlock("Wait for exception");
        errorQueue.pop_wait(error);
        boost::this_thread::interruption_point();
        owner->testableModeLock("Process exception");
        if(owner->isTestableModeEnabled()) --owner->testableMode_counter;
        std::lock_guard<std::mutex> lk(errorMutex);
        // report exception to the control system
        deviceError.status = 1;
        deviceError.message = error;
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();

        // wait some time until retrying
        owner->testableModeUnlock("Wait for recovery");
        usleep(500000);
        owner->testableModeLock("Try recovery");

        // empty exception reporting queue
        while(errorQueue.pop()) {
          if(owner->isTestableModeEnabled()) --owner->testableMode_counter;
        }

        // reset exception state and try again
        deviceError.status = 0;
        deviceError.message = "";
        deviceError.setCurrentVersionNumber({});
        deviceError.writeAll();
        errorCondVar.notify_all();
      }
    }
    catch(...) {
      // before we leave this thread, we might need to notify other waiting
      // threads. boost::this_thread::interruption_point() throws an exception
      // when the thread should be interrupted, so we will end up here
      errorCondVar.notify_all();
      throw;
    }
  }

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
      errorQueue.push("terminate");
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

} // namespace ChimeraTK
