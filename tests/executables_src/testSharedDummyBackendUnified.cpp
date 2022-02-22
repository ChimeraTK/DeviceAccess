#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE SharedDummyBackendUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "TransferGroup.h"
#include "BackendFactory.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "SharedDummyBackend.h"
#include "UnifiedBackendTest.h"
#include "sharedDummyHelpers.h"

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
    mirrorRequest_Type = sharedDummy->getRegisterAccessor<uint32_t>("MIRRORREQUEST/TYPE", 1, 0, AccessModeFlags{});
    mirrorRequest_Busy = sharedDummy->getRegisterAccessor<uint32_t>("MIRRORREQUEST/BUSY", 1, 0, AccessModeFlags{});
    //TODO debug - it seems this register causes clean-up to fail
    //mirrorRequest_Updated = sharedDummy->getRegisterAccessor<uint32_t>(
    //    "MIRRORREQUEST/UPDATED", 1, 0, AccessModeFlags{AccessMode::wait_for_new_data});
  }
  boost::shared_ptr<NDRegisterAccessor<uint32_t>> mirrorRequest_Type;
  boost::shared_ptr<NDRegisterAccessor<uint32_t>> mirrorRequest_Busy;
  boost::shared_ptr<NDRegisterAccessor<uint32_t>> mirrorRequest_Updated;

  void requestMirroring(MirrorRequest_Type reqType) {
    sharedDummy->open();
    // trigger mirror operation by helper thread and wait on completion
    mirrorRequest_Type->accessData(0) = reqType;
    mirrorRequest_Type->write();
    mirrorRequest_Busy->accessData(0) = 1;
    mirrorRequest_Busy->write();
    int timeoutCnt = timeOutForWaitOnHelperProcess_ms / 50;
    do {
      // we use boost::sleep because it defines an interruption point for signals
      boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
      mirrorRequest_Busy->readLatest();
    } while(mirrorRequest_Busy->accessData(0) == 1 && (--timeoutCnt >= 0));
    BOOST_CHECK(timeoutCnt >= 0);
  }

  void start() {
    // start second accessing application in background
    BOOST_CHECK(!std::system("./testSharedDummyBackendUnifiedExt "
                             "--run_test=SharedDummyBackendUnifiedTestSuite/testRegisterAccessor"
                             " & echo $! > ./testSharedDummyBackendUnifiedExt.pid"));
  }
  // request helper to stop gracefully - this includes a handshake waiting on it's termination
  void stopGracefully() { requestMirroring(mirrorRequest_Stop); }
  void kill() {
    std::system("pidfile=./testSharedDummyBackendUnifiedExt.pid; if [ -f $pidfile ]; "
                "then kill $(cat $pidfile); rm $pidfile; fi ");
  }
  // discard all accessors. do not use after this point
  void reset() {
    mirrorRequest_Type.reset();
    mirrorRequest_Busy.reset();
    mirrorRequest_Updated.reset();
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
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData();

  boost::shared_ptr<NDRegisterAccessor<minimumUserType>> acc{
      sharedDummy->getRegisterAccessor<minimumUserType>(derived->path(), 1, 0, AccessModeFlags{})};
  boost::shared_ptr<NDRegisterAccessor<minimumUserType>> accBackdoor{sharedDummy->getRegisterAccessor<minimumUserType>(
      std::string("MIRRORED/") + derived->path(), 1, 0, AccessModeFlags{})};

  void ensureOpen() {
    // since the front-door and back-door access goes over the same SharedDummyBackend instance, the spec tests
    // unintentionally also close our back-door and we need to make sure it's open again.
    sharedDummy->open();
  }
  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    ensureOpen();
    minimumUserType val00 = acc->accessData(0);
    return {{val00 + 3}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    ensureOpen();
    gHelperProcess.requestMirroring(mirrorRequest_From);
    accBackdoor->readLatest();
    minimumUserType val00 = accBackdoor->accessData(0);

    return {{val00}};
  }

  void setRemoteValue() {
    ensureOpen();
    auto x = generateValue<minimumUserType>()[0][0];
    accBackdoor->accessData(0) = x;
    accBackdoor->write();
    gHelperProcess.requestMirroring(mirrorRequest_To);
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

// TODO implement async read test
// we need to support the sequence setRemoteValue, getRemoteValue, read
// and pushing the new value must be initiated by the helper process
// setRemoteValue of Integers_base will trigger new value for UPDATED
struct Integers_signed32_async : public Integers_base<Integers_signed32_async> {
  std::string path() { return "MIRRORREQUEST/UPDATED"; }
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return { ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    // TODO check - do we need generateValue at all here?
    ensureOpen();
    minimumUserType val00 = gHelperProcess.mirrorRequest_Updated->accessData(0);
    // incremented value of UPDATED
    return {{val00 + 1}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    ensureOpen();
    gHelperProcess.mirrorRequest_Updated->readLatest();
    // TODO maybe accBackdoor will couse problems since non-existing ?
    minimumUserType val00 = gHelperProcess.mirrorRequest_Updated->accessData(0);

    return {{val00}};
  }

  void forceAsyncReadInconsistency() {
    // TODO do we need this?
    // Change value without sending interrupt
    auto x = generateValue<minimumUserType>()[0][0];
  }
};
struct Integers_signed32_async_rw : Integers_signed32_async {
  // Using the DUMMY_WRITEABLE register here since usually an async regiter is r/o implicitly
  std::string path() { return "MIRRORREQUEST/UPDATED/DUMMY_WRITEABLE"; }
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  gHelperProcess.start();

  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .addRegister<Integers_signed32>()
      .addRegister<Integers_signed32_RO>()
      .addRegister<Integers_signed32_WO>()
      .addRegister<Integers_signed32_DummyWritable>()
      //.addRegister<Integers_signed32_async>()
      //.addRegister<Integers_signed32_async_rw>()
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
