/*
 * DeviceModule.cc
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#include <ChimeraTK/DeviceBackend.h>
#include <ChimeraTK/Device.h>

#include "Application.h"
#include "DeviceModule.h"
//#include "ControlSystemModule.h"

namespace ChimeraTK {

  DeviceModule::DeviceModule(const std::string& _deviceAliasOrURI, const std::string& _registerNamePrefix)
  : Module(nullptr,
           _registerNamePrefix.empty() ? "<Device:"+_deviceAliasOrURI+">"
             : _registerNamePrefix.substr(_registerNamePrefix.find_last_of("/")+1),
           ""),
    deviceAliasOrURI(_deviceAliasOrURI),
    registerNamePrefix(_registerNamePrefix)
  {  
    
  }

  
  /*********************************************************************************************************************/
  
  DeviceModule::DeviceModule(Application *application, const std::string& _deviceAliasOrURI, const std::string& _registerNamePrefix)
  : Module(nullptr,
           _registerNamePrefix.empty() ? "<Device:"+_deviceAliasOrURI+">"
             : _registerNamePrefix.substr(_registerNamePrefix.find_last_of("/")+1),
           ""),
    deviceAliasOrURI(_deviceAliasOrURI),
    registerNamePrefix(_registerNamePrefix)
  {
    application->registerDeviceModule(this);
  }

  /*********************************************************************************************************************/

  DeviceModule::~DeviceModule() {
    assert(!moduleThread.joinable());
  }
  
  
  /*********************************************************************************************************************/

  VariableNetworkNode DeviceModule::operator()(const std::string& registerName, UpdateMode mode,
      const std::type_info &valueType, size_t nElements) const {
    return {registerName, deviceAliasOrURI, registerNamePrefix/registerName, mode, {VariableDirection::invalid, false}, valueType, nElements};
  }

  /*********************************************************************************************************************/

  Module& DeviceModule::operator[](const std::string& moduleName) const {
    if(subModules.count(moduleName) == 0) {
      subModules[moduleName] = {deviceAliasOrURI, registerNamePrefix/moduleName};
    }
    return subModules[moduleName];
  }

  /*********************************************************************************************************************/

  const Module& DeviceModule::virtualise() const {
    return *this;
  }

  /*********************************************************************************************************************/

  void DeviceModule::connectTo(const Module &target, VariableNetworkNode trigger) const {
    auto &cat = virtualiseFromCatalog();
    cat.connectTo(target, trigger);
  }

  /*********************************************************************************************************************/

  VirtualModule& DeviceModule::virtualiseFromCatalog() const {
    if(virtualisedModuleFromCatalog_isValid) return virtualisedModuleFromCatalog;

    virtualisedModuleFromCatalog = VirtualModule(deviceAliasOrURI, "Device module", ModuleType::Device);

    // obtain register catalogue
    Device d;
    d.open(deviceAliasOrURI);   /// @todo: do not actually open the device (needs extension of DeviceAccess)!
    auto catalog = d.getRegisterCatalogue();

    // iterate catalogue, create VariableNetworkNode for all registers starting with the registerNamePrefix
    size_t prefixLength = registerNamePrefix.length();
    for(auto &reg : catalog) {
      if(std::string(reg.getRegisterName()).substr(0,prefixLength) != std::string(registerNamePrefix)) continue;

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
      const std::type_info *valTyp{&typeid(AnyType)};
      auto &dd = reg.getDataDescriptor();  //numeric, string, boolean, nodata, undefined
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
        else {    // fractional
          valTyp = &typeid(double);
        }
      }
      else if(dd.fundamentalType() == RegisterInfo::FundamentalType::boolean) {
        valTyp = &typeid(int32_t);
      }
      else if(dd.fundamentalType() == RegisterInfo::FundamentalType::string) {
        valTyp = &typeid(std::string);
      }

      auto name = std::string(reg.getRegisterName()).substr(prefixLength);
      auto lastSlash = name.find_last_of("/");
      auto dirname = name.substr(0,lastSlash);
      auto basename = name.substr(lastSlash+1);
      VariableNetworkNode node(basename, deviceAliasOrURI, reg.getRegisterName(), updateMode,
                               direction, *valTyp, reg.getNumberOfElements());
      virtualisedModuleFromCatalog.createAndGetSubmoduleRecursive(dirname).addAccessor(node);

    }

    virtualisedModuleFromCatalog_isValid = true;
    return virtualisedModuleFromCatalog;
  }
  
  /*********************************************************************************************************************/

  void DeviceModule::reportException(std::string errMsg ){
    
    std::unique_lock<std::mutex> lk(errorMutex);
    errorQueue.push(errMsg);
    errorCondVar.wait(lk);
    lk.unlock();
    
  }
  
  /*********************************************************************************************************************/
  
  void DeviceModule::handleException(){
    Application::registerThread("DM_"+getName());
    std::string error;
    
    while(true)
    {
      
        errorQueue.pop_wait(error);
        //std::scoped_lock<std::mutex> lk(errorMutex);
        std::lock_guard<std::mutex> lk(errorMutex);
        deviceError.status = 1;
        deviceError.message = error;
        VersionNumber cV;
        deviceError.setCurrentVersionNumber(cV);
        deviceError.writeAll();

        if (error == "ExitOnNone")
          break;
        Device d;          
        while(true) 
        {
          try{
            d.open(deviceAliasOrURI);
            if (d.isOpened())
            {
              break;
            }
            boost::this_thread::interruption_point();
          }
          catch(std::exception ex)
          {
            deviceError.status = 1;
            deviceError.message = ex.what();
            deviceError.setCurrentVersionNumber(cV);
            deviceError.writeAll();
          }
          usleep(500000);
          boost::this_thread::interruption_point();
        }
        boost::this_thread::interruption_point();
        deviceError.status = 0;
        deviceError.message = "";
        deviceError.setCurrentVersionNumber(cV);
        deviceError.writeAll();
        errorCondVar.notify_all();
      }
      errorCondVar.notify_all();
  }
  
  /*********************************************************************************************************************/

  void DeviceModule::run() {

    // start the module thread
    assert(!moduleThread.joinable());
    moduleThread = boost::thread(&DeviceModule::handleException, this);
  }

/*********************************************************************************************************************/

  void DeviceModule::terminate() {
    
    reportException("ExitOnNone");
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      moduleThread.join();  
    }
    assert(!moduleThread.joinable());
  }


  void DeviceModule::defineConnections(){
    std::string prefix = "Devices."+deviceAliasOrURI+"/";
    ControlSystemModule cs(prefix);
    deviceError.connectTo(cs["DeviceError"]);
  }

}
