// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

///@todo FIXME My dynamic init header is a hack. Change the test to use
/// BOOST_AUTO_TEST_CASE!
#include "boost_dynamic_init_test.h"
using namespace boost::unit_test_framework;

#define PCIEDEV_TEST_SLOT 0
#define LLRFDRV_TEST_SLOT 4
#define PCIEUNI_TEST_SLOT 6

#include "BackendFactory.h"
#include "Device.h"
#include "Exception.h"
#include "NumericAddress.h"
#include "PcieBackend.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;
using ChimeraTK::numeric_address::BAR;

// constants for the registers and their contents. We keep the hard coded
// values at one place and only use the constants in the code below.
#define WORD_FIRMWARE_OFFSET 0x0
#define WORD_COMPILATION_OFFSET 0x4
#define WORD_USER_OFFSET 0xC
#define WORD_CLK_CNT_OFFSET 0x10
#define WORD_DUMMY_OFFSET 0x3C
#define DMMY_AS_ASCII 0x444D4D59
#define WORD_ADC_ENA_OFFSET 0x44
#define N_WORDS_DMA 25
#define PCIE_DEVICE "PCIE6"
#define LLRF_DEVICE "LLRF10"
//#define PCIE_UNI_DEVICE "PCIEUNI11"
#define PCIE_UNI_DEVICE "PCIE0"
#define NON_EXISTING_DEVICE "DUMMY9"

/*******************************************************************************************************************/

// Use a file lock on /var/run/lock/mtcadummy/<devicenode> for all device nodes
// we are using in this test, to ensure we are not running concurrent tests in
// parallel using the same kernel dummy drivers.
//
// Note: The lock is automatically released when the process terminates!
struct TestLocker {
  std::vector<std::string> usedNodes{"mtcadummys0", "llrfdummys4", "noioctldummys5", "pcieunidummys6"};

  TestLocker() {
    mkdir("/var/run/lock/mtcadummy",
        0777); // ignore errors intentionally, as directory might already exist
    for(auto& node : usedNodes) {
      std::string lockfile = "/var/run/lock/mtcadummy/" + node;

      // open dmap file for locking
      int fd = open(lockfile.c_str(), O_WRONLY | O_CREAT, 0777);
      if(fd == -1) {
        std::cout << "Cannot open file '" << lockfile << "' for locking." << std::endl;
        exit(1);
      }

      // obtain lock
      int res = flock(fd, LOCK_EX);
      if(res == -1) {
        std::cout << "Cannot acquire lock on file 'shareddummyTest.dmap'." << std::endl;
        exit(1);
      }
    }
  }

  ~TestLocker() {
    for(auto& node : usedNodes) {
      std::string lockfile = "/var/run/lock/mtcadummy/" + node;
      unlink(lockfile.c_str());
    }
  }
};
static TestLocker testLocker;

/*******************************************************************************************************************/

/** The unit tests for the PcieBackend class, based on the
 *  boost unit test library. We use a class which holds a private
 *  instance of the PcieBackend to be tested, which avoids code duplication
 *  (instantiating and opening the backend over and over again etc.)
 *
 *  The tests should be run in the order stated here. At least testOpen()
 *  has to be executed first, and testClose() has to be executed last.
 *  Further dependencies are implemented in the teste suite.
 */
static BackendFactory& factoryInstance = BackendFactory::getInstance();
class PcieBackendTest {
 public:
  PcieBackendTest(std::string const& deviceFileName, unsigned int slot);

  /** A simple test which calls the default constructor and checks that the
   * backend is closed. We keep this separate because in principle constructors
   * might throw, and this should explicitly be tested. But not within the
   * instances of the test class, so it's a static function.
   */
  static void testConstructor();

  /** Tests whether openig of the backend works, and that the exception is
   *  thrown correctly if the backend cannot be opened.
   */

  void testFailIfClosed();

  // Try Creating a backend and check if it is connected.
  void testCreateBackend();

  // Try opening the created backend and check it's open status.
  void testOpen();

  // Try closing the created backend and check it's open status.
  void testClose();

  void testRead();
  void testWriteArea();

  void testReadRegister();
  void testWriteRegister();

  void testReadDMA();
  void testWriteDMA();

  void testReadDeviceInfo();
  /// Test that all functions throw an exception if the backend is not opened.
  void testFailIfBackendClosed();

 private:
  PcieBackend _pcieBackend;
  std::string _deviceFileName;
  unsigned int _slot;

  boost::shared_ptr<PcieBackend> _pcieBackendInstance;

