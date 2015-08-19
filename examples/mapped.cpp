#include <MtcaMappedDevice/MappedDevice.h>
#include "MtcaMappedDevice/DeviceFactory.h"
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// We do not need to deal with addresses any more but use the 
// register and module name
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main(){

	static mtca4u::DeviceFactory FactoryInstance = mtca4u::DeviceFactory::getInstance();
	/** Entry in dmap file is
	 * PCIE2  sdm://./pci:mtcadummys0; mtcadummy.map
	 */
	boost::shared_ptr< mtca4u::MappedDevice< mtca4u::BaseDevice > > myMappedDevice =
	FactoryInstance.createMappedDevice("PCIE2");
  // read and print a data word from a register
  int32_t dataWord;
  myMappedDevice->readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  // write something different to the register, read it back and print it
  // A bit clumsy: As write can take multiple words we have to pass a 
  // pointer.
  // Read the documentation  mtca4u::devMap< T >::readReg  if you
  // want to use arrays!
  int32_t writeWord = dataWord + 42;
  myMappedDevice->writeReg(REGISTER_NAME, MODULE_NAME, &writeWord);
  myMappedDevice->readReg(REGISTER_NAME, MODULE_NAME, &dataWord);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myMappedDevice->closeDev();

  return 0;
}
