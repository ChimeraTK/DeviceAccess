/*
 * TestFacility.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_TEST_FACILITY
#define CHIMERATK_TEST_FACILITY

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
        auto pv = boost::make_shared<TestDecoratorRegisterAccessor<T>>(pvManager->getProcessArray<T>(name));
        return mtca4u::ScalarRegisterAccessor<T>(pv);
      }
      
      /** Obtain an array-type process variable from the application, which is published to the control system. */
      template<typename T>
      mtca4u::OneDRegisterAccessor<T> getArray(const mtca4u::RegisterPath &name) const {
        auto pv = boost::make_shared<TestDecoratorRegisterAccessor<T>>(pvManager->getProcessArray<T>(name));
        return mtca4u::OneDRegisterAccessor<T>(pv);
      }
      
  protected:
    
      boost::shared_ptr<ControlSystemPVManager> pvManager;
      
  };
  
} /* namespace ChimeraTK */

#endif /* CHIMERATK_TEST_FACILITY */
