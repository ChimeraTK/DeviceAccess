#ifndef _INDEPENDENT_CONTOL_CORE_H_
#define _INDEPENDENT_CONTOL_CORE_H_

#include "InstaCoSADev.h"

#include <boost/scoped_ptr.hpp>

#include <ControlSystemAdapter/DevicePVManager.h>
#include <ControlSystemAdapter/ProcessScalar.h>
#include <ControlSystemAdapter/DeviceSynchronizationUtility.h>
#include <ControlSystemAdapter/SynchronizationDirection.h>

/** This is just a simple example class.
 *
 *  All functions are definded inline for the sake of the example. 
 *  It is strongly recommended to use proper header/object separation for
 *  real code!
 */
class IndependentControlCore{
 private:
  mtca4u::DevicePVManager::SharedPtr _processVariableManager;
 
  boost::scoped_ptr< boost::thread > _deviceThread;

  mtca4u::Device _device;

  mtca4u::InstaCoSADev _synchroniser;

  void mainLoop();

 public:
  /** The constructor gets an instance of the variable factory to use. 
   *  The variables in the factory should already be initialised because the hardware is initialised here.
   */
  IndependentControlCore(boost::shared_ptr<mtca4u::DevicePVManager> const & processVariableManager)
  : _processVariableManager( processVariableManager ),
    _synchroniser( processVariableManager )
  {

    // open the device
    mtca4u::BackendFactory::getInstance().setDMapFilePath("dummy.dmap");
    _device.open("Dummy0");

    // initialise the synchroniser
    _synchroniser.addModule(_device, "MyModule", "MyLocation");

    // start the device thread, which is executing the main loop
    _deviceThread.reset( new boost::thread( boost::bind( &IndependentControlCore::mainLoop, this ) ) );
  }
  
  ~IndependentControlCore(){
    // stop the device thread before any other destructors are called
    _deviceThread->interrupt();
    _deviceThread->join();
 }

};

inline void IndependentControlCore::mainLoop(){
  mtca4u::DeviceSynchronizationUtility syncUtil(_processVariableManager);
 
  while (!boost::this_thread::interruption_requested()) {
    syncUtil.receiveAll();
    _synchroniser.transferData(mtca4u::deviceToControlSystem);
    syncUtil.sendAll();
    boost::this_thread::sleep_for( boost::chrono::milliseconds(100) );
  }
}

#endif // _INDEPENDENT_CONTOL_CORE_H_
