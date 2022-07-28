// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ChimeraTK/Device.h>
#include <ChimeraTK/Utilities.h>

int main() {
  ChimeraTK::setDMapFilePath("registration_example.dmap");
  // The device in known now because it is specified in the dmap file.

  ChimeraTK::Device myCustomDevice;
  myCustomDevice.open("MY_CUSTOM_DEVICE");

  return 0;
}
