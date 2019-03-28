/*!
 * \author Nadeem Shehzad (DESY)
 * \date 21.02.2019
 * \page excpetiondoc Exception Handling
 * \section Introduction
 *
 * To handle expection, the current simple implementation includes two error
 * state variables:
 *  - "state" (boolean flag if error occurred)
 *  - "message" (string with error message)
 *
 * These variables are automatically connected to the control systen in this
 * format:
 *  - /Devices/{AliasName}/message
 *  - /Devices/{AliasName}/status
 *
 * In this implementation a user/application can report an exception
 * by calling reportException of DeviceModule with an exception string.
 * The reportException packs the exception in a queue and the blocks the thread.
 * This queue is processed by an internal function handleException which
 * updates the DeviceError variables (status=1 and message= YourExceptionString)
 * and tries to open the device. Once device can be opened the DeviceError
 * variables are updated (status=0 and message="") and blocking threads
 * are notified to continue. It must be noted that whatever operation which
 * lead to exception e.g., read or write, should be repeated after the exception
 * is handled.
 *
 * Checkout testExceptionTest.cc under tests/executables_src to see how it
 * works.
 */

/*
 * DeviceModule.h
 *
 *  Created on: Jun 27, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_DEVICE_MODULE_H
#define CHIMERATK_DEVICE_MODULE_H

#include "ControlSystemModule.h"
#include "Module.h"
#include "ScalarAccessor.h"
#include "VariableGroup.h"
#include "VariableNetworkNode.h"
#include "VirtualModule.h"
#include <ChimeraTK/ForwardDeclarations.h>
#include <ChimeraTK/RegisterPath.h>

namespace ChimeraTK {
  class Application;
  class DeviceModule;

  /*********************************************************************************************************************/

  namespace detail {
    struct DeviceModuleProxy : Module {
      DeviceModuleProxy(const DeviceModule& owner, const std::string& registerNamePrefix);
      DeviceModuleProxy(DeviceModuleProxy&& other);
      DeviceModuleProxy() {}

      VariableNetworkNode operator()(const std::string& registerName, UpdateMode mode,
          const std::type_info& valueType = typeid(AnyType), size_t nElements = 0) const;
      VariableNetworkNode operator()(const std::string& registerName, const std::type_info& valueType,
          size_t nElements = 0, UpdateMode mode = UpdateMode::poll) const;
      VariableNetworkNode operator()(const std::string& variableName) const override;
      Module& operator[](const std::string& moduleName) const override;

      const Module& virtualise() const override;
      void connectTo(const Module& target, VariableNetworkNode trigger = {}) const override;
      ModuleType getModuleType() const override { return ModuleType::Device; }

      DeviceModuleProxy& operator=(DeviceModuleProxy&& other);

     private:
      friend class ChimeraTK::DeviceModule;
      const DeviceModule* _myowner;
      std::string _registerNamePrefix;
    };
  } // namespace detail

  /*********************************************************************************************************************/

  class DeviceModule : public Module {
   public:
    /** Constructor: The device represented by this DeviceModule is identified by
     * either the device alias found in the DMAP file or directly an URI. */
    DeviceModule(Application* application, const std::string& deviceAliasOrURI);

    /** Default constructor: create dysfunctional device module */
    DeviceModule() {}

    /** Destructor */
    virtual ~DeviceModule();

    /** Move operation with the move constructor */
    DeviceModule(DeviceModule&& other) { operator=(std::move(other)); }

    /** Move assignment */
    DeviceModule& operator=(DeviceModule&& other) {
      assert(!moduleThread.joinable());
      Module::operator=(std::move(other));
      deviceAliasOrURI = std::move(other.deviceAliasOrURI);
      registerNamePrefix = std::move(other.registerNamePrefix);
      deviceError = std::move(other.deviceError);
      owner = other.owner;
      proxies = std::move(other.proxies);
      for(auto& proxy : proxies) proxy.second._myowner = this;
      owner->registerDeviceModule(this);
      return *this;
    }

    /** The subscript operator returns a VariableNetworkNode which can be used in
     * the Application::initialise()
     *  function to connect the register with another variable. */
    VariableNetworkNode operator()(const std::string& registerName, UpdateMode mode,
        const std::type_info& valueType = typeid(AnyType), size_t nElements = 0) const;
    VariableNetworkNode operator()(const std::string& registerName, const std::type_info& valueType,
        size_t nElements = 0, UpdateMode mode = UpdateMode::poll) const {
      return operator()(registerName, mode, valueType, nElements);
    }
    VariableNetworkNode operator()(const std::string& variableName) const override {
      return operator()(variableName, UpdateMode::poll);
    }

    Module& operator[](const std::string& moduleName) const override;

    const Module& virtualise() const override;

    void connectTo(const Module& target, VariableNetworkNode trigger = {}) const override;

    ModuleType getModuleType() const override { return ModuleType::Device; }

    /** Use this function to report an exception. It should be called whenever a
     * ChimeraTK::runtime_error has been caught when trying to interact with this
     * device. This function shall not be called by the user, all exception
     *  handling is done internally by ApplicationCore. */
    void reportException(std::string errMsg);

    void run() override;

    void terminate() override;

    VersionNumber getCurrentVersionNumber() const override { return currentVersionNumber; }

    void setCurrentVersionNumber(VersionNumber versionNumber) override {
      if(versionNumber > currentVersionNumber) currentVersionNumber = versionNumber;
    }

    VersionNumber currentVersionNumber;
    /** This function connects DeviceError VariableGroup to ContolSystem*/
    void defineConnections() override;

   protected:
    // populate virtualisedModuleFromCatalog based on the information in the
    // device's catalogue
    VirtualModule& virtualiseFromCatalog() const;
    mutable VirtualModule virtualisedModuleFromCatalog{"INVALID", "", ModuleType::Invalid};
    mutable bool virtualisedModuleFromCatalog_isValid{false};

    std::string deviceAliasOrURI;
    ChimeraTK::RegisterPath registerNamePrefix;

    // List of proxies accessed through the operator[]. This is mutable since
    // it is little more than a cache and thus does not change the logical state
    // of this module
    mutable std::map<std::string, detail::DeviceModuleProxy> proxies;

    // create or return a proxy for a submodule (full hierarchy)
    detail::DeviceModuleProxy& getProxy(const std::string& fullName) const;

    /** A  VariableGroup for exception status and message. It can be protected, as
     * it is automatically connected to the control system in
     * DeviceModule::defineConnections() */
    struct DeviceError : public VariableGroup {
      using VariableGroup::VariableGroup;
      ScalarOutput<int> status{this, "status", "", ""};
      ScalarOutput<std::string> message{this, "message", "", ""};
    };
    DeviceError deviceError{this, "DeviceError", "Error status of the device"};

    /** The thread waiting for reportException(). It runs handleException() */
    boost::thread moduleThread;

    /** Queue used for communication between reportException() and the
     * moduleThread. */
    cppext::future_queue<std::string> errorQueue{5};

    /** Mutex for errorCondVar */
    std::mutex errorMutex;

    /** This condition variable is used to block reportException() until the error
     * state has been resolved by the moduleThread. */
    std::condition_variable errorCondVar;

    /** This functions tries to open the device and set the deviceError. Once done it notifies the waiting thread(s).
     *  The function is running an endless loop inside its own thread (moduleThread). */
    void handleException();

    Application* owner;

    friend class Application;
    friend class detail::DeviceModuleProxy;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_MODULE_H */
