// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"
#include "Utilities.h"

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
  ChimeraTK::setDMapFilePath("uio_test.dmap");

  /*
   * Create a device. Make sure a device alias is present
   * in the dmap file.
   */
  ChimeraTK::Device myDevice("MOTCTRL");
  myDevice.open();

  /*
   * Registers are defined by a path, which consists of a hierarchy of
   * names separated by '/'. In this is example it is Module/Register.
   * In this basic example we use a register which contains a single value
   * (a scalar).
   */
  ChimeraTK::ScalarRegisterAccessor<uint32_t> maximumAcceleration =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_MAX_ACC");

  /*
   * To get the value from the device call read.
   */
  maximumAcceleration.read();

  /*
   * Now you can treat the accessor as if it was a regular float variable.
   */
  std::cout << "Current motor maximum acceleration is " << maximumAcceleration << std::endl;
  maximumAcceleration += 1000;
  std::cout << "Motor maximum acceleration changed to " << maximumAcceleration << std::endl;

  /*
   * After you are done manipulating the accessor write it to the hardware.
   */
  maximumAcceleration.write();

  /*
   * It is good style to close the device when you are done, although
   * this would happen automatically once the device goes out of scope.
   */
  myDevice.close();

  return 0;
}
