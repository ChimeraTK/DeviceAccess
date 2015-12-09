#include <mtca4u/Device.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";

int main() {
  // Before you use a device you have to tell the factory 
  // which dmap file to use.
  // \todo Fixme: we use one from the unit tests. examples should have its own
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);

  /** Entry in dmap file is
   * PCIE1     sdm://./pci:pcieunidummys6; mtcadummy.map
   */
  boost::shared_ptr<mtca4u::Device> myDevice( new mtca4u::Device());
  myDevice->open("PCIE1");
  
  boost::shared_ptr<mtca4u::Device::RegisterAccessor> accessor =
  		myDevice->getRegisterAccessor(REGISTER_NAME, MODULE_NAME);
  // read and print a data word works just like the Device functions,
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
