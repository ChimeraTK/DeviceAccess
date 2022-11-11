// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE SharedDummyBackendUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "SharedDummyBackend.h"
#include "sharedDummyHelpers.h"
#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(SharedDummyBackendUnifiedTestSuite)

/**********************************************************************************************************************/

//  Use hardcoded information from the dmap-file
static std::string instanceId{"1"};
static std::string mapFileName{"sharedDummyUnified.map"};
static std::string cdd(std::string("(sharedMemoryDummy:") + instanceId + "?map=" + mapFileName + ")");
static boost::shared_ptr<SharedDummyBackend> sharedDummy;
const int timeOutForWaitOnHelperProcess_ms = 2000;

// lock preventing concurrent test execution
static TestLocker testLocker("sharedDummyUnified.dmap");

// use static instance with destructor to stop background application after all tests
struct HelperProcess {
  HelperProcess() {
    // set up sharedDummy as communication interface to background process
    sharedDummy = boost::dynamic_pointer_cast<SharedDummyBackend>(BackendFactory::getInstance().createBackend(cdd));
    sharedDummy->open();
    mirrorRequest.type = sharedDummy->getRegisterAccessor<uint32_t>("MIRRORREQUEST/TYPE", 1, 0, AccessModeFlags{});
    mirrorRequest.busy = sharedDummy->getRegisterAccessor<uint32_t>("MIRRORREQUEST/BUSY", 1, 0, AccessModeFlags{});
    // TODO debug - it seems this register causes clean-up to fail
    //  Potentially because there is still a reference in the TransferGroup in the asyncInterruptDispatcher, causing a
    //  circular reference to the device shared pointer
    mirrorRequest.updated = sharedDummy->getRegisterAccessor<uint32_t>(
        "MIRRORREQUEST/UPDATED", 1, 0, AccessModeFlags{AccessMode::wait_for_new_data});
    mirrorRequest.triggerInterrupt =
        sharedDummy->getRegisterAccessor<uint32_t>("MIRRORREQUEST/DATA_INTERRUPT", 1, 0, {});
  }

  struct {
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> type;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> busy;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> updated;
    boost::shared_ptr<NDRegisterAccessor<uint32_t>> triggerInterrupt;
  } mirrorRequest;

  void requestMirroring(MirrorRequestType reqType, bool triggerDataInterrupt = false) {
    sharedDummy->open();
    // trigger mirror operation by helper thread and wait on completion
    mirrorRequest.triggerInterrupt->accessData(0) = triggerDataInterrupt ? 1 : 0;
    mirrorRequest.triggerInterrupt->write();
    mirrorRequest.type->accessData(0) = (int)reqType;
    mirrorRequest.type->write();
    mirrorRequest.busy->accessData(0) = 1;
    mirrorRequest.busy->write();
    int timeoutCnt = timeOutForWaitOnHelperProcess_ms / 50;
    do {
      // we use boost::sleep because it defines an interruption point for signals
      boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
      mirrorRequest.busy->readLatest();
    } while(mirrorRequest.busy->accessData(0) == 1 && (--timeoutCnt >= 0));
    BOOST_CHECK(timeoutCnt >= 0);
  }

  void start() {
    // start second accessing application in background
    BOOST_CHECK(!std::system("./testSharedDummyBackendUnifiedExt "
                             "--run_test=SharedDummyBackendUnifiedTestSuite/testRegisterAccessor > /dev/null"
                             " & echo $! > ./testSharedDummyBackendUnifiedExt.pid"));
  }
  // request helper to stop gracefully - this includes a handshake waiting on it's termination
  void stopGracefully() { requestMirroring(MirrorRequestType::stop); }
  void kill() {
    auto ret = std::system("pidfile=./testSharedDummyBackendUnifiedExt.pid; if [ -f $pidfile ]; "
                           "then kill $(cat $pidfile); rm $pidfile; fi ");
    if(ret == -1) {
      throw std::runtime_error("Attempt to kill helper process failed.");
    }
  }
  // discard all accessors. do not use after this point
  void reset() {
    mirrorRequest.type.reset();
    mirrorRequest.busy.reset();
    mirrorRequest.updated.reset();
    mirrorRequest.triggerInterrupt.reset();
  }
  ~HelperProcess() { kill(); }
} gHelperProcess;

/**********************************************************************************************************************/