  // Internal function for better code readablility.
  // Returns an error message. If the message is empty the test succeeded.
  std::string checkDmaValues(std::vector<int32_t> const& dmaBuffer);
};

/*******************************************************************************************************************/

class PcieBackendTestSuite : public test_suite {
 public:
  PcieBackendTestSuite(std::string const& deviceFileName, unsigned int slot) : test_suite("PcieBackend test suite") {
    BackendFactory::getInstance().setDMapFilePath(TEST_DMAP_FILE_PATH);
    // add member function test cases to a test suite
    boost::shared_ptr<PcieBackendTest> pcieBackendTest(new PcieBackendTest(deviceFileName, slot));

    test_case* createBackendTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testCreateBackend, pcieBackendTest);
    test_case* openTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testOpen, pcieBackendTest);

    test_case* readTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testRead, pcieBackendTest);
    test_case* writeAreaTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testWriteArea, pcieBackendTest);

    test_case* readRegisterTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testReadRegister, pcieBackendTest);
    test_case* writeRegisterTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testWriteRegister, pcieBackendTest);

    test_case* readDMATestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testReadDMA, pcieBackendTest);
    test_case* writeDMATestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testWriteDMA, pcieBackendTest);

    test_case* readDeviceInfoTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testReadDeviceInfo, pcieBackendTest);

    test_case* closeTestCase = BOOST_CLASS_TEST_CASE(&PcieBackendTest::testClose, pcieBackendTest);

    test_case* testConstructor = BOOST_TEST_CASE(&PcieBackendTest::testConstructor);

    createBackendTestCase->depends_on(testConstructor);
    openTestCase->depends_on(createBackendTestCase);
    readTestCase->depends_on(openTestCase);
    writeAreaTestCase->depends_on(readTestCase);
    readRegisterTestCase->depends_on(writeAreaTestCase);
    writeRegisterTestCase->depends_on(readRegisterTestCase);
    readDMATestCase->depends_on(writeRegisterTestCase);
    writeDMATestCase->depends_on(readDMATestCase);
    readDeviceInfoTestCase->depends_on(writeDMATestCase);
    closeTestCase->depends_on(readDeviceInfoTestCase);

    add(testConstructor);

    add(createBackendTestCase);
    add(openTestCase);

    add(readTestCase);
    add(writeAreaTestCase);

    add(readRegisterTestCase);
    add(writeRegisterTestCase);

    add(readDMATestCase);
    add(writeDMATestCase);

    add(readDeviceInfoTestCase);

    add(closeTestCase);
  }

 private:
};

/*******************************************************************************************************************/

bool init_unit_test() {
  framework::master_test_suite().p_name.value = "PcieBackend test suite";

  std::stringstream llrfdummyFileName;
  llrfdummyFileName << "/dev/llrfdummys" << LLRFDRV_TEST_SLOT;
  // framework::master_test_suite().add( new
  // PcieBackendTestSuite(llrfdummyFileName.str(), LLRFDRV_TEST_SLOT) );
  // framework::master_test_suite().add( new PcieBackendTestSuite(LLRF_DEVICE,
  // LLRFDRV_TEST_SLOT) );

  std::stringstream mtcadummyFileName;
  mtcadummyFileName << "/dev/mtcadummys" << PCIEDEV_TEST_SLOT;
  // framework::master_test_suite().add( new PcieBackendTestSuite(PCIE_DEVICE,
  // PCIEDEV_TEST_SLOT) );

  std::stringstream pcieunidummyFileName;
  pcieunidummyFileName << "/dev/pcieunidummys" << PCIEUNI_TEST_SLOT;
  framework::master_test_suite().add(new PcieBackendTestSuite(PCIE_UNI_DEVICE, PCIEUNI_TEST_SLOT));

  return true;
}

/*******************************************************************************************************************/

// The implementations of the individual tests

void PcieBackendTest::testConstructor() {
  std::cout << "testConstructor" << std::endl;
  PcieBackend pcieBackend("");
  BOOST_CHECK(pcieBackend.isOpen() == false);
}

PcieBackendTest::PcieBackendTest(std::string const& deviceFileName, unsigned int slot)
: _pcieBackend(deviceFileName), _deviceFileName(deviceFileName), _slot(slot) {}

