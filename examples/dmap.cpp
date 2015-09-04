#include <MtcaMappedDevice/MappedDevice.h>
#include <MtcaMappedDevice/DMapFilesParser.h>
#include <MtcaMappedDevice/DeviceFactory.h>
#include <MtcaMappedDevice/BaseDeviceImpl.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

// All information needed to access the device is 
// the device alias and the register names 
// (plus a .dmap file and .map files)
static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main(){
  /** Create a pcie device. Make sure pcie is registerd and a device alias is present
   * in dmap file. Look at DeviceFactory for further explaination */

  /*static mtca4u::DeviceFactory FactoryInstance = mtca4u::DeviceFactory::getInstance();
	boost::shared_ptr< mtca4u::MappedDevice< mtca4u::BaseDevice > > myMappedDevice =
	FactoryInstance.createMappedDevice("PCIE1");*/
	boost::shared_ptr<mtca4u::MappedDevice> myMappedDevice( new mtca4u::MappedDevice());
	myMappedDevice->open("PCIE1");

	/*boost::shared_ptr<mtca4u::MappedDevice<mtca4u::BaseDevice>::RegisterAccessor> accessor =
			myMappedDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);*/

	boost::shared_ptr<mtca4u::MappedDevice::RegisterAccessor> accessor =
				myMappedDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);

  // look on accessor.cpp for more examples what to do with the accessor
  std::cout << "Data as float is " << accessor->read<float>() << std::endl;

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myMappedDevice->close();

  return 0;
}
