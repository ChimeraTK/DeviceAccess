#include <ChimeraTK/Device.h>
#include <ChimeraTK/Utilities.h>
#include <iostream>

int main() {
  ChimeraTK::setDMapFilePath("example.dmap");

  ChimeraTK::Device myDevice("MY_DEVICE");
  myDevice.open();

  /*
   * In this example there is a data region called "DATA" in
   * a module called "ADC".
   */
  ChimeraTK::TwoDRegisterAccessor<double> twoDAccessor = myDevice.getTwoDRegisterAccessor<double>("ADC/DATA");

  /*
   * Read data for all channels from the hardware
   */
  twoDAccessor.read();

  /*
   * You can access each sequence/channel individually. They are std::vectors.
   * You get a reference to the vector inside the accessor. No data copying.
   */
  for(size_t i = 0; i < twoDAccessor.getNChannels(); ++i) {
    std::cout << "Channel " << i << ":";
    std::vector<double>& channel = twoDAccessor[i];
    for(double sample : channel) {
      std::cout << " " << sample;
    }
    std::cout << std::endl;
  }

  /*
   * You can modify the stuff at will in the accessors internal buffer.
   * In this example we use two [] operators like a 2D array.
   */
  for(size_t i = 0; i < twoDAccessor.getNChannels(); ++i) {
    for(size_t j = 0; j < twoDAccessor.getNElementsPerChannel(); ++j) {
      twoDAccessor[i][j] = i * 100 + j;
    }
  }

  /*
   * Finally write to the hardware.
   */
  twoDAccessor.write();

  myDevice.close();

  return 0;
}