template<typename Derived>
struct Integers_base {
  Derived* derived{static_cast<Derived*>(this)};
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::raw}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 0; }
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableAsyncReadInconsistency()
                                           .enableTestRawTransfer();

  boost::shared_ptr<NDRegisterAccessor<minimumUserType>> acc{
      sharedDummy->getRegisterAccessor<minimumUserType>(derived->path(), 1, 0, AccessModeFlags{})};
  boost::shared_ptr<NDRegisterAccessor<rawUserType>> accBackdoor{sharedDummy->getRegisterAccessor<rawUserType>(
      std::string("MIRRORED/") + derived->path(), 1, 0, {AccessMode::raw})};

  void ensureOpen() {
    // since the front-door and back-door access goes over the same SharedDummyBackend instance, the spec tests
    // unintentionally also close our back-door and we need to make sure it's open again.
    sharedDummy->open();
  }

  // Type can be raw type or user type
  template<typename Type>
  std::vector<std::vector<Type>> generateValue(bool raw = false) {
    ensureOpen();
    accBackdoor->readLatest();
    rawUserType rawVal00 = accBackdoor->accessData(0);
    rawVal00 += 3;
    Type val00 = (raw ? rawVal00 : derived->template rawToCooked<Type, rawUserType>(rawVal00));
    return {{val00}};
  }

  // Type can be raw type or user type
  template<typename Type>
  std::vector<std::vector<Type>> getRemoteValue(bool raw = false) {
    ensureOpen();
    gHelperProcess.requestMirroring(MirrorRequestType::from);
    accBackdoor->readLatest();
    rawUserType rawVal00 = accBackdoor->accessData(0);
    Type val00 = (raw ? rawVal00 : derived->template rawToCooked<Type, rawUserType>(rawVal00));

    return {{val00}};
  }

  void setRemoteValue() {
    ensureOpen();
    auto x = generateValue<rawUserType>(/* raw = */ true)[0][0];
    accBackdoor->accessData(0) = x;
    accBackdoor->write();
    gHelperProcess.requestMirroring(MirrorRequestType::to);
  }

  // default implementation just casting. Re-implement in derived classes if needed.
  template<typename UserType, typename RawType>
  RawType cookedToRaw(UserType val) {
    return static_cast<RawType>(val);
  }

  // default implementation just casting. Re-implement in derived classes if needed.
  template<typename UserType, typename RawType>
  UserType rawToCooked(RawType val) {
    return static_cast<UserType>(val);
  }

  // we need this because it's expected in template, but unused
  void setForceRuntimeError(bool /*enable*/, size_t) {}
};

struct Integers_signed32 : public Integers_base<Integers_signed32> {
  std::string path() { return "INTC_RW"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
};
struct Integers_signed32_RO : public Integers_base<Integers_signed32_RO> {
  std::string path() { return "INTA_RO"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
};
struct Integers_signed32_WO : public Integers_base<Integers_signed32_WO> {
  std::string path() { return "INTB_WO"; }
  bool isWriteable() { return true; }
  bool isReadable() { return false; }
};
struct Integers_signed32_DummyWritable : public Integers_base<Integers_signed32_DummyWritable> {
  std::string path() { return "INTA_RO/DUMMY_WRITEABLE"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
};

struct Integers_signed32_async : public Integers_base<Integers_signed32_async> {
  static int value;
  std::string path() { return "INTD_ASYNC"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue(bool = false) {
    return {{++value}};
  }

  void setRemoteValue() {
    ensureOpen();
    auto x = generateValue<minimumUserType>()[0][0];
    accBackdoor->accessData(0) = x;
    accBackdoor->write();
    gHelperProcess.requestMirroring(MirrorRequestType::to, true);
  }
};

int Integers_signed32_async::value = 12;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  gHelperProcess.start();

  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<Integers_signed32>()
      .addRegister<Integers_signed32_RO>()
      .addRegister<Integers_signed32_WO>()
      .addRegister<Integers_signed32_DummyWritable>()
      .addRegister<Integers_signed32_async>()
      .runTests(cdd);

  gHelperProcess.kill();
}

/**
 * Checks that shared memory has been removed, after all backend instances (including background process) are gone
 */
BOOST_AUTO_TEST_CASE(testVerifyMemoryDeleted) {
  gHelperProcess.start();
  gHelperProcess.stopGracefully();
  gHelperProcess.reset();

  // also clear our backend instance. This should also remove allocated SHM segments and semaphores
  // - note, this only works if the global instance map uses weak pointers
  sharedDummy.reset();

  boost::filesystem::path absPathToMapFile = boost::filesystem::absolute(mapFileName);
  std::string shmName{createExpectedShmName(instanceId, absPathToMapFile.string(), getUserName())};

  // Check that memory is removed
  BOOST_CHECK(!shm_exists(shmName));
}

BOOST_AUTO_TEST_SUITE_END()