std::string PcieBackendTest::checkDmaValues(std::vector<int32_t> const& dmaBuffer) {
  std::cout << "testDmaValues" << std::endl;
  bool dmaValuesOK = true;
  size_t i; // we need this after the loop
  for(i = 0; i < dmaBuffer.size(); ++i) {
    if(dmaBuffer[i] != static_cast<int32_t>(i * i)) {
      dmaValuesOK = false;
      break;
    }
  }

  if(dmaValuesOK) {
    return std::string(); // an empty string means test is ok
  }

  std::stringstream errorMessage;
  errorMessage << "Content of transferred DMA block is not valid. First wrong "
                  "value at index "
               << i << " is " << dmaBuffer[i] << std::endl;

  return errorMessage.str();
}

/*******************************************************************************************************************/

void PcieBackendTest::testReadDeviceInfo() {
  std::cout << "testReadDeviceInfo" << std::endl;
  // The device info returns slot and driver version (major and minor).
  // For the dummy major and minor are the same as firmware and compilation,
  // respectively.
  int32_t major;
  //_pcieBackendInstance->readReg(WORD_FIRMWARE_OFFSET, &major, /*bar*/ 0);
  _pcieBackendInstance->read(/*bar*/ 0, WORD_FIRMWARE_OFFSET, &major, 4);
  int32_t minor;
  //_pcieBackendInstance->readReg(WORD_COMPILATION_OFFSET, &minor, /*bar*/ 0);
  _pcieBackendInstance->read(/*bar*/ 0, WORD_COMPILATION_OFFSET, &minor, 4);
  std::stringstream referenceInfo;
  referenceInfo << "SLOT: " << _slot << " DRV VER: " << major << "." << minor;

  std::string deviceInfo;
  deviceInfo = _pcieBackendInstance->readDeviceInfo();
  BOOST_CHECK_EQUAL(referenceInfo.str(), deviceInfo);
}

/*******************************************************************************************************************/

void PcieBackendTest::testReadDMA() {
  std::cout << "testReadDMA" << std::endl;
  // Start the ADC on the dummy device. This will fill the "DMA" buffer with the
  // default values (index^2) in the first 25 words.
  //_pcieBackendInstance->writeReg(/*bar*/ 0, WORD_ADC_ENA_OFFSET, 1);
  int32_t data = 1;
  _pcieBackendInstance->write(/*bar*/ 0, WORD_ADC_ENA_OFFSET, &data, 4);

  std::vector<int32_t> dmaUserBuffer(N_WORDS_DMA, -1);

  _pcieBackendInstance->read(/*the dma bar*/ 2, /*offset*/ 0, &dmaUserBuffer[0], N_WORDS_DMA * sizeof(int32_t));

  std::string errorMessage = checkDmaValues(dmaUserBuffer);
  BOOST_CHECK_MESSAGE(errorMessage.empty(), errorMessage);

  // test dma with offset
  // read 20 words from address 5
  std::vector<int32_t> smallBuffer(20, -1);
  static const unsigned int readOffset = 5;
  _pcieBackendInstance->read(
      /*the dma bar*/ 2, /*offset*/ readOffset * sizeof(int32_t), &smallBuffer[0],
      smallBuffer.size() * sizeof(int32_t));

  for(size_t i = 0; i < smallBuffer.size(); ++i) {
    BOOST_CHECK(smallBuffer[i] == static_cast<int32_t>((i + readOffset) * (i + readOffset)));
  }
}

/*******************************************************************************************************************/

void PcieBackendTest::testWriteDMA() {
  std::cout << "testWriteDMA" << std::endl;
}

/*******************************************************************************************************************/

void PcieBackendTest::testRead() {
  std::cout << "testRead" << std::endl;
  // FIXME: Change the driver to have the standard register set and adapt this
  // code

  // Read the first two words, which are WORD_FIRMWARE and WORD_COMPILATION
  // We checked that single reading worked, so we use it to create the
  // reference.
  int32_t firmwareContent;
  _pcieBackendInstance->read(/*bar*/ 0, WORD_FIRMWARE_OFFSET, &firmwareContent, 4);
  int32_t compilationContent;
  _pcieBackendInstance->read(/*bar*/ 0, WORD_COMPILATION_OFFSET, &compilationContent, 4);

  // Now try reading them as area
  int32_t twoWords[2];
  twoWords[0] = 0xFFFFFFFF;
  twoWords[1] = 0xFFFFFFFF;

  _pcieBackendInstance->read(/*bar*/ 0, WORD_FIRMWARE_OFFSET, twoWords, 2 * sizeof(int32_t));
  BOOST_CHECK((twoWords[0] == firmwareContent) && (twoWords[1] == compilationContent));

  // now try to read only six of the eight bytes. This should throw an exception
  // because it is not a multiple of 4.
  BOOST_CHECK_THROW(
      _pcieBackendInstance->read(/*bar*/ 0, /*offset*/ 0, twoWords, /*nBytes*/ 6), ChimeraTK::runtime_error);

  // also check another bar
  // Start the ADC on the dummy device. This will fill bar 2 (the "DMA" buffer)
  // with the default values (index^2) in the first 25 words.
  int32_t data = 1;
  _pcieBackendInstance->write(/*bar*/ 0, WORD_ADC_ENA_OFFSET, &data, 4);
  // use the same test as for DMA
  std::vector<int32_t> bar2Buffer(N_WORDS_DMA, -1);
  _pcieBackendInstance->read(/*the dma bar*/ 2, /*offset*/ 0, &bar2Buffer[0], N_WORDS_DMA * sizeof(int32_t));

  std::string errorMessage = checkDmaValues(bar2Buffer);
  BOOST_CHECK_MESSAGE(errorMessage.empty(), errorMessage);
}

