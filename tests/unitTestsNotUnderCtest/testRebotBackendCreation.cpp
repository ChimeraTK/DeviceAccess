// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testRebotBackendCreation
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DMapFileParser.h"
#include "NumericAddress.h"
#include "RebotBackend.h"
#include "Utilities.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}

void checkWriteReadFromRegister(ChimeraTK::Device& rebotDevice);

BOOST_AUTO_TEST_SUITE(RebotDeviceTestSuite)

BOOST_AUTO_TEST_CASE(testFactoryForRebotDeviceCreation) {
  // set dmap file path
  auto dmapPathBackup = ChimeraTK::getDMapFilePath();
  std::string dmapPath{"./dummies.dmap"};
  if(framework::master_test_suite().argc > 1) {
    dmapPath = framework::master_test_suite().argv[1];
  }
  ChimeraTK::setDMapFilePath(dmapPath);
  std::string port{"5001"};
  if(framework::master_test_suite().argc > 2) {
    port = framework::master_test_suite().argv[2];
  }

  // There are four situations where the map-file information is coming from
  // 1. From the dmap file (old way, third column in dmap file)
  // 2. From the URI (new, recommended, not supported by dmap parser at the
  // moment)
  // 3. No map file at all (not supported by the dmap parser at the moment)
  // 4. Both dmap file and URI contain the information (prints a warning and
  // takes the one from the dmap file)

  // 1. The original way with map file as third column in the dmap file
  ChimeraTK::Device rebotDevice;
  rebotDevice.open("mskrebot");
  checkWriteReadFromRegister(rebotDevice);

  ChimeraTK::Device rebotDevice2;
  // create another mskrebot
  rebotDevice2.open("mskrebot");
  checkWriteReadFromRegister(rebotDevice2);

  rebotDevice.write<double>("BOARD/WORD_USER", 48);
  rebotDevice.close(); // we have to close this because

  // 2. Creating without map file in the dmap only works by putting an sdm on
  // creation because we have to bypass the dmap file parser which at the time
  // of writing this requires a map file as third column
  ChimeraTK::Device secondDevice;
  secondDevice.open("sdm://./rebot=localhost," + port + ",mtcadummy_rebot.map");
  BOOST_CHECK(secondDevice.read<double>("BOARD/WORD_USER") == 48);
  secondDevice.close();

  // 3. We don't have a map file, so we have to use numerical addressing
  ChimeraTK::Device thirdDevice;
  thirdDevice.open("sdm://./rebot=localhost," + port);
  BOOST_CHECK(thirdDevice.read<int32_t>(ChimeraTK::numeric_address::BAR() / 0 / 0xC) == 48 << 3); // The user register
                                                                                                  // is on bar 0,
                                                                                                  // address 0xC. We
                                                                                                  // have no fixed point
                                                                                                  // data conversion but
                                                                                                  // 3 fractional bits.
  thirdDevice.close();

  // 4. This should print a warning. We can't check that, so we just check that
  // it does work like the other two options.
  ChimeraTK::Device fourthDevice;
  fourthDevice.open("REBOT_DOUBLEMAP");
  BOOST_CHECK(fourthDevice.read<double>("BOARD/WORD_USER") == 48);

  // reset dmap path to what was at the start  of these tests
  ChimeraTK::setDMapFilePath(dmapPathBackup);
}

BOOST_AUTO_TEST_SUITE_END()

void checkWriteReadFromRegister(ChimeraTK::Device& rebotDevice) {
  std::vector<int32_t> dataToWrite({2, 3, 100, 20});

  // 0xDEADBEEF is a word preset by the dummy firmware in the WORD_COMPILATION
  // register (addr 0x04). reading and verifying this register means the read
  // api of device acces works for the rebot device.
  BOOST_CHECK_EQUAL(rebotDevice.read<uint32_t>("BOARD/WORD_COMPILATION"), 0xDEADBEEF);

  // ADC.WORLD_CLK_MUX is a 4 word/element register, this test would verify
  // write to the device through the api works. (THe read command has been
  // established to work by the read of the preset word).
  rebotDevice.write("ADC/WORD_CLK_MUX", dataToWrite);
  BOOST_CHECK(rebotDevice.read<int>("ADC/WORD_CLK_MUX", 4) == dataToWrite);

  // test read from offset 2 on a multi word/element register.
  auto acc1 = rebotDevice.getScalarRegisterAccessor<int32_t>("ADC/WORD_CLK_MUX", 2);
  acc1.read();
  BOOST_CHECK_EQUAL(dataToWrite[2], static_cast<int32_t>(acc1));

  // test write one element at offset position 2 on a multiword register.
  acc1 = dataToWrite[0];
  acc1.write();
  acc1 = 0;
  acc1.read();
  BOOST_CHECK_EQUAL(dataToWrite[0], static_cast<int32_t>(acc1));

  // test writing a continuous block from offset 1 in a multiword register.
  auto acc2 = rebotDevice.getOneDRegisterAccessor<int32_t>("ADC/WORD_CLK_MUX", 2, 1);
  acc2 = std::vector<int32_t>({676, 9987});
  acc2.write();
  acc2 = std::vector<int32_t>({0, 0});
  acc2.read();
  BOOST_CHECK_EQUAL(acc2[0], 676);
  BOOST_CHECK_EQUAL(acc2[1], 9987);

  // write to larger area
  auto testArea = rebotDevice.getOneDRegisterAccessor<int32_t>("ADC/TEST_AREA"); // testArea is 1024 words long
  for(int i = 0; i < 10; ++i) {
    testArea[i] = i;
  }
  testArea.write();
  testArea.read();
  for(int i = 0; i < 10; ++i) {
    BOOST_CHECK_EQUAL(testArea[i], i);
  }
}
