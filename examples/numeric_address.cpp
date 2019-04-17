#include <ChimeraTK/Device.h>
#include <ChimeraTK/NumericAddress.h>
#include <iostream>

/*
 * When you are doing numerical addressing you usually don't have a map file
 * yet. You directly open the device (pcie for instance) with the URI syntax
 * which you usually put into the dmap file. In this example, the map file is
 * only needed to tell the dummy what to simulate. In the code the map file
 * information is not used since the numeric address is directly written in the
 * code. Otherwise, the example is identical to "basic.cpp". Look there for
 * additional documentation.
 */

int main() {
  /*
   * If you have the mtcadummy driver installed you can also use a pci device:
   * myDevice("(pci:pcieunidummys6)");
   *
   * Note: The dummy always needs a map file to know the size of the address
   * space to emulate.
   */
  ChimeraTK::Device myDevice("(dummy?map=my_device.map)");
  myDevice.open();

  /*
   * Here, the register is accessed by its numeric address through a special
   * register path name. The first component is a constant defining that a
   * numeric address will follow ("BAR"). The second component is the bar number
   * (here 0), the third component is the address in bytes (here 32) and the
   * optional register length in bytes (here 4, which is the default). This
   * address matches the address of the register
   * TEMPERATURE_CONTROLLER.SET_POINT in the map file.
   *
   * When using numeric addresses directly, no fixed point conversion is
   * performed.
   */
  ChimeraTK::ScalarRegisterAccessor<int> temperatureSetPoint =
      myDevice.getScalarRegisterAccessor<int>(ChimeraTK::numeric_address::BAR / 0 / 32 * 4);

  temperatureSetPoint.read();
  std::cout << "Current temperature set point is " << temperatureSetPoint << std::endl;
  temperatureSetPoint += 15;
  std::cout << "Temperature set point changed to " << temperatureSetPoint << std::endl;
  temperatureSetPoint.write();

  myDevice.close();

  return 0;
}