/*******************************************************************************************************************/

void PcieBackendTest::testWriteArea() {
  std::cout << "testWriteArea" << std::endl;
  // FIXME: Change the driver to have the standard register set and adapt this
  // code

  // Read the two WORD_CLK_CNT words, write them and read them back
  int32_t originalClockCounts[2];
  int32_t increasedClockCounts[2];
  int32_t readbackClockCounts[2];

  _pcieBackendInstance->read(/*bar*/ 0, WORD_CLK_CNT_OFFSET, originalClockCounts, 2 * sizeof(int32_t));
  increasedClockCounts[0] = originalClockCounts[0] + 1;
  increasedClockCounts[1] = originalClockCounts[1] + 1;
  _pcieBackendInstance->write(/*bar*/ 0, WORD_CLK_CNT_OFFSET, increasedClockCounts, 2 * sizeof(int32_t));
  _pcieBackendInstance->read(/*bar*/ 0, WORD_CLK_CNT_OFFSET, readbackClockCounts, 2 * sizeof(int32_t));
  BOOST_CHECK(
      (increasedClockCounts[0] == readbackClockCounts[0]) && (increasedClockCounts[1] == readbackClockCounts[1]));

  // now try to write only six of the eight bytes. This should throw an
  // exception because it is not a multiple of 4.
  BOOST_CHECK_THROW(_pcieBackendInstance->write(/*bar*/ 0, WORD_CLK_CNT_OFFSET, originalClockCounts,
                        /*nBytes*/ 6),
      ChimeraTK::runtime_error);

  // also test another bar (area in bar 2), the usual drill: write and read
  // back, we know that reading works from the previous test
  std::vector<int32_t> writeBuffer(N_WORDS_DMA, 0xABCDEF01);
  std::vector<int32_t> readbackBuffer(N_WORDS_DMA, -1);
  _pcieBackendInstance->write(/*bar*/ 2, 0, &writeBuffer[0], N_WORDS_DMA * sizeof(int32_t));
  _pcieBackendInstance->read(/*bar*/ 2, 0, &readbackBuffer[0], N_WORDS_DMA * sizeof(int32_t));
  BOOST_CHECK(readbackBuffer == writeBuffer);
}

/*******************************************************************************************************************/

void PcieBackendTest::testReadRegister() {
  std::cout << "testReadRegister" << std::endl;
  // FIXME: Change the driver to have the standard register set and adapt this
  // code

  // read the WORD_COMPILATION register in bar 0. It's value is not 0.
  int32_t dataWord = 0; // initialise with 0 so we can check if reading the content works.

  _pcieBackendInstance->open(); // no need to check if this works because we did
                                // the open test first
  //_pcieBackendInstance->readReg(WORD_DUMMY_OFFSET, &dataWord, /*bar*/ 0);
  _pcieBackendInstance->read(/*bar*/ 0, WORD_DUMMY_OFFSET, &dataWord, 4);
  BOOST_CHECK_EQUAL(dataWord, DMMY_AS_ASCII);

  /** There has to be an exception if the bar is wrong. 6 is definitely out of
   * range. */
  // BOOST_CHECK_THROW( _pcieBackendInstance->readReg(WORD_DUMMY_OFFSET,
  // &dataWord, /*bar*/ 6),
  BOOST_CHECK_THROW(_pcieBackendInstance->getRegisterAccessor<int>("#6/0x3C", 4, 0, {}), ChimeraTK::logic_error);
}

/*******************************************************************************************************************/

