// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SubdeviceBackendUnifiedTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
#include "LogicalNameMappingBackend.h"
#include "UnifiedBackendTest.h"

#include <boost/thread/barrier.hpp>

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendUnifiedTestSuite)

/**********************************************************************************************************************/

/**
 * dummy backend used for testing the double buffering handshake.
 * a double-buffer read consists of (write ctrl, read buffernumber, read other buffer, write ctrl)
 * The overwritten functions of this class refer to the inner protocol
 */
struct DummyForDoubleBuffering : public ExceptionDummy {
  using ExceptionDummy::ExceptionDummy;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return returnInstance<DummyForDoubleBuffering>(
        parameters.at("map"), convertPathRelativeToDmapToAbs(parameters.at("map")));
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "DummyForDoubleBuffering", &DummyForDoubleBuffering::createInstance, {"map"});
    }
  };

  void read(uint64_t bar, uint64_t address, int32_t* data, size_t sizeInBytes) override {
    // Note, although ExceptionDummy::read() cannot be called concurrently with read or write from the
    // fw simulating side, this limitation should not matter here since we only interrupt
    // DummyForDoubleBuffering::read() and not it's base implementation

    for(unsigned i = 0; i < 2; i++) {
      if(blockNextRead[i]) {
        blockedInRead[i].wait();
        unblockRead[i].wait();
        blockNextRead[i] = false;
      }
    }
    // finalize reading by calling Exception backend read
    ChimeraTK::ExceptionDummy::read(bar, address, data, sizeInBytes);
  }
  // use this to request that next read blocks.
  // array index corresponds to that of barrier arrays, on pair of barriers per reader thread we control.
  // We know that read is called only 2nd after
  // write (for the buffer-switching enable ctrl register), so in this sense it requests blocking
  // after only part of the double-buffer read operation is done
  static thread_local bool blockNextRead[2];
  // after request that read blocks, you must wait on this
  std::array<boost::barrier, 2> blockedInRead{boost::barrier{2}, boost::barrier{2}};
  // use this to unblock the read
  std::array<boost::barrier, 2> unblockRead{boost::barrier{2}, boost::barrier{2}};
};
thread_local bool DummyForDoubleBuffering::blockNextRead[2] = {false, false};

static DummyForDoubleBuffering::BackendRegisterer gDFDBRegisterer;

static std::string rawDeviceCdd("(DummyForDoubleBuffering?map=doubleBuffer.map)");
std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + rawDeviceCdd + ")";
static auto backdoor =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(rawDeviceCdd));

/**********************************************************************************************************************/

template<typename Register>
struct AreaType : Register {
  static uint32_t _currentBufferNumber;

  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {/* TODO ChimeraTK::AccessMode::wait_for_new_data */}; }
  size_t nChannels() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableTestWriteNeverLosesData()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestRawTransfer();

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    auto values = this->getRemoteValue<typename Register::minimumUserType>();
    for(size_t i = 0; i < this->nChannels(); ++i)
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        values[i][j] += this->increment * (i + j + 1);
      }
    return values;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue(bool = false) {
    // For Variables we don't have a backdoor. We have to use the normal read and write
    // functions which are good enough. It seems like a self consistency test, but all
    // functionality the variable has to provide is that I can write something, and
    // read it back, which is tested with it.

    // We might have to open the backend to perform the operation. We have to remember
    // that we did so and close it again it we did. Some tests require the backend to be closed.

    auto currentBufferNumber = backdoor->getRegisterAccessor<uint32_t>("APP.1.WORD_DUB_BUF_CURR", 0, 0, {});
    auto buffer0 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF0", this->nElementsPerChannel(), 0, {});
    auto buffer1 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF1", this->nElementsPerChannel(), 0, {});

    bool deviceWasOpened = false;
    if(!backdoor->isOpen()) {
      backdoor->open();
      deviceWasOpened = true;
    }

    boost::shared_ptr<NDRegisterAccessor<typename Register::minimumUserType>> currentBuffer;

    currentBufferNumber->read();

    if(currentBufferNumber->accessData(0) == 1) {
      currentBuffer = buffer0;
    }
    else {
      currentBuffer = buffer1;
    }
    currentBuffer->read();
    std::vector<std::vector<UserType>> v;
    for(size_t i = 0; i < this->nChannels(); ++i) {
      v.push_back(std::vector<UserType>());
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        v[i].push_back(currentBuffer->accessData(j));
      }
    }

    if(deviceWasOpened) {
      backdoor->close();
    }

    return v;
  }

  void setRemoteValue() {
    auto currentBufferNumber = backdoor->getRegisterAccessor<uint32_t>("APP.1.WORD_DUB_BUF_CURR", 0, 0, {});
    auto buffer0 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF0", this->nElementsPerChannel(), 0, {});
    auto buffer1 = backdoor->getRegisterAccessor<typename Register::minimumUserType>(
        "APP/0/DAQ0_BUF1", this->nElementsPerChannel(), 0, {});
    boost::shared_ptr<NDRegisterAccessor<typename Register::minimumUserType>> currentBuffer;

    bool deviceWasOpened = false;
    if(!backdoor->isOpen()) {
      backdoor->open();
      deviceWasOpened = true;
    }

    currentBufferNumber->accessData(0) = _currentBufferNumber;
    currentBufferNumber->write();
    _currentBufferNumber = _currentBufferNumber ? 0 : 1; // change current buffer no. 0->1 or 1->0

    auto values = this->generateValue<typename Register::minimumUserType>();

    if(currentBufferNumber->accessData(0) == 1) {
      currentBuffer = buffer0;
    }
    else {
      currentBuffer = buffer1;
    }
    for(size_t i = 0; i < this->nChannels(); ++i) {
      for(size_t j = 0; j < this->nElementsPerChannel(); ++j) {
        currentBuffer->accessData(i, j) = values[i][j];
      }
    }
    currentBuffer->write();

    if(deviceWasOpened) {
      backdoor->close();
    }
  }

  void setForceRuntimeError(bool enable, size_t caseNumber) {
    if(caseNumber == 0) {
      backdoor->throwExceptionRead = enable;
      backdoor->throwExceptionOpen = enable;
    }
  }
};

