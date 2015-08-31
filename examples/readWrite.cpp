#include <MtcaMappedDevice/BaseDevice.h>
#include <MtcaMappedDevice/PcieDevice.h>
#include <MtcaMappedDevice/DeviceFactory.h>
#include <MtcaMappedDevice/BaseDeviceImpl.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// For the simple example the offset of the user word is hard coded.
static const unsigned int WORD_USER_OFFSET = 0xC;
static const unsigned int WORD_USER_BAR = 0;

int main(){

  static mtca4u::DeviceFactory FactoryInstance = mtca4u::DeviceFactory::getInstance();
  boost::shared_ptr< mtca4u::MappedDevice< mtca4u::BaseDevice > > myDevice =
  FactoryInstance.createMappedDevice("PCIE1");
  // read and print a data word from a register
  int32_t dataWord;
  myDevice->readRaw(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  myDevice->writeRaw(WORD_USER_OFFSET, dataWord + 42, 0 /*bar 0*/);
  myDevice->readRaw(WORD_USER_OFFSET, &dataWord, 0 /*bar 0*/);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myDevice->close();

  return 0;
}