void PcieBackendTest::testWriteRegister() {
  std::cout << "testWriteRegister" << std::endl;
  // FIXME: Change the driver to have the standard register set and adapt this
  // code

  // We read the user register, increment it by one, write it and reread it.
  // As we checked that reading work, this is a reliable test that writing is
  // ok.
  int32_t originalUserWord, newUserWord;
  _pcieBackendInstance->read(/*bar*/ 0, WORD_USER_OFFSET, &originalUserWord, 4);
  int32_t data = originalUserWord + 1;
  _pcieBackendInstance->write(/*bar*/ 0, WORD_USER_OFFSET, &data, 4);
  _pcieBackendInstance->read(/*bar*/ 0, WORD_USER_OFFSET, &newUserWord, 4);

  BOOST_CHECK_EQUAL(originalUserWord + 1, newUserWord);
}

/*******************************************************************************************************************/

void PcieBackendTest::testClose() {
  std::cout << "testClose" << std::endl;
  /* Try closing the backend */
  _pcieBackendInstance->close();
  /* backend should not be open now */
  BOOST_CHECK(_pcieBackendInstance->isOpen() == false);
  // It always has to be possible to call close again.
  _pcieBackendInstance->close();
  BOOST_CHECK(_pcieBackendInstance->isOpen() == false);
}

/*******************************************************************************************************************/

void PcieBackendTest::testOpen() {
  std::cout << "testOpen" << std::endl;
  _pcieBackendInstance->open();
  BOOST_CHECK(_pcieBackendInstance->isOpen() == true);
  // It must always be possible to re-open a backend. It should try to re-connect.
  _pcieBackendInstance->open();
  BOOST_CHECK(_pcieBackendInstance->isOpen());
}

/*******************************************************************************************************************/

void PcieBackendTest::testCreateBackend() {
  std::cout << "testCreateBackend" << std::endl;
  /** Try creating a non existing backend */
  BOOST_CHECK_THROW(factoryInstance.createBackend(NON_EXISTING_DEVICE), ChimeraTK::logic_error);
  /** Try creating an existing backend */
  _pcieBackendInstance = boost::dynamic_pointer_cast<PcieBackend>(factoryInstance.createBackend(_deviceFileName));
  BOOST_CHECK(_pcieBackendInstance != nullptr);
  /** Backend should not be in open state */
  BOOST_CHECK(_pcieBackendInstance->isOpen() == false);

  // OK, now that we know that basic creation is working let's do some tests of
  // the specifics of the create function. We use the device interface because
  // it is much more convenient.

  // There are four situations where the map-file information is coming from
  // 1. From the dmap file (old way, third column in dmap file)
  // 2. From the URI (new, recommended, not supported by dmap parser at the
  // moment)
  // 3. No map file at all (not supported by the dmap parser at the moment)
  // 4. Both dmap file and URI contain the information (prints a warning and
  // takes the one from the dmap file)

  // 1. The original way with map file as third column in the dmap file
  Device firstDevice;
  firstDevice.open("PCIE0");
  // this backend is without module in the register name
  firstDevice.write<double>("WORD_USER", 48);
  BOOST_CHECK(true);

  // 2. Creating without map file in the dmap only works by putting an sdm on
  // creation because we have to bypass the dmap file parser which at the time
  // of writing this requires a map file as third column
  Device secondDevice;
  secondDevice.open("sdm://./pci:pcieunidummys6=mtcadummy.map");
  BOOST_CHECK(secondDevice.read<double>("BOARD/WORD_USER") == 48);

  Device secondDevice2;
  // try opening same device again.
  secondDevice2.open("sdm://./pci:pcieunidummys6=mtcadummy.map");
  BOOST_CHECK(secondDevice2.read<double>("BOARD/WORD_USER") == 48);

  // 3. We don't have a map file, so we have to use numerical addressing
  Device thirdDevice;
  thirdDevice.open("sdm://./pci:pcieunidummys6");
  BOOST_CHECK(thirdDevice.read<int32_t>(BAR() / 0 / 0xC) == 48 << 3); // The user register is on bar 0, address 0xC.
                                                                      // We have no fixed point data conversion but 3
                                                                      // fractional bits.

  // 4. This should print a warning. We can't check that, so we just check that
  // it does work like the other two options.
  Device fourthDevice;
  fourthDevice.open("PCIE_DOUBLEMAP");
  BOOST_CHECK(fourthDevice.read<double>("BOARD/WORD_USER") == 48);

  // close the backend for the following tests. One of the Devices has opened
  // it...
  _pcieBackendInstance->close();
}

/*******************************************************************************************************************/
