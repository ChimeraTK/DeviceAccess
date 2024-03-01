// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
// Define a name for the test module.
#define BOOST_TEST_MODULE AsyncVarAndNestedInterruptsUnified
// Only after defining the name include the unit test header.
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BackendFactory.h"
//#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ExceptionDummyBackend.h"
//#include "TransferGroup.h"
#include "UnifiedBackendTest.h"

using namespace ChimeraTK;

//*******************************************************************************************************************/
struct DummyForCleanupCheck : public ExceptionDummy {
  using ExceptionDummy::ExceptionDummy;

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::make_shared<DummyForCleanupCheck>(parameters["map"]);
  }
  ~DummyForCleanupCheck() override {
    std::cout << "~DummyForCleanupCheck()" << std::endl;
    cleanupCalled = true;
  }

  struct BackendRegisterer {
    BackendRegisterer() {
      ChimeraTK::BackendFactory::getInstance().registerBackendType(
          "DummyForCleanupCheck", &DummyForCleanupCheck::createInstance, {"map"});
    }
  };
  static std::atomic_bool cleanupCalled;
};
std::atomic_bool DummyForCleanupCheck::cleanupCalled{false};
static DummyForCleanupCheck::BackendRegisterer gDFCCRegisterer;

/* ===============================================================================================
 * This test is checking async variables and the map-file related part of interrupts for
 * consistency with the specification (implemented in the unified test).
 * - AsyncNDRegisterAccessor
 * - AsyncVariable (multiple listeners to one logical async variable)
 * - Basic interrupt controller handler functionality (via DummyInterruptControllerHandler)
 * - TriggeredPollDistributor
 * - Instaniation from the map file
 *
 * FIXME: Unified test does not support void variables yet.
 * ==============================================================================================*/

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(AsyncVarAndNestedInterruptsUnifiedTestSuite)

//********************************************************************************************************************/

static std::string cdd("(DummyForCleanupCheck:1?map=testNestedInterrupts.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<ExceptionDummy>(BackendFactory::getInstance().createBackend(cdd));

template<class WITHPATH, uint32_t INTERRUPT>
struct TriggeredInt {
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() {
    return {ChimeraTK::AccessMode::raw, ChimeraTK::AccessMode::wait_for_new_data};
  }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef int32_t minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .enableTestRawTransfer();

  DummyRegisterAccessor<int32_t> acc{exceptionDummy.get(), "", WITHPATH::path()};

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{acc + static_cast<int32_t>(INTERRUPT)}}; // just re-use the interrupt here. Any number does the job.
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{acc}};
  }

  void setRemoteValue() {
    acc = generateValue<minimumUserType>()[0][0];
    if(!WITHPATH::activeInterruptsPath().empty()) {
      DummyRegisterAccessor<uint32_t> activeInterrupts{exceptionDummy.get(), "", WITHPATH::activeInterruptsPath()};
      activeInterrupts = WITHPATH::activeInterruptsValue();
    }
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }

  void forceAsyncReadInconsistency() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }
};

//********************************************************************************************************************/

struct datafrom6 : public TriggeredInt<datafrom6, 6> {
  static std::string path() { return "/datafrom6"; }
  static std::string activeInterruptsPath() { return ""; } // empty
  static uint32_t activeInterruptsValue() { return 0; }
};

struct datafrom5_9 : public TriggeredInt<datafrom5_9, 5> {
  static std::string path() { return "/datafrom5_9"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller5/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 9U; }
};

struct datafrom4_8_2 : public TriggeredInt<datafrom4_8_2, 4> {
  static std::string path() { return "/datafrom4_8_2"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 2U; }
  datafrom4_8_2() {
    DummyRegisterAccessor<uint32_t> activeParentInterrupts{
        exceptionDummy.get(), "", "/int_ctrls/controller4/active_ints"};
    activeParentInterrupts = 1U << 8U;
  }
};

struct datafrom4_8_3 : public TriggeredInt<datafrom4_8_3, 4> {
  static std::string path() { return "/datafrom4_8_3"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 3U; }
  datafrom4_8_3() {
    DummyRegisterAccessor<uint32_t> activeParentInterrupts{
        exceptionDummy.get(), "", "/int_ctrls/controller4/active_ints"};
    activeParentInterrupts = 1U << 8U;
  }
};

//********************************************************************************************************************/

