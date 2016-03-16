#include <mtca4u/Device.h>
#include <mtca4u/BufferingRegisterAccessor.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// All information needed to access the device is 
// the device alias and the register names 
// (plus a .dmap file)
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main(){
  // Before you use a device you have to tell the factory 
  // which dmap file to use.
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");

  /** Create a device. Make sure a device alias is present
   * in the dmap file. */
  mtca4u::Device myDevice;
  myDevice.open("MY_DEVICE");
  
  mtca4u::BufferingRegisterAccessor<float> accessor = myDevice.getBufferingRegisterAccessor<float>(MODULE_NAME, REGISTER_NAME);
  // To get the value from the backend call read.
  accessor.read();

  // Now you can treat the accessor as if it was a float.
  std::cout << "Data as float is " << accessor << std::endl;
  accessor += 1.5;
  std::cout << "Data now is  " << accessor << std::endl;

  // After you are done manipulating the accessor write it to the hardware.
  accessor.write();

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice.close();
  
  return 0;
}
