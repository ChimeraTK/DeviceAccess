#include <ChimeraTK/Device.h>
#include <ChimeraTK/Utilities.h>
#include <iostream>

/*
 * All information needed to access the device is
 * the device alias and the register names
 * (plus a .dmap file)
 */

int main() {
  /*
   * Before you use a device you have to tell DeviceAccess
   * which dmap file to use.
   */
  ChimeraTK::setDMapFilePath("example.dmap");

  /*
   * Create a device. Make sure a device alias is present
   * in the dmap file.
   */
  ChimeraTK::Device myDevice("MY_DEVICE");
  myDevice.open();

  /*
   * Registers are defined by a path, which consists of a hierarchy of
   * names separated by '/'. In this is example it is Module/Register.
   * In this basic example we use a register which contains a single value
   * (a scalar).
   *
   * The example device has a temperature controller with a set value.
   */
  ChimeraTK::ScalarRegisterAccessor<float> temperatureSetPoint =
      myDevice.getScalarRegisterAccessor<float>("TEMPERATURE_CONTROLLER/SET_POINT");

  /*
   * To get the value from the device call read.
   */
  temperatureSetPoint.read();

  /*
   * Now you can treat the accessor as if it was a regular float variable.
   */
  std::cout << "Current temperature set point is " << temperatureSetPoint << std::endl;
  temperatureSetPoint += 1.5;
  std::cout << "Temperature set point changed to " << temperatureSetPoint << std::endl;

  /*
   * After you are done manipulating the accessor write it to the hardware.
   */
  temperatureSetPoint.write();

  /*
   * It is good style to close the device when you are done, although
   * this would happen automatically once the device goes out of scope.
   */
  myDevice.close();

  return 0;
}
