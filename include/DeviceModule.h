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
#include "RecoveryHelper.h"
#include <ChimeraTK/ForwardDeclarations.h>
#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/Device.h>

namespace ChimeraTK {
  class Application;
  class DeviceModule;
  namespace history {
    struct ServerHistory;
  }

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

  /** Implementes access to a ChimeraTK::Device.
   */
  class DeviceModule : public Module {
   public:
    /** Constructor: The device represented by this DeviceModule is identified by
     * either the device alias found in the DMAP file or directly an URI.
     * A callback function to initialise the device can be registered as an optional argument (see addInitialisationHandler()
     * for more information).*/
    DeviceModule(Application* application, const std::string& deviceAliasOrURI,
        std::function<void(DeviceModule*)> initialisationHandler = nullptr);
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
      device = std::move(other.device);
      deviceAliasOrURI = std::move(other.deviceAliasOrURI);
      registerNamePrefix = std::move(other.registerNamePrefix);
      deviceError = std::move(other.deviceError);
      owner = other.owner;
      proxies = std::move(other.proxies);
      deviceHasError = other.deviceHasError;
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
     * device. It is primarily used by the ExceptionHandlingDecorator, but also user modules
     * can report exception and trigger the recovery mechanism like this. */
    void reportException(std::string errMsg);

    /** This functions is blocking until the device has been opened, initialsed and all recovery accessors
      * have been written. If the device is not in an error state, the function will return immediately. */
    void waitForRecovery();

    void prepare() override;

    void run() override;

    void terminate() override;

    /** Notify all condition variables that are waiting inside reportExeption(). This is
     *  called from other threads hosting accessors. You must call a boost::thread::terminate() on the
     *  thread running the accessor, then call DeviceModule::notify() to wake up reportException, which will detect the interruption and return.
     */
    void notify();

    VersionNumber getCurrentVersionNumber() const override { return currentVersionNumber; }

    void setCurrentVersionNumber(VersionNumber versionNumber) override {
      if(versionNumber > currentVersionNumber) currentVersionNumber = versionNumber;
    }

    VersionNumber currentVersionNumber{nullptr};

    /** This function connects DeviceError VariableGroup to ContolSystem*/
    void defineConnections() override;

    mutable Device device;

    DataValidity getDataValidity() const override { return DataValidity::ok; }
    void incrementDataFaultCounter() override {
      throw ChimeraTK::logic_error("incrementDataFaultCounter() called on a DeviceModule. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    void decrementDataFaultCounter() override {
      throw ChimeraTK::logic_error("decrementDataFaultCounter() called on a DeviceModule. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }

    void incrementExceptionCounter(bool /*writeAllOutputs*/) override {
      throw ChimeraTK::logic_error("incrementExceptionCounter() called on  a DeviceModule. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }
    void decrementExceptionCounter() override {
      throw ChimeraTK::logic_error("decrementExceptionCounter() called on  a DeviceModule. This is probably "
                                   "caused by incorrect ownership of variables/accessors or VariableGroups.");
    }

    /** Add initialisation handlers to the device.
     *
     *  Initialisation handlers are called after the device has been opened, or after the device is recovering
     *  from an error (i.e. an accessor has thrown an exception and Device::isFunctional() returns true afterwards).
     *
     *  You can add mupltiple handlers. They are executed in the sequence in which they are registered. If a handler
     *  has been registered in the constructor, it is called first.
     *
     *  The handler function is called from the DeviceModule thread (not from the thread with the accessor that threw the exception).
     *  It is handed a pointer to the instance of the DeviceModule
     *  where the handler was registered. The handler function may throw a ChimeraTK::runtime_error, so you don't have to
     *  catch errors thrown when accessing the Device inside the handler. After a handler has thrown an exception, the
     *  following handlers are not called. The DeviceModule will wait until the Device reports isFunctional() again and retry.
     *  The exception is reported to other modules and the control system.
     *
     *  Notice: Especially in network based devices which do not hold a permanent connection, it is not always possible
     *  to predict whether the next read()/write() will succeed. In this case the Device will always report isFunctional()
     *  and one just has to retry. In this case the DeviceModule will start the initialisation sequence every 500 ms.
     */
    void addInitialisationHandler(std::function<void(DeviceModule*)> initialisationHandler);

    /** A trigger that indicated that the device just became available again an error (in contrast to the
      *  error status which is also send when the device goes away).
      *  The output is public so your module can connect to it and trigger re-sending of variables that
      *  have to be send to the device again. e.g. after this has re-booted.
      *  Attention: It is not send the first time the device is being opened. In this case the normal startup
      *  mechanism takes care that the data is send.
      *  Like the deviceError, it is automatically published to the control systen to ensure that there is at least one
      *  consumer connected.
      */
    ScalarOutput<int> deviceBecameFunctional{
        this, "deviceBecameFunctional", "", ""}; // should be changed to data type void

    /** Add a TransferElement to the list DeviceModule::writeRecoveryOpen. This list will be written during a recovery,
     * after the constant accessors DeviceModule::writeAfterOpen are written. This is locked by a unique_lock.
     * You can get a shared_lock with getRecoverySharedLock(). */
    void addRecoveryAccessor(boost::shared_ptr<RecoveryHelper> recoveryAccessor);

    /** Returns a shared lock for the DeviceModule::recoverySharedMutex. This locks writing
     * the list DeviceModule::writeRecoveryOpen, during a recovery.*/
    boost::shared_lock<boost::shared_mutex> getRecoverySharedLock();

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

    /** Mutex for errorCondVar.
        Attention: In testable mode this mutex must only be locked when holding the testable mode mutex!*/
    std::mutex errorMutex;

    /** This condition variable is used to block reportException() until the error
     * state has been resolved by the moduleThread. */
    std::condition_variable errorIsResolvedCondVar;

    /** This condition variable is used to block the error handling thread until an exception is reported.*/
    std::condition_variable errorIsReportedCondVar;

    /** The error flag (predicate) for the conditionVariable */
    bool deviceHasError{true};

    /** This functions tries to open the device and set the deviceError. Once done it notifies the waiting thread(s).
     *  The function is running an endless loop inside its own thread (moduleThread). */
    void handleException();

    /** List of TransferElements to be written after the device has been recovered.
     *  See function addRecoveryAccessor() for details.*/
    std::list<boost::shared_ptr<RecoveryHelper>> recoveryHelpers;

    Application* owner{nullptr};

    mutable bool deviceIsInitialized = false;

    /* The list of initialisation handler callback functions */
    std::list<std::function<void(DeviceModule*)>> initialisationHandlers;

    /** Mutex for writing the DeviceModule::writeRecoveryOpen.*/
    boost::shared_mutex recoverySharedMutex;

    friend class Application;
    // Access to virtualiseFromCatalog() is needed by ServerHistory
    friend struct history::ServerHistory;
    // Access to virtualiseFromCatalog() is needed by MicroDAQ
    template<typename TRIGGERTYPE>
    friend class MicroDAQ;
    friend struct detail::DeviceModuleProxy;

    template<typename T>
    friend class ExceptionHandlingDecorator;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_DEVICE_MODULE_H */
