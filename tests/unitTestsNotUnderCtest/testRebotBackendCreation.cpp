#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testRebotBackendCreation
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "RebotBackend.h"
#include "Device.h"

#include "Utilities.h"
#include "DMapFileParser.h"
#include "NumericAddress.h"

namespace mtca4u{
  using namespace ChimeraTK;
}

void checkWriteReadFromRegister(mtca4u::Device& rebotDevice);

BOOST_AUTO_TEST_SUITE(RebotDeviceTestSuite)


BOOST_AUTO_TEST_CASE( testFactoryForRebotDeviceCreation ){

  // set dmap file path
  auto dmapPathBackup = mtca4u::getDMapFilePath();
  mtca4u::setDMapFilePath("./dummies.dmap");


  // There are four situations where the map-file information is coming from
  // 1. From the dmap file (old way, third column in dmap file)
  // 2. From the URI (new, recommended, not supported by dmap parser at the moment)
  // 3. No map file at all (not supported by the dmap parser at the moment)
  // 4. Both dmap file and URI contain the information (prints a warning and takes the one from the dmap file)

  // 1. The original way with map file as third column in the dmap file
  mtca4u::Device rebotDevice;
  rebotDevice.open("mskrebot");
  checkWriteReadFromRegister(rebotDevice);
  
  mtca4u::Device rebotDevice2;
  // create another mskrebot
  rebotDevice2.open("mskrebot");
  checkWriteReadFromRegister(rebotDevice2);
  
  rebotDevice.write<double>("BOARD/WORD_USER", 48 );
  rebotDevice.close(); // we have to close this because

  // 2. Creating without map file in the dmap only works by putting an sdm on creation because we have to bypass the
  // dmap file parser which at the time of writing this requires a map file as third column
  mtca4u::Device secondDevice;
  secondDevice.open("sdm://./rebot=localhost,5001,mtcadummy_rebot.map");
  BOOST_CHECK( secondDevice.read<double>("BOARD/WORD_USER")== 48 );
  secondDevice.close();

  // 3. We don't have a map file, so we have to use numerical addressing
  mtca4u::Device thirdDevice;
  thirdDevice.open("sdm://./rebot=localhost,5001");
  BOOST_CHECK( thirdDevice.read<int32_t>(mtca4u::numeric_address::BAR/0/0xC)== 48<<3 ); // The user register is on bar 0, address 0xC.
                                                               // We have no fixed point data conversion but 3 fractional bits.
  thirdDevice.close();

  // 4. This should print a warning. We can't check that, so we just check that it does work like the other two options.
  mtca4u::Device fourthDevice;
  fourthDevice.open("REBOT_DOUBLEMAP");
  BOOST_CHECK( fourthDevice.read<double>("BOARD/WORD_USER")== 48 );

  // reset dmap path to what was at the start  of these tests
  mtca4u::setDMapFilePath(dmapPathBackup);
}

BOOST_AUTO_TEST_SUITE_END()

void checkWriteReadFromRegister(mtca4u::Device& rebotDevice) {
  int32_t dataToWrite[4] = {2, 3, 100, 20};
  int32_t readInData[4];

  // 0xDEADBEEF is a word preset by the dummy firmware in the WORD_COMPILATION
  // register (addr 0x04). reading and verifying this register means the read
  // api of device acces works for the rebot device.
  rebotDevice.readReg("WORD_COMPILATION", "BOARD", readInData, sizeof(int32_t));
  BOOST_CHECK_EQUAL(0xDEADBEEF, readInData[0]);

  // ADC.WORLD_CLK_MUX is a 4 word/element register, this test would verify
  // write to the device through the api works. (THe read command has been
  // established to work by the read of the preset word).
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", dataToWrite, sizeof(dataToWrite));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(readInData));
  for(int i = 0; i < 4; i++){
    BOOST_CHECK_EQUAL(dataToWrite[i], readInData[i]);
  }


  // test read from offset 2 on a multi word/element register.
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(int32_t), 2*sizeof(int32_t));
  BOOST_CHECK_EQUAL(dataToWrite[2], readInData[0]);

  // test write one element at offset position 2 on a multiword register.
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", dataToWrite, sizeof(int32_t), 2 * sizeof(int32_t));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(int32_t), 2 * sizeof(int32_t));
  BOOST_CHECK_EQUAL(dataToWrite[0], readInData[0]);


  // test writing a continuous block from offset 1 in a multiword register.
  int32_t data[2] = {7896, 45678};
  rebotDevice.writeReg("WORD_CLK_MUX", "ADC", data, sizeof(data), 1 * sizeof(int32_t));
  rebotDevice.readReg("WORD_CLK_MUX", "ADC", readInData, sizeof(data), 1 * sizeof(int32_t));
  for(int i = 0; i < 2; i++){
    BOOST_CHECK_EQUAL(data[i], readInData[i]);
  }

  // test writing a continuous block from offset 1 in a multiword register
  // through an accessor
  data[0] = 676; data[1] = 9987;
  auto accessor = rebotDevice.getRegisterAccessor("WORD_CLK_MUX", "ADC");
  accessor->write(data, 2, 1);
  accessor->read(readInData, 2, 1);
  for(int i = 0; i < 2; i++){
    BOOST_CHECK_EQUAL(data[i], readInData[i]);
  }


  // write to larger area using offsets in a loop
  auto testArea = rebotDevice.getRegisterAccessor(
      "TEST_AREA", "ADC"); // testArea is 1024 words long

  for (int i = 0; i < 10; ++i) {
    int value = i;
    testArea->write(&value, 1, i);
  }
  for (int i = 0; i < 10; ++i) {
    int value;
    testArea->read(&value, 1, i);
    BOOST_CHECK_EQUAL(value, i);
  }
}
