// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

/*  Unfortunately the DeviceAccess base package does not have a demo push type accessor we can use.
 *  That's why this code uses the Doocs backend. Use it together with example2 from ApplicationCore.
 *  You have to activate the zmq sending in the ApplicationCore example2 by using the configuration files
 *  demoApp2-DoocsVariableConfig.xml and demo_example2.conf, which are provided with this example.
 *
 *  Compile this file by linking directly with the Doocs backend (for instance directly on the command line):
 *  g++ read_temperature_doocs_zmq.cpp -o read_temperature_doocs_zmq `ChimeraTK-DeviceAccess-config --cppflags
 * --ldflags` -lpthread -Wl,--no-as-needed -lChimeraTK-DeviceAccess-DoocsBackend
 */
#include <ChimeraTK/Device.h>

#include <iostream>

int main() {
  ChimeraTK::Device d;
  d.open("(doocs:TEST.DOOCS/LOCALHOST_610498009/Bakery)");

  // You have to activate the receiving of asynchronousy send data before an accessor with AccessMode::wait_for_new_data
  // will receive anything. You can first create all accessors and then activate all of them at the same point in time
  // with this.
  d.activateAsyncRead();

  // The third argument is a list of ChimeraTK::AccessMode. We only have one argument in it:
  // AccessMode::wait_for_new_data.
  auto temperature =
      d.getScalarRegisterAccessor<float>("Oven.temperatureReadback", 0, {ChimeraTK::AccessMode::wait_for_new_data});

  // The read now blocks until data is received. You can use it to syncrchonise your code to incoming data. No need for
  // additional sleeps etc.
  while(true) {
    temperature.read();
    std ::cout << "The temperature is " << temperature << " degC. " << std ::endl;
  }
}
