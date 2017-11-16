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
#include <mtca4u/ScalarRegisterAccessor.h>
#include <mtca4u/OneDRegisterAccessor.h>

#include "Application.h"
#include "TestDecoratorRegisterAccessor.h"

namespace ChimeraTK {

  /** Helper class to facilitate tests of applications based on ApplicationCore */
  class TestFacility {
    
    public:
    
      /** The constructor will internally obtain the instance of the application, so the instance of the TestFacility
       *  must not be created before the application (i.e. usually not before the main() routine). The application will
       *  automatically be put into the testable mode and initialised. */
      TestFacility() {
        auto pvManagers = createPVManager();
        pvManager = pvManagers.first;
        Application::getInstance().setPVManager(pvManagers.second);
        Application::getInstance().enableTestableMode();
        Application::getInstance().initialise();
      }
      
      /** Start the application. This simply calls Application::run(). Since the application is in testable mode, it
       *  will be ??? TODO define precisely what happens on start up */
      void runApplication() const {
        Application::getInstance().run();
      }
      
      /** Perform a "step" of the application. This runs the application until all input provided to it has been
       *  processed and all application modules wait for new data in blocking read calls. This function returns only
       *  after the application has reached that stated and was paused again. After returning from this function,
       *  the result can be checked and new data can be provided to the application. The new data will not be
       *  processed until the next call to step(). */
      void stepApplication() const {
        Application::getInstance().stepApplication();
      }
      
      /** Obtain a scalar process variable from the application, which is published to the control system. */
      template<typename T>
      mtca4u::ScalarRegisterAccessor<T> getScalar(const mtca4u::RegisterPath &name) const {

        // check for existing accessor in cache
        if(boost::fusion::at_key<T>(scalarMap.table).count(name) > 0) {
          return boost::fusion::at_key<T>(scalarMap.table)[name];
        }

        // obtain accessor from ControlSystemPVManager
        auto pv = pvManager->getProcessArray<T>(name);
        if(pv == nullptr) {
          throw mtca4u::DeviceException("Process variable '"+name+"' does not exist.",
                                        mtca4u::DeviceException::REGISTER_DOES_NOT_EXIST);
        }

        // decorate with TestDecoratorRegisterAccessor if variable is sender and receiver is not poll-type,
        // and store it in cache
        if(pv->isWriteable() && !Application::getInstance().testableMode_isPollMode[pv->getUniqueId()]) {
          auto deco = boost::make_shared<TestDecoratorRegisterAccessor<T>>(pv);
          Application::getInstance().testableMode_names[pv->getUniqueId()] = "ControlSystem:"+name;
          boost::fusion::at_key<T>(scalarMap.table)[name].replace(mtca4u::ScalarRegisterAccessor<T>(deco));
        }
        else {
          boost::fusion::at_key<T>(scalarMap.table)[name].replace(mtca4u::ScalarRegisterAccessor<T>(pv));
        }

        // return the accessor as stored in the cache
        return boost::fusion::at_key<T>(scalarMap.table)[name];
      }
      
      /** Obtain an array-type process variable from the application, which is published to the control system. */
      template<typename T>
      mtca4u::OneDRegisterAccessor<T> getArray(const mtca4u::RegisterPath &name) const {

        // check for existing accessor in cache
        if(boost::fusion::at_key<T>(arrayMap.table).count(name) > 0) {
          return boost::fusion::at_key<T>(arrayMap.table)[name];
        }

        // obtain accessor from ControlSystemPVManager
        auto pv = pvManager->getProcessArray<T>(name);
        if(pv == nullptr) {
          throw mtca4u::DeviceException("Process variable '"+name+"' does not exist.",
                                        mtca4u::DeviceException::REGISTER_DOES_NOT_EXIST);
        }

        // decorate with TestDecoratorRegisterAccessor if variable is sender and receiver is not poll-type,
        // and store it in cache
        if(pv->isWriteable() && !Application::getInstance().testableMode_isPollMode[pv->getUniqueId()]) {
          auto deco = boost::make_shared<TestDecoratorRegisterAccessor<T>>(pv);
          Application::getInstance().testableMode_names[pv->getUniqueId()] = "ControlSystem:"+name;
          boost::fusion::at_key<T>(arrayMap.table)[name].replace(mtca4u::OneDRegisterAccessor<T>(deco));
        }
        else {
          boost::fusion::at_key<T>(arrayMap.table)[name].replace(mtca4u::OneDRegisterAccessor<T>(pv));
        }

        // return the accessor as stored in the cache
        return boost::fusion::at_key<T>(arrayMap.table)[name];
      }
      
      /** Convenience function to write a scalar process variable in a single call */
      template<typename TYPE>
      void writeScalar( const std::string &name, const TYPE value ) {
        auto acc = getScalar<TYPE>(name);
        acc = value;
        acc.write();
      }
      
      /** Convenience function to write an array process variable in a single call */
      template<typename TYPE>
      void writeArray( const std::string &name, const std::vector<TYPE> &value ) {
        auto acc = getArray<TYPE>(name);
        acc = value;
        acc.write();
      }
      
      /** Convenience function to read the latest value of a scalar process variable in a single call */
      template<typename TYPE>
      TYPE readScalar( const std::string &name ) {
        auto acc = getScalar<TYPE>(name);
        acc.readLatest();
        return acc;
      }
      
      /** Convenience function to read the latest value of an array process variable in a single call */
      template<typename TYPE>
      std::vector<TYPE> readArray( const std::string &name ) {
        auto acc = getArray<TYPE>(name);
        acc.readLatest();
        return acc;
      }

      
  protected:
    
      boost::shared_ptr<ControlSystemPVManager> pvManager;
      
      // Cache (possible decorated) accessors to avoid the need to create accessors multiple times. This would not work
      // if the accessor is decorated, since the buffer would be lost and thus the current value could no longer be
      // obtained. This has to be done separately for scalar and array accessors and in dependence of the user type.
      // Since this is a cache and does not change the logical behaviour of the class, the maps are defined mutable.
      template<typename UserType>
      using ScalarMap = std::map<std::string, mtca4u::ScalarRegisterAccessor<UserType>>;
      mutable mtca4u::TemplateUserTypeMap<ScalarMap> scalarMap;

      template<typename UserType>
      using ArrayMap = std::map<std::string, mtca4u::OneDRegisterAccessor<UserType>>;
      mutable mtca4u::TemplateUserTypeMap<ArrayMap> arrayMap;

  };
  
} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_FACILITY */
