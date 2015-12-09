#include <mtca4u/Device.h>
#include <mtca4u/DMapFilesParser.h>
#include <mtca4u/BackendFactory.h>
#include <mtca4u/DeviceBackendImpl.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// All information needed to access the device is 
// the device alias and the register names 
// (plus a .dmap file and .map files)
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main(){
  // Before you use a device you have to tell the factory 
  // which dmap file to use.
  // \todo Fixme: we use one from the unit tests. examples should have its own
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);

  /** Create a pcie device. Make sure a device alias is present
   * in dmap file. Look at BackendFactory for further explanation */
  boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
  myDevice->open("PCIE1");
  
  boost::shared_ptr<mtca4u::Device::RegisterAccessor> accessor =
    myDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);
  
  // look on accessor.cpp for more examples what to do with the accessor
  std::cout << "Data as float is " << accessor->read<float>() << std::endl;
  
  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->close();
  
  return 0;
}
