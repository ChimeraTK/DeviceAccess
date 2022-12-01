// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Device.h"
#include "Utilities.h"

#include <iostream>

/*
 * This test code needs to be executed on a Xilinx ZCU102 evaluation board, using the hardware project
 *  files from git@gitlab.msktools.desy.de:fpgafw/projects/test/test_bsp_motctrl.git (Tag: 0.1.0, Commit ID: a6160e40)
 */

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
  ChimeraTK::setDMapFilePath("uioDeviceTest.dmap");

  /*
   * Create a device. Make sure a device alias is present
   * in the dmap file.
   */
  ChimeraTK::Device myDevice("MOTCTRL");

  myDevice.open();

  ChimeraTK::ScalarRegisterAccessor<uint32_t> motorPosition = myDevice.getScalarRegisterAccessor<uint32_t>(
      "MOTOR_CONTROL/MOTOR_POSITION", 0, {ChimeraTK::AccessMode::wait_for_new_data});

  myDevice.close();
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

  motorDestination = 0;
  motorDestination.write();

  motorPosition.read();
  std::cout << "Motor at position " << motorPosition << std::endl;

  for(size_t i = 0; i < 10; i++) {
    // Set new target position
    motorDestination += 5000;
    motorDestination.write();
    std::cout << std::endl << "Target position is " << motorDestination << std::endl;

    // Start motor movement
    motorStart = 1;
    motorStart.write();
    motorStart = 0;
    motorStart.write();

    // Wait until motor reached position
    motorPosition.read();
    std::cout << "Motor at position " << motorPosition << std::endl;
  }

  myDevice.close();

  return 0;
}
