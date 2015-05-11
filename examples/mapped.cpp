#include <MtcaMappedDevice/devMap.h>

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// We do not need to deal with addresses any more but use the 
// register and module name
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";
static const std::string DEVICE_NAME = "/dev/mtcadummys0";
static const std::string MAP_NAME = "mtcadummy.map";

int main(){
  // Unfortunatey devMap is templated against the implementation type
  // so the abstraction to devBase does not work. To be resolved soon.
  mtca4u::devMap<mtca4u::devPCIE> myMappedDevice;

  // open the device
  myMappedDevice.openDev(DEVICE_NAME, MAP_NAME);

  // read and print a data word from a register
  int32_t dataWord;
  myMappedDevice.readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  // A bit clumsy: As write can take multiple words we have to pass a 
  // pointer.
  // Read the documentation  mtca4u::devMap< T >::readReg  if you
  // want to use arrays!
  int32_t writeWord = dataWord + 42;
  myMappedDevice.writeReg(REGISTER_NAME, MODULE_NAME, &writeWord);
  myMappedDevice.readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myMappedDevice.closeDev();

  return 0;
}
