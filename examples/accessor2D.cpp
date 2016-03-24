#include <mtca4u/Device.h>
#include <mtca4u/BackendFactory.h>
#include <iostream>

int main() {
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");
  mtca4u::Device myDevice;
  myDevice.open("MY_DEVICE");

  /* In this example there is a data region called "DATA" in
   * a module called "ADC".
   */
  mtca4u::TwoDRegisterAccessor<double> twoDAccessor =
    myDevice.getTwoDRegisterAccessor<double>("ADC/DATA");

  /* Read data for all channels from the hardware
   */
  twoDAccessor.read();
  
  /* You can access each sequence/channel individually. They are std::vectors.
   * You get a reference to the vector inside the accessor. No data copying.
   */
  for (size_t i = 0; i < twoDAccessor.getNumberOfDataSequences(); ++i){
    std::cout << "Channel " << i << ":";
    std::vector<double> & channel = twoDAccessor[i];
    for (double sample : channel){
      std::cout << " " << sample;
    }
    std::cout << std::endl;
  }

  /* You can modify the stuff at will in the accessors internal buffer.
   * In this example we use two [] operators like a 2D array.
   */
  for (size_t i = 0; i < twoDAccessor.getNumberOfDataSequences(); ++i){
    for (size_t j = 0; j < twoDAccessor.getNumberOfSamples(); ++j){
      twoDAccessor[i][j] = i*100 + j;
    }
  }

  /* Finally write to the hardware.
   */
  twoDAccessor.write();

  myDevice.close();

  return 0;
}