// Use bool accessors instead of void
template<class WITHPATH, uint32_t INTERRUPT>
struct BoolAsVoid {
  bool isWriteable() { return false; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef ChimeraTK::Boolean minimumUserType;
  typedef ChimeraTK::Void rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly()
                                           .disableTestWriteNeverLosesData()
                                           .disableTestRawTransfer();

  template<typename Type>
  std::vector<std::vector<Type>> generateValue([[maybe_unused]] bool raw = false) {
    return {{{}}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue([[maybe_unused]] bool raw = false) {
    return {{{}}};
  }

  void setRemoteValue() {
    if(!WITHPATH::activeInterruptsPath().empty()) {
      DummyRegisterAccessor<uint32_t> activeInterrupts{exceptionDummy.get(), "", WITHPATH::activeInterruptsPath()};
      activeInterrupts = WITHPATH::activeInterruptsValue();
    }
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }

  void forceAsyncReadInconsistency() {}

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
    exceptionDummy->throwExceptionOpen = enable;
    if(exceptionDummy->isOpen()) {
      exceptionDummy->triggerInterrupt(INTERRUPT);
    }
  }
};

//********************************************************************************************************************/

struct interrupt6 : public BoolAsVoid<interrupt6, 6> {
  static std::string path() { return "/interrupt6"; }
  static std::string activeInterruptsPath() { return ""; } // empty
  static uint32_t activeInterruptsValue() { return 0; }

  // The accessor itself cannot trigger a runtime error, as it is indirectly fed by a thread that does not
  // know about the individual accessors.
  // Exceptions only are written to the queue when setException() is called.
  size_t nRuntimeErrorCases() { return 0; }
};

struct canonicalInterrupt6 : public BoolAsVoid<canonicalInterrupt6, 6> {
  static std::string path() { return "/!6"; }
  static std::string activeInterruptsPath() { return ""; } // empty
  static uint32_t activeInterruptsValue() { return 0; }
  size_t nRuntimeErrorCases() { return 0; }
};

struct interrupt5_9 : public BoolAsVoid<interrupt5_9, 5> {
  static std::string path() { return "/interrupt5_9"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller5/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 9U; }
};

struct canonicalInterrupt5 : public BoolAsVoid<canonicalInterrupt5, 5> {
  static std::string path() { return "/!5"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller5/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 9U; }
  size_t nRuntimeErrorCases() { return 0; }
};

struct canonicalInterrupt5_9 : public BoolAsVoid<canonicalInterrupt5_9, 5> {
  static std::string path() { return "/!5:9"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller5/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 9U; }
};

struct interrupt4_8_2 : public BoolAsVoid<interrupt4_8_2, 4> {
  static std::string path() { return "/interrupt4_8_2"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 2U; }
};

struct canonicalInterrupt4a : public BoolAsVoid<canonicalInterrupt4a, 4> {
  static std::string path() { return "/!4"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 2U; }
  size_t nRuntimeErrorCases() { return 0; }
};

struct canonicalInterrupt4_8a : public BoolAsVoid<canonicalInterrupt4_8a, 4> {
  static std::string path() { return "/!4:8"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 2U; }
};

struct canonicalInterrupt4_8_2 : public BoolAsVoid<canonicalInterrupt4_8_2, 4> {
  static std::string path() { return "/!4:8:2"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 2U; }
};

struct interrupt4_8_3 : public BoolAsVoid<interrupt4_8_3, 4> {
  static std::string path() { return "/interrupt4_8_3"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 3U; }
};

struct canonicalInterrupt4b : public BoolAsVoid<canonicalInterrupt4b, 4> {
  static std::string path() { return "/!4"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 3U; }
  size_t nRuntimeErrorCases() { return 0; }
};

struct canonicalInterrupt4_8b : public BoolAsVoid<canonicalInterrupt4_8b, 4> {
  static std::string path() { return "/!4:8"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 3U; }
};

struct canonicalInterrupt4_8_3 : public BoolAsVoid<canonicalInterrupt4_8_3, 4> {
  static std::string path() { return "/!4:8:3"; }
  static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
  static uint32_t activeInterruptsValue() { return 1U << 3U; }
};

//********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  struct canonicalInterrupt4_8b : public BoolAsVoid<canonicalInterrupt4_8b, 4> {
    static std::string path() { return "/!4:8"; }
    static std::string activeInterruptsPath() { return "/int_ctrls/controller4_8/active_ints"; }
    static uint32_t activeInterruptsValue() { return 1U << 3U; }
  };

  std::cout << "*** testRegisterAccessor *** " << std::endl;
  {
    ChimeraTK::UnifiedBackendTest<>()
        .addRegister<datafrom6>()
        .addRegister<datafrom5_9>()
        .addRegister<datafrom4_8_2>()
        .addRegister<datafrom4_8_3>()
        .addRegister<interrupt6>()
        .addRegister<canonicalInterrupt6>()
        .addRegister<interrupt5_9>()
        .addRegister<canonicalInterrupt5>()
        .addRegister<canonicalInterrupt5_9>()
        .addRegister<interrupt4_8_2>()
        .addRegister<canonicalInterrupt4a>()
        .addRegister<canonicalInterrupt4_8a>()
        .addRegister<canonicalInterrupt4_8_2>()
        .addRegister<interrupt4_8_3>()
        .addRegister<canonicalInterrupt4b>()
        .addRegister<canonicalInterrupt4_8b>()
        .addRegister<canonicalInterrupt4_8_3>()
        .runTests(cdd);
  }
  exceptionDummy = nullptr;
  BOOST_CHECK(DummyForCleanupCheck::cleanupCalled);
}

//*********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
