#include <MtcaMappedDevice/Device.h>
#include "MtcaMappedDevice/BackendFactory.h"
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";
static const std::string DEVICE_NAME = "/dev/mtcadummys0";
static const std::string MAP_NAME = "mtcadummy.map";

int main() {

	static mtca4u::BackendFactory FactoryInstance = mtca4u::BackendFactory::getInstance();
	/** Entry in dmap file is
	 * PCIE1     sdm://./pci:pcieunidummys6; mtcadummy.map
	 */
	/*boost::shared_ptr< mtca4u::Device< mtca4u::DeviceBackend > > myDevice =
	FactoryInstance.createDevice("PCIE1");*/

	boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
	myDevice->open("PCIE1");

	/*boost::shared_ptr<mtca4u::Device<mtca4u::DeviceBackend>::RegisterAccessor> accessor =
  			myDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);*/
	boost::shared_ptr<mtca4u::Device::RegisterAccessor> accessor =
			myDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);
	// read and print a data word works just like the devMap functions,
	// except that you don't give the register name
	int32_t dataWord;
	accessor->readRaw(&dataWord);
	std::cout << "Data word on the device is " << dataWord << std::endl;

	int32_t writeWord = dataWord + 42;
	accessor->writeRaw(&writeWord);
	accessor->readRaw(&dataWord);
	std::cout << "Data word on the device now is " << dataWord << std::endl;

	// The data word in the example is interpreted as 12 bit signed
	// fixed point with 3 fractional bits. We can directly use the float
	// representation
	std::cout << "Data as float is " << accessor->read<float>() << std::endl;

	accessor->write(17.32);
	accessor->readRaw(&dataWord);
	std::cout << "Float value " << accessor->read<float>()
            		<< " has the fixed point representation " << std::showbase
								<< std::hex << dataWord << std::dec << std::endl;
	// Note how the float is rounded to the next possible fixed point
	// representation.

	// It is good style to close the device when you are done, although
	// this would happen automatically once the device goes out of scope.
	myDevice->close();

	return 0;
}
