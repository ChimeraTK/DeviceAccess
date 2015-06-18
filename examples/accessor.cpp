#include <MtcaMappedDevice/devMap.h>

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string REGISTER_NAME = "WORD_USER";
static const std::string MODULE_NAME = "BOARD";
static const std::string DEVICE_NAME = "/dev/mtcadummys0";
static const std::string MAP_NAME = "mtcadummy.map";

int main() {
  // Unfortunatey devMap is templated against the implementation type
  // so the abstraction to devBase does not work. To be resolved soon.
  mtca4u::devMap<mtca4u::devPCIE> myMappedDevice;

  // open the device
  myMappedDevice.openDev(DEVICE_NAME, MAP_NAME);

  boost::shared_ptr<mtca4u::devMap<mtca4u::devPCIE>::RegisterAccessor>
  accessor = myMappedDevice.getRegisterAccessor(REGISTER_NAME, MODULE_NAME);

  // read and print a data word works just like the devMap functions,
  // except that you don't give the register name
  int32_t dataWord;
  accessor->readReg(&dataWord);
  std::cout << "Data word on the device is " << dataWord << std::endl;

  int32_t writeWord = dataWord + 42;
  accessor->writeReg(&writeWord);
  accessor->readReg(&dataWord);
  std::cout << "Data word on the device now is " << dataWord << std::endl;

  // The data word in the example is interpreted as 12 bit signed
  // fixed point with 3 fractional bits. We can directly use the float
  // representation
  std::cout << "Data as float is " << accessor->read<float>() << std::endl;

  accessor->write(17.32);
  accessor->readReg(&dataWord);
  std::cout << "Float value " << accessor->read<float>()
            << " has the fixed point representation " << std::showbase
            << std::hex << dataWord << std::dec << std::endl;
  // Note how the float is rounded to the next possible fixed point
  // representation.

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  myMappedDevice.closeDev();

  return 0;
}
