/*
 * TestFacility.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_FACILITY
#define CHIMERATK_TEST_FACILITY

#include <boost/fusion/include/at_key.hpp>

#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
#include <ChimeraTK/ScalarRegisterAccessor.h>

#include "Application.h"
#include "DeviceModule.h"
#include "TestableModeAccessorDecorator.h"

namespace ChimeraTK {

  /** Helper class to facilitate tests of applications based on ApplicationCore */
  class TestFacility {
   public:
    /** The constructor will internally obtain the instance of the application, so
     * the instance of the TestFacility must not be created before the application
     * (i.e. usually not before the main() routine). The application will
     *  automatically be put into the testable mode and initialised. */
    explicit TestFacility(bool enableTestableMode = true) {
      auto pvManagers = createPVManager();
      pvManager = pvManagers.first;
      Application::getInstance().setPVManager(pvManagers.second);
      if(enableTestableMode) Application::getInstance().enableTestableMode();
      Application::getInstance().initialise();
    }

    /** Start the application in testable mode. */
    void runApplication() const {
      // send default values for all control system variables
      for(auto& pv : pvManager->getAllProcessVariables()) {
        callForType(pv->getValueType(), [&pv, this](auto arg) {
          if(!pv->isWriteable()) return;
          typedef decltype(arg) T;
          auto pv_casted = boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(pv);
          auto table = boost::fusion::at_key<T>(defaults.table);
          if(table.find(pv->getName()) != table.end()) {
            pv_casted->accessChannel(0) = table.at(pv->getName());
          }
          pv_casted->write();
        });
      }
      // start the application
      Application::getInstance().run();
      // set thread name
      Application::registerThread("TestThread");
      // wait until all devices are opened
      Application::testableModeUnlock("waitDevicesToOpen");
      while(true) {
        boost::this_thread::yield();
        bool allOpened = true;
        for(auto dm : Application::getInstance().deviceModuleList) {
          if(!dm->device.isOpened()) allOpened = false;
        }
        if(allOpened) break;
      }
      Application::testableModeLock("waitDevicesToOpen");
      // receive all initial values for the control system variables
      for(auto& pv : pvManager->getAllProcessVariables()) {
        if(!pv->isReadable()) continue;
        pv->readNonBlocking();
        assert(pv->readNonBlocking() == false);
      }
    }

    /** Perform a "step" of the application. This runs the application until all
     * input provided to it has been processed and all application modules wait
     * for new data in blocking read calls. This function returns only after the
     * application has reached that stated and was paused again. After returning
     * from this function, the result can be checked and new data can be provided
     * to the application. The new data will not be
     *  processed until the next call to step(). */
    void stepApplication() const { Application::getInstance().stepApplication(); }

    /** Obtain a scalar process variable from the application, which is published
     * to the control system. */
    template<typename T>
    ChimeraTK::ScalarRegisterAccessor<T> getScalar(const ChimeraTK::RegisterPath& name) const {
      // check for existing accessor in cache
      if(boost::fusion::at_key<T>(scalarMap.table).count(name) > 0) {
        return boost::fusion::at_key<T>(scalarMap.table)[name];
      }

      // obtain accessor from ControlSystemPVManager
      auto pv = pvManager->getProcessArray<T>(name);
      if(pv == nullptr) {
        throw ChimeraTK::logic_error("Process variable '" + name + "' does not exist.");
      }

      // obtain variable id from pvIdMap and transfer it to idMap (required by the
      // TestableModeAccessorDecorator)
      size_t varId = Application::getInstance().pvIdMap[pv->getUniqueId()];

      // decorate with TestableModeAccessorDecorator if variable is sender and
      // receiver is not poll-type, and store it in cache
      if(pv->isWriteable() && !Application::getInstance().testableMode_isPollMode[varId]) {
        auto deco = boost::make_shared<TestableModeAccessorDecorator<T>>(pv, false, true, varId, varId);
        Application::getInstance().testableMode_names[varId] = "ControlSystem:" + name;
        boost::fusion::at_key<T>(scalarMap.table)[name].replace(ChimeraTK::ScalarRegisterAccessor<T>(deco));
      }
      else {
        boost::fusion::at_key<T>(scalarMap.table)[name].replace(ChimeraTK::ScalarRegisterAccessor<T>(pv));
      }

      // return the accessor as stored in the cache
      return boost::fusion::at_key<T>(scalarMap.table)[name];
    }

    /** Obtain an array-type process variable from the application, which is
     * published to the control system. */
    template<typename T>
    ChimeraTK::OneDRegisterAccessor<T> getArray(const ChimeraTK::RegisterPath& name) const {
      // check for existing accessor in cache
      if(boost::fusion::at_key<T>(arrayMap.table).count(name) > 0) {
        return boost::fusion::at_key<T>(arrayMap.table)[name];
      }

      // obtain accessor from ControlSystemPVManager
      auto pv = pvManager->getProcessArray<T>(name);
      if(pv == nullptr) {
        throw ChimeraTK::logic_error("Process variable '" + name + "' does not exist.");
      }

      // obtain variable id from pvIdMap and transfer it to idMap (required by the
      // TestableModeAccessorDecorator)
      size_t varId = Application::getInstance().pvIdMap[pv->getUniqueId()];

      // decorate with TestableModeAccessorDecorator if variable is sender and
      // receiver is not poll-type, and store it in cache
      if(pv->isWriteable() && !Application::getInstance().testableMode_isPollMode[varId]) {
        auto deco = boost::make_shared<TestableModeAccessorDecorator<T>>(pv, false, true, varId, varId);
        Application::getInstance().testableMode_names[varId] = "ControlSystem:" + name;
        boost::fusion::at_key<T>(arrayMap.table)[name].replace(ChimeraTK::OneDRegisterAccessor<T>(deco));
      }
      else {
        boost::fusion::at_key<T>(arrayMap.table)[name].replace(ChimeraTK::OneDRegisterAccessor<T>(pv));
      }

      // return the accessor as stored in the cache
      return boost::fusion::at_key<T>(arrayMap.table)[name];
    }

    /** Convenience function to write a scalar process variable in a single call
     */
    template<typename TYPE>
    void writeScalar(const std::string& name, const TYPE value) {
      auto acc = getScalar<TYPE>(name);
      acc = value;
      acc.write();
    }

    /** Convenience function to write an array process variable in a single call
     */
    template<typename TYPE>
    void writeArray(const std::string& name, const std::vector<TYPE>& value) {
      auto acc = getArray<TYPE>(name);
      acc = value;
      acc.write();
    }

    /** Convenience function to read the latest value of a scalar process variable
     * in a single call */
    template<typename TYPE>
    TYPE readScalar(const std::string& name) {
      auto acc = getScalar<TYPE>(name);
      acc.readLatest();
      return acc;
    }

    /** Convenience function to read the latest value of an array process variable
     * in a single call */
    template<typename TYPE>
    std::vector<TYPE> readArray(const std::string& name) {
      auto acc = getArray<TYPE>(name);
      acc.readLatest();
      return acc;
    }

    /** Set default value for scalar process variable. */
    template<typename T>
    void setScalarDefault(const ChimeraTK::RegisterPath& name, const T& value) {
      std::vector<T> vv;
      vv.push_back(value);
      setArrayDefaults(name, vv);
    }

    /** Set default value for array process variable. */
    template<typename T>
    void setArrayDefault(const ChimeraTK::RegisterPath& name, const std::vector<T>& value) {
      // check if PV exists
      auto pv = pvManager->getProcessArray<T>(name);
      if(pv == nullptr) {
        throw ChimeraTK::logic_error("Process variable '" + name + "' does not exist.");
      }
      // store default value in map
      boost::fusion::at_key<T>(defaults.table)[name] = value;
    }

   protected:
    boost::shared_ptr<ControlSystemPVManager> pvManager;

    // Cache (possible decorated) accessors to avoid the need to create accessors
    // multiple times. This would not work if the accessor is decorated, since the
    // buffer would be lost and thus the current value could no longer be
    // obtained. This has to be done separately for scalar and array accessors and
    // in dependence of the user type. Since this is a cache and does not change
    // the logical behaviour of the class, the maps are defined mutable.
    template<typename UserType>
    using ScalarMap = std::map<std::string, ChimeraTK::ScalarRegisterAccessor<UserType>>;
    mutable ChimeraTK::TemplateUserTypeMap<ScalarMap> scalarMap;

    template<typename UserType>
    using ArrayMap = std::map<std::string, ChimeraTK::OneDRegisterAccessor<UserType>>;
    mutable ChimeraTK::TemplateUserTypeMap<ArrayMap> arrayMap;

    // default values for process variables
    template<typename UserType>
    using Defaults = std::map<std::string, std::vector<UserType>>;
    ChimeraTK::TemplateUserTypeMap<Defaults> defaults;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_FACILITY */
