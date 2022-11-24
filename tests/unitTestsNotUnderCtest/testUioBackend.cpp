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
  myDevice.activateAsyncRead();

  /*
   * Registers are defined by a path, which consists of a hierarchy of
   * names separated by '/'. In this is example it is Module/Register.
   * In this basic example we use a register which contains a single value
   * (a scalar).
   */

  ChimeraTK::ScalarRegisterAccessor<uint32_t> maximumAcceleration =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_MAX_ACC");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> maximumVelocity =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_MAX_VEL");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> baseVelocity =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_BASE_VEL");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> pulseWidth =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_PULSE_WIDTH");

  ChimeraTK::ScalarRegisterAccessor<int32_t> motorDestination =
      myDevice.getScalarRegisterAccessor<int32_t>("MOTOR_CONTROL/MOTOR_DESTINATION");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> motorStart =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_START");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> motorPosition = myDevice.getScalarRegisterAccessor<uint32_t>(
      "MOTOR_CONTROL/MOTOR_POSITION", 0, {ChimeraTK::AccessMode::wait_for_new_data});

  //  ChimeraTK::ScalarRegisterAccessor<uint32_t> motorPosition =
  //      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_POSITION");

  ChimeraTK::ScalarRegisterAccessor<uint32_t> resetMotorPosition =
      myDevice.getScalarRegisterAccessor<uint32_t>("MOTOR_CONTROL/MOTOR_POSITION_RESET");

  /* Configure motor control logic*/
  maximumAcceleration = 2000;
  maximumAcceleration.write();

  maximumVelocity = 2000;
  maximumVelocity.write();

  baseVelocity = 0;
  baseVelocity.write();

  pulseWidth = 200;
  pulseWidth.write();

  /* Read back configuration*/

  maximumAcceleration.read();
  maximumVelocity.read();
  baseVelocity.read();
  pulseWidth.read();

  std::cout << "maximumAcceleration = " << maximumAcceleration << std::endl;
  std::cout << "maximumVelocity     = " << maximumVelocity << std::endl;
  std::cout << "baseVelocity        = " << baseVelocity << std::endl;
  std::cout << "pulseWidth          = " << pulseWidth << std::endl;

  /* Move motor*/

  motorStart = 0;
  motorStart.write();

  resetMotorPosition = 1;
  resetMotorPosition.write();

  resetMotorPosition = 0;
  resetMotorPosition.write();

  motorDestination = 22000;
  motorDestination.write();

  motorDestination.read();
  std::cout << "Target position is " << motorDestination << std::endl;

  motorStart = 1;
  motorStart.write();
  motorStart = 0;
  motorStart.write();

  motorPosition.read();
  std::cout << "Current motor position is " << motorPosition << std::endl;

  motorPosition.read();
  std::cout << "Current motor position is " << motorPosition << std::endl;

  motorPosition.read();
  std::cout << "Current motor position is " << motorPosition << std::endl;

  motorPosition.read();
  std::cout << "Current motor position is " << motorPosition << std::endl;

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