/*********************************************************************************************************************/

struct MyArea1 {
  std::string path() { return "/doubleBuffer"; }
  size_t nElementsPerChannel() { return 10; }
  size_t address() { return 20; }
  int32_t increment = 3;

  typedef uint32_t minimumUserType;
  typedef int32_t rawUserType;
};

/*********************************************************************************************************************/

template<typename Register>
uint32_t AreaType<Register>::_currentBufferNumber = 0;

BOOST_AUTO_TEST_CASE(testUnified) {
  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyArea1>>().runTests(lmap);
}

struct DeviceFixture {
  Device d;
  boost::shared_ptr<NDRegisterAccessor<uint32_t>> doubleBufferingEnabled;
  // we call the backend frontdoor when we modify the behavior of the thread which reads via double buffering mechanism
  boost::shared_ptr<DummyForDoubleBuffering> frontdoor;
  DeviceFixture() : d(lmap) {
    // before any access, also via backdoor, must open
    d.open();
    frontdoor = boost::dynamic_pointer_cast<DummyForDoubleBuffering>(backdoor);
    assert(frontdoor);
    doubleBufferingEnabled = backdoor->getRegisterAccessor<uint32_t>("APP/1/WORD_DUB_BUF_ENA", 0, 0, {});
    doubleBufferingEnabled->accessData(0) = 1;
    doubleBufferingEnabled->write();
  }
};

BOOST_FIXTURE_TEST_CASE(testSlowReader, DeviceFixture) {
  /*
   * Test race condition: slow reader, which blocks the Firmware from buffer switching.
   */
  auto accessor = d.getOneDRegisterAccessor<uint32_t>("/doubleBuffer");

  // make double buffer operation block after write to ctrl register, at read of buffer number
  std::thread s([&] {
    // this thread reads from double-buffered region
    frontdoor->blockNextRead[0] = true;
    accessor.read();
  });

  // wait that thread s is in blocked double-buffer read
  frontdoor->blockedInRead[0].wait();

  // simplification: instead of writing fw simulation which would overwrite data now,
  // just check that buffer switching was disabled
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(!doubleBufferingEnabled->accessData(0));

  frontdoor->unblockRead[0].wait();
  s.join();

  // check that buffer switching enabled, by finalization of double-buffered read
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(doubleBufferingEnabled->accessData(0));
}

BOOST_FIXTURE_TEST_CASE(testConcurrentRead, DeviceFixture) {
  /*
   * A test which exposes the dangerous race condition of two readers
   * - reader A deactivates buffer switching, starts reading buffer0
   * - reader B (again) deactivates buffer switching, starts reading buffer0
   * - reader A finished with reading, activates buffer switching already, which is too early
   *   here the correct double buffering implementation would need to wait on reader B
   * - firmware writes into buffer1 and when done, switches buffers
   *     the writing may have started earlier (e.g. before reader A started reading), important here is
   *     only buffer switch at end
   * - firmware writes into buffer0 and corrupts data
   * - reader B finishes reading, and gets corrupt data, enables buffer switching.
   */

  // wait on "reader B has started" and then wait on "reader A has finished" inside reader B

  std::thread readerA([&] {
    auto accessor = d.getOneDRegisterAccessor<uint32_t>("/doubleBuffer");
    // begin read
    frontdoor->blockNextRead[0] = true;
    accessor.read();
  });
  std::thread readerB([&] {
    auto accessor = d.getOneDRegisterAccessor<uint32_t>("/doubleBuffer");
    // wait that readerA is in blocked double-buffer read
    frontdoor->blockedInRead[0].wait();
    // begin read
    frontdoor->blockNextRead[1] = true;
    accessor.read();
  });
  frontdoor->blockedInRead[1].wait(); // wait that reader B also in blocked read
  frontdoor->unblockRead[0].wait();   // this is for reader A
  readerA.join();

  // check that after reader A returned, buffer switching is still disabled
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(!doubleBufferingEnabled->accessData(0));

  frontdoor->unblockRead[1].wait(); // this is for reader B
  // check that after reader B returned, buffer switching is enabled
  readerB.join();
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(doubleBufferingEnabled->accessData(0));
}

