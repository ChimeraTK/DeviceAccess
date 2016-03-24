#include <mtca4u/Device.h>
#include <mtca4u/BackendFactory.h>
#include <iostream>

int main() {
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");
  mtca4u::Device myDevice;
  myDevice.open("MY_DEVICE");

  /* The device contains a register called CLOCKS in the BOARD section.
   * It contains 4 values for 4 different clocks.
   */
  mtca4u::OneDRegisterAccessor<double> clocks =
    myDevice.getOneDRegisterAccessor<double>("BOARD/CLOCKS");
  std::cout << "The clocks register has " << clocks.getNElements() << " elements." <<std::endl;

  /* Read data for the whole register from the hardware
   */
  clocks.read();
  
  /* The OneDRegisterAccessor behaves like a std::vector, incl. [] operator
   * and iterators.
   */
  for (size_t i = 0; i < clocks.getNElements(); ++i){
      clocks[i] = 42+i;
  }
  std::cout << "Clocks are";
  for (auto clockValue : clocks){
      std::cout << " " << clockValue;
  }
  std::cout << std::endl;

  /* Write all values of the CLOCKS register to the hardware.
   */
  clocks.write();

  myDevice.close();

  return 0;
}
