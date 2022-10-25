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

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(DoubleBufferingBackendUnifiedTestSuite)

/**********************************************************************************************************************/

/**
 * dummy backend used for testing the double buffering handshake
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

    if(useLock) {
      iWasHere = true;
      std::lock_guard lg(readerInterruptLock);
    }
    // finalize reading by calling Exception backend read
    ChimeraTK::ExceptionDummy::read(bar, address, data, sizeInBytes);
  }
  static thread_local bool useLock;
  // TODO - replace with condition var or boost::barrier
  std::atomic<bool> iWasHere;
  std::mutex readerInterruptLock;
};
thread_local bool DummyForDoubleBuffering::useLock = false;

static DummyForDoubleBuffering::BackendRegisterer gDFDBRegisterer;

static std::string rawDeviceCdd("(DummyForDoubleBuffering?map=doubleBuffer.map)");
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
  size_t nRuntimeErrorCases() { return 0; }

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

  void setForceRuntimeError(bool /*enable*/, size_t) { assert(false); }
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
  std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + rawDeviceCdd + ")";
  ChimeraTK::UnifiedBackendTest<>().addRegister<AreaType<MyArea1>>().runTests(lmap);
}

BOOST_AUTO_TEST_CASE(testSlowReader) {
  /*
   * Test race condition: slow reader, which blocks the Firmware from buffer switching.
   * TODO discuss - could this test be done within unified test? I guess not.
   */
  // we call the backend frontdoor when we modify the behavior of the thread reading via double buffering mechanism
  auto frontdoor = boost::dynamic_pointer_cast<DummyForDoubleBuffering>(backdoor);
  assert(frontdoor);
  auto doubleBufferingEnabled = backdoor->getRegisterAccessor<uint32_t>("APP/1/WORD_DUB_BUF_ENA", 0, 0, {});
  doubleBufferingEnabled->accessData(0) = 1;
  doubleBufferingEnabled->write();
  std::string lmap = "(logicalNameMap?map=doubleBuffer.xlmap&target=" + rawDeviceCdd + ")";
  Device d(lmap);
  d.open();

  // make double buffer operation block after write to ctrl register, at read of buffer number
  frontdoor->iWasHere = false;
  std::unique_lock lockGuard(frontdoor->readerInterruptLock);

  auto accessor = d.getOneDRegisterAccessor<uint32_t>("/doubleBuffer");

  std::thread s([&] {
    // this thread reads from double-buffered region
    frontdoor->useLock = true;
    accessor.read();
  });

  // wait that threas s is in blocked double-buffer read
  while(!frontdoor->iWasHere) {
    usleep(10000);
  }

  // simplification: instead of writing fw simulation which would overwrite data now,
  // just check that buffer switching was disabled
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(!doubleBufferingEnabled->accessData(0));

  lockGuard.unlock();
  s.join();

  // check that buffer switching enabled, by finalization of double-buffered read
  doubleBufferingEnabled->readLatest();
  BOOST_CHECK(doubleBufferingEnabled->accessData(0));
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