/**
 *  DeviceFixture used for the 2D access tests
 *  here no overwriting of ExceptionBackend
 */
struct DeviceFixture2D {
  // TODO - switch back to normal dummy?
  const std::string rawDeviceCdd = "(sharedMemoryDummy?map=doubleBuffer.map)";
  const std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + this->rawDeviceCdd + ")";
  Device d;
  boost::shared_ptr<DeviceBackend> backdoor;
  boost::shared_ptr<NDRegisterAccessor<uint32_t>> doubleBufferingEnabled, writingBufferNum;
  boost::shared_ptr<NDRegisterAccessor<float>> buf0, buf1;
  size_t wordOffset = 2; // must match xlmap

  DeviceFixture2D() : d(this->lmap) {
    // before any access, also via backdoor, must open
    d.open();
    backdoor = BackendFactory::getInstance().createBackend(this->rawDeviceCdd);
    doubleBufferingEnabled = backdoor->getRegisterAccessor<uint32_t>("DAQ0/WORD_DUB_BUF_ENA", 1, wordOffset, {});
    doubleBufferingEnabled->accessData(0) = 1;
    doubleBufferingEnabled->write();

    writingBufferNum =
        backdoor->getRegisterAccessor<uint32_t>("DAQ0/WORD_DUB_BUF_CURR/DUMMY_WRITEABLE", 1, wordOffset, {});
    buf0 = backdoor->getRegisterAccessor<float>("APP0/DAQ0_BUF0", 0, 0, {});
    buf1 = backdoor->getRegisterAccessor<float>("APP0/DAQ0_BUF1", 0, 0, {});
  }
};

// TODO - check whether . in register names causes problems
BOOST_FIXTURE_TEST_CASE(testExtractedChannels1, DeviceFixture2D) {
  /*
   * simple test for access to extracted channels of multiplexed 2D region
   * here focus is on deconstruction since we have a bug there
   */
  writingBufferNum->accessData(0) = 1;
  writingBufferNum->write();

  float modulation = 4.2;
  buf0->accessData(0, 0) = modulation;
  buf1->accessData(0, 0) = 2 * modulation;
  buf0->write();
  buf1->write();

  boost::barrier waitForBufferSwap{2};
  std::thread readerA([&] {
    auto accessorA = d.getOneDRegisterAccessor<float>("/modulation1");
    accessorA.readLatest();
    BOOST_CHECK_CLOSE(accessorA[0], modulation, 1e-4);
    waitForBufferSwap.wait();
    accessorA.readLatest();
    BOOST_CHECK_CLOSE(accessorA[0], 2 * modulation, 1e-4);
  });

  writingBufferNum->accessData(0) = 0;
  writingBufferNum->write();
  waitForBufferSwap.wait();

  readerA.join();
}

BOOST_FIXTURE_TEST_CASE(testExtractedChannels2, DeviceFixture2D) {
  /*
   * test access to extracted channels of multiplexed 2D region
   * this is an application of concurrent readers
   * also test indirection via target=this
   */

  writingBufferNum->accessData(0) = 1;
  writingBufferNum->write();

  float modulation = 4.2;
  float correction = 10.1;
  buf0->accessData(0, 0) = modulation;
  buf1->accessData(0, 0) = 2 * modulation;
  buf0->accessData(1, 0) = correction;
  buf1->accessData(1, 0) = 2 * correction;
  buf0->write();
  buf1->write();
  auto lmapWritingBufferNum = d.getOneDRegisterAccessor<uint32_t>("/currentBufferNumber");
  lmapWritingBufferNum.readLatest();
  BOOST_CHECK(lmapWritingBufferNum[0] == 1);

  boost::barrier waitForBufferSwap{3};
  std::thread readerA([&] {
    auto accessorA = d.getOneDRegisterAccessor<float>("/modulation2");
    accessorA.readLatest();
    BOOST_CHECK_CLOSE(accessorA[0], modulation, 1e-4);
    waitForBufferSwap.wait();
    accessorA.readLatest();
    BOOST_CHECK_CLOSE(accessorA[0], 2 * modulation, 1e-4);
  });
  std::thread readerB([&] {
    auto accessorB = d.getOneDRegisterAccessor<float>("/correction");
    accessorB.read();
    BOOST_CHECK_CLOSE(accessorB[0], correction, 1e-4);
    waitForBufferSwap.wait();
    accessorB.read();
    BOOST_CHECK_CLOSE(accessorB[0], 2 * correction, 1e-4);
  });

  writingBufferNum->accessData(0) = 0;
  writingBufferNum->write();
  waitForBufferSwap.wait();

  readerA.join();
  readerB.join();
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
