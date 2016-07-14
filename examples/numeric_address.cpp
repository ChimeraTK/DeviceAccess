#include <mtca4u/Device.h>
#include <mtca4u/BackendFactory.h>
#include <iostream>

/* All information needed to access the device is
 * the device alias and the register names
 * (plus a .dmap file)
 */

int main(){
  /* Before you use a device you have to tell the factory
   * which dmap file to use.
   * \todo There should be a global function to do this. It is an implementation
   * detail that it's the factory which has to know it.
   */
  mtca4u::BackendFactory::getInstance().setDMapFilePath("example.dmap");

  /* Create a device. Make sure a device alias is present
   * in the dmap file.
   */
  mtca4u::Device myDevice;
  myDevice.open("MY_DEVICE");
  
  /* Here, the register is accessed by its numeric address through a special register
   * path name. The first component is a constant defining that a numeric address will
   * follow ("BAR"). The second component is the bar number (here 0), the third
   * component is the address in bytes (here 32) and the optional register length in
   * bytes (here 4, which is the default). This address matches the address of the
   * register TEMPERATURE_CONTROLLER.SET_POINT in the map file.
   *  
   * When using numeric addresses directly, no fixed point conversion is performed.
   */
    mtca4u::ScalarRegisterAccessor<int> temperatureSetPoint
    = myDevice.getScalarRegisterAccessor<int>(mtca4u::NumericAddress::BAR/0/32*4);

  /* To get the value from the device call read.
   */
  temperatureSetPoint.read();

  /* Now you can treat the accessor as if it was a regular float variable.
   */
  std::cout << "Current temperature set point is " << temperatureSetPoint << std::endl;
  temperatureSetPoint += 15;
  std::cout << "Temperature set point changed to " << temperatureSetPoint << std::endl;

  /* After you are done manipulating the accessor write it to the hardware.
   */
  temperatureSetPoint.write();

  /* It is good style to close the device when you are done, although
   * this would happen automatically once the device goes out of scope.
   */
  myDevice.close();
  
  return 0;
}
