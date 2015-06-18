#include <MtcaMappedDevice/devMap.h>
#include <MtcaMappedDevice/dmapFilesParser.h>

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// All information needed to access the device is 
// the device alias and the register names 
// (plus a .dmap file and .map files)
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";
static const std::string DEVICE_ALIAS = "DUMMY2";

int main(){
  // Unfortunatey devMap is templated against the implementation type
  // so the abstraction to devBase does not work. To be resolved soon.
  mtca4u::devMap<mtca4u::devPCIE> myMappedDevice;

  // Open the device. The command is a bit lengthy because
  // retrieving the device name and the map file name is a three
  // step process.
  // - create a dmapFilesParser which scans the current directory "."
  // - get the dmapElem for the correct device alias
  // - get the device name and the map file name from the dmapElem 
  //   (it's a pair of strings)
  // All are concatenated and executed as the argument of the 
  // open function to avoid unnecessary temporary variables.
  myMappedDevice.openDev(mtca4u::dmapFilesParser(".")
			 .getdMapFileElem(DEVICE_ALIAS)
			 .getDeviceFileAndMapFileName());

  boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor> accessor = 
    myMappedDevice.getRegisterAccessor(REGISTER_NAME, MODULE_NAME);

  // look on accessor.cpp for more examples what to do with the accessor
  std::cout << "Data as float is " << accessor->read<float>() << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myMappedDevice.closeDev();

  return 0;
}
