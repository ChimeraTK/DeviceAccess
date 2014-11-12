#include <MtcaMappedDevice/devBase.h>
#include <MtcaMappedDevice/devPCIE.h>

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// For the simple example the offset of the user word is hard coded.
static const unsigned int WORD_USER_OFFSET = 0xC;
static const unsigned int WORD_USER_BAR = 0;
static const std::string DEVICE_NAME = "/dev/mtcadummys0";

int main(){
  // Always use the devBase interface in the user code. This allows
  // for instance to switch to an mtca4u::DummyDevice for testing 
  // without having to touch the code.
  // (Shared pointers are used to do the memory management for us, see
  //  mtca4u coding style rules. Equivalent to 
  // 'devBase * myDevice = new devPCIE', except that 
  // we don't have to care about deleting the object.)
  boost::shared_ptr<mtca4u::devBase> myDevice( new mtca4u::devPCIE );

  // open the device
  myDevice->openDev(DEVICE_NAME);

  // read and print a data word from a register
  int32_t dataWord;
  myDevice->readReg(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  myDevice->writeReg(WORD_USER_OFFSET, dataWord + 42, 0 /*bar 0*/);
  myDevice->readReg(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->closeDev();

  return 0;
}
