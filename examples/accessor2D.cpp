#include <mtca4u/Device.h>
#include <mtca4u/TwoDRegisterAccessor.h>
#include <mtca4u/BackendFactory.h>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

static const std::string MODULE_NAME = "ADC";
static const std::string DATA_REGION_NAME = "DATA";

int main() {
  // Before you use a device you have to tell the factory which dmap file to use.
  // \todo There should be a global function to do this. It is an implementation
  // detail that it's the factory which has to know it.
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");

  // open the device:
  boost::shared_ptr< mtca4u::Device > device(new mtca4u::Device());
  device->open("MY_DEVICE");

  mtca4u::TwoDRegisterAccessor<double> twoDAccessor =
    device->getTwoDRegisterAccessor<double>(MODULE_NAME, DATA_REGION_NAME);

  // read data for all channels from the hardware
  twoDAccessor.read();
  
  // You can access each sequence/channel individually. They are std::vectors.
  // You get a reference to the vector inside the accessor. No data copying.
  for (size_t i = 0; i < twoDAccessor.getNumberOfDataSequences(); ++i){
    std::cout << "Channel " << i << ":";
    std::vector<double> & channel = twoDAccessor[i];
    for (double sample : channel){
      std::cout << " " << sample;
    }
    std::cout << std::endl;
  }

  // Or you can just use it like a 2D array and modify the stuff at will in the
  // accessors internal buffer
  for (size_t i = 0; i < twoDAccessor.getNumberOfDataSequences(); ++i){
    for (size_t j = 0; j < twoDAccessor.getNumberOfSamples(); ++j){
      twoDAccessor[i][j] = i*100 + j;
    }
  }

  // Finally write to the hardware
  twoDAccessor.write();

  // It is good style to close the device when you are done, although
  // this would happen automatically once the device goes out of scope.
  device->close();

  return 0;
}
