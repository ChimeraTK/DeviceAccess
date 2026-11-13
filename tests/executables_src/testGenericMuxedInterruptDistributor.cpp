// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE GenericMuxedInterruptDistributorTest
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"

#include <nlohmann/json.hpp>

#include <boost/test/unit_test.hpp>

#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>

using namespace ChimeraTK;
using namespace boost::unit_test_framework;

/**********************************************************************************************************************/

template<typename AccessorType>
bool readWithTimeout(AccessorType& acc, size_t msTimeout = 3000) {
  auto startTime = std::chrono::steady_clock::now();
  auto currentTime = startTime;
  while(currentTime < startTime + std::chrono::milliseconds(msTimeout)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if(acc.readNonBlocking()) {
      return true;
    }
    currentTime = std::chrono::steady_clock::now();
  }
  return false;
}

BOOST_AUTO_TEST_SUITE(INTCHandlerTestSuite)

static constexpr std::string_view testCdd{"(WriteMonitoring:xdma/slot5?map=irq_test.mapp)"};

/**********************************************************************************************************************/

// We need a special backend because the firmware has several "clear on 1", which internally modify
// individual bits in a word. We have to monitor the writes and log this state. Only looking at the last write is not sufficient.
class WriteMonitoringBackend : public DummyBackend {
 public:
  WriteMonitoringBackend(const std::string& mapFileName) : DummyBackend(mapFileName) {
    acknowledged["ISR"] = 0;
    acknowledged["IAR"] = 0;
    acknowledged["ICR"] = 0;

    setWriteCallbackFunction(AddressRange(0, 0x00800008, 4),
        [&] { acknowledged["ISR"] |= static_cast<uint32_t>(getRawAccessor("TEST0", "ISR")); });
    setWriteCallbackFunction(AddressRange(0, 0x0090000C, 4),
        [&] { acknowledged["IAR"] |= static_cast<uint32_t>(getRawAccessor("TEST1", "IAR")); });
    setWriteCallbackFunction(AddressRange(0, 0x00A0000C, 4),
        [&] { acknowledged["ICR"] |= static_cast<uint32_t>(getRawAccessor("TEST2", "ICR")); });
    setWriteCallbackFunction(AddressRange(0, 0x00D00008, 4),
        [&] { acknowledged["ISR"] |= static_cast<uint32_t>(getRawAccessor("TEST5", "ISR")); });
    setWriteCallbackFunction(AddressRange(0, 0x00D0000C, 4), // comment for formatting
        [&] { sie |= static_cast<uint32_t>(getRawAccessor("TEST5", "SIE")); });
  }

  static boost::shared_ptr<DeviceBackend> createInstance(
      [[maybe_unused]] std::string address, std::map<std::string, std::string> parameters) {
    if(parameters["map"].empty()) {
      throw ChimeraTK::logic_error("No map file name given.");
    }
    return boost::shared_ptr<DeviceBackend>(new WriteMonitoringBackend(parameters["map"]));
  }

  std::map<std::string, uint32_t> acknowledged;
  uint32_t sie{0};
};

/**********************************************************************************************************************/

class BackendRegisterer {
 public:
  BackendRegisterer() {
    ChimeraTK::BackendFactory::getInstance().registerBackendType(
        "WriteMonitoring", &WriteMonitoringBackend::createInstance);
  }
};

static BackendRegisterer b;

/**********************************************************************************************************************/

struct TestFixture {
  Device device{std::string{testCdd}};
  VoidRegisterAccessor accInterrupt;
  VoidRegisterAccessor dummyInterrupt;
  ScalarRegisterAccessor<uint32_t> isr;
  uint32_t _interrupt;
  boost::shared_ptr<WriteMonitoringBackend> dummyBackend;

  TestFixture(uint32_t interrupt, bool activateAsyncFirst) : _interrupt(interrupt) {
    dummyBackend = boost::dynamic_pointer_cast<WriteMonitoringBackend>(device.getBackend());
    assert(dummyBackend);

    if(activateAsyncFirst) {
      device.open();
      device.activateAsyncRead();
    }

    accInterrupt.replace(device.getVoidRegisterAccessor(
        "!" + std::to_string(interrupt) + ":4", {ChimeraTK::AccessMode::wait_for_new_data}));
    dummyInterrupt.replace(device.getVoidRegisterAccessor("DUMMY_INTERRUPT_" + std::to_string(interrupt)));

    // Note: we simulate firmware write access to the ISR register here, but in some situations the register is also
    // software-side writeable, in which case there is no DUMMY_WRITEABLE version of the register.
    ChimeraTK::RegisterPath isrName = "TEST" + std::to_string(interrupt) + "/ISR";
    if(!device.getRegisterCatalogue().getRegister(isrName).isWriteable()) {
      isrName /= "DUMMY_WRITEABLE";
    }
    isr.replace(device.getScalarRegisterAccessor<uint32_t>(isrName));

    if(activateAsyncFirst) {
      // only if asyncRead is active we will get an initial value. Pop it here for convenience.
      BOOST_TEST(readWithTimeout(accInterrupt));
    }
  }

  virtual ~TestFixture() { device.close(); }
};

/**********************************************************************************************************************/

/**
 * Test that a logic error is thrown as soon as you try to get an accessor with invalid map file entries.
 * Print the exception message for manual checking. Don't automate checking the string content. It is not
 * part of the API and subject to refactoring.
 */
struct ThrowTestFixture {
  Device device{std::string{testCdd}};
  VoidRegisterAccessor accInterrupt;

  ThrowTestFixture(uint32_t interrupt) {
    std::string testRegister("!" + std::to_string(interrupt) + ":4");
    try {
      accInterrupt.replace(device.getVoidRegisterAccessor(testRegister, {ChimeraTK::AccessMode::wait_for_new_data}));
      BOOST_CHECK_MESSAGE(false, "Creating register \"" + testRegister + "\" did not throw as expected!");
    }
    catch(ChimeraTK::logic_error& e) {
      std::cout << "Caught expected exception for " << boost::unit_test::framework::current_test_case().p_name
                << ". Print for manual check of message: " << e.what() << std::endl;
      BOOST_CHECK(true); // just to have a hook for the last successful test
    }
  }
};

/**********************************************************************************************************************/

struct Inactive0 : public TestFixture {
  Inactive0() : TestFixture(0, false) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(inactiveIER, Inactive0) {
  device.open();

  auto ier = device.getScalarRegisterAccessor<int>("TEST0/IER");
  ier.read();
  BOOST_TEST(0x0 == ier);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0);

  device.activateAsyncRead();

  ier.read();
  BOOST_TEST(0x10 == ier);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x10);
  dummyBackend->acknowledged["ISR"] = 0;

  auto accInterrupt2 = device.getVoidRegisterAccessor("!0:5", {ChimeraTK::AccessMode::wait_for_new_data});
  ier.read();
  BOOST_TEST(ier == 0x30);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x20); // we must NOT acknowledge 0x10 again!

  accInterrupt.replace(VoidRegisterAccessor{});

  // FIXME: This is the expected result if the SubDomain destructor would notify the MuxedInterruptDistributor.
  // This is not implemented yet.
#if false
  ier.read();
  BOOST_TEST(ier == 0x20); // only the second iterator remains
#endif

  // at this point the Domain and with it the MuxedInterruptDistributor goe out of scope, and the distributor's
  // destructor is kicking in.
  accInterrupt2.replace(VoidRegisterAccessor{});
  ier.read();
  BOOST_TEST(ier == 0x0);
}

/**********************************************************************************************************************/

struct Active0 : public TestFixture {
  Active0() : TestFixture(0, true) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(activateIER, Active0) {
  auto ier = device.getScalarRegisterAccessor<int>("TEST0/IER");
  ier.read();
  BOOST_TEST(0x10 == ier);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x10);
  // no need to repeat all following tests in inactiveIER here.
}

/**********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(activateOnActiveDomain) {
  // Test that the activation is called correctly for all members if the upper layers in the distribution tree are
  // already active Tree:
  //
  // Domain 3 - SubDomain [3] -- VariableDistributor<void> [3]
  //                          \_ MuxedInterruptDistibutor  [3] --- SubDomain [3, 0] -- VariableDistributor<void> [3, 0]
  //                                                           \                    \_ MuxedInterruptDistibutor  [3, 0]
  //                                                            \_ SubDomain [3, 1] -- VariableDistributor<void> [3, 1]
  //                                                                                \_ MuxedInterruptDistibutor  [3, 1]
  ChimeraTK::Device device;
  device.open(std::string{testCdd});
  device.activateAsyncRead();

  // Create the domain and check that it is active
  auto accessor3 = device.getVoidRegisterAccessor("!3", {AccessMode::wait_for_new_data});
  readWithTimeout(accessor3); // initial value
  auto dummyInterruptTrigger =
      device.getVoidRegisterAccessor("DUMMY_INTERRUPT_3"); // for triggering the dummy interrupt
  dummyInterruptTrigger.write();
  readWithTimeout(accessor3); // initial value

  // Test 1: get an accessor for a sub SubDomain of MuxedInterruptDistibutor [3], which is in the already
  // active SubDomain [3].
  auto acc3_0 = device.getVoidRegisterAccessor("!3:0", {AccessMode::wait_for_new_data});
  readWithTimeout(acc3_0);

  // Test that the MuxedInterruptDistibutor [3] has been activated
  // MER might be write-only
  BOOST_TEST(
      device.read<int32_t>("TEST3/MER/DUMMY_READABLE") == 0x3); // MER always has two bits which both have to be set

  // Test that the handshake for sub-domain [3, 0] has been activated
  BOOST_TEST(device.read<uint32_t>("TEST3/IER") == (1U << 0));

  // Test 2: The the SubDomain behind the MuxedInterruptDistributor [3] itself is activated (not only the handshake) if
  // the distributor is already active. We use SubDomain [3, 1] for this and test the SubDomain activation indirectly by
  // the activation of the MuxedInterruptDistibutor  [3, 1]
  auto acc3_1 = device.getVoidRegisterAccessor("!3:1:3", {AccessMode::wait_for_new_data});
  readWithTimeout(acc3_1);

  BOOST_TEST(device.read<int32_t>("TEST3/SUB1/MER") == 0x3);
  BOOST_TEST(device.read<uint32_t>("TEST3/SUB1/IER") == (1U << 3));
}

/**********************************************************************************************************************/

struct AcknowledgeTest : public TestFixture {
  AcknowledgeTest(uint32_t interrupt, std::string ackReg) : TestFixture(interrupt, true), ackRegister(ackReg) {}

  VoidRegisterAccessor accInterrupt2;
  std::string ackRegister;

  void run() {
    // acknowledge has been written when activating
    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x10);

    // prepare the status before sending the interrupt
    // set one more bit to be sensitive to the handshake (need to see changes)
    isr.setAndWrite(0x11);

    dummyBackend->acknowledged[ackRegister] = 0;

    dummyInterrupt.write();
    // wait until interrupt handler is done
    BOOST_TEST(readWithTimeout(accInterrupt));

    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x10);

    dummyBackend->acknowledged[ackRegister] = 0;
    accInterrupt2.replace(device.getVoidRegisterAccessor(
        "!" + std::to_string(_interrupt) + ":5", {ChimeraTK::AccessMode::wait_for_new_data}));
    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x20);
    readWithTimeout(accInterrupt2); // pop the initial value

    // Signal the first accessor

    isr.setAndWrite(0x11);

    dummyBackend->acknowledged[ackRegister] = 0;
    dummyInterrupt.write();
    BOOST_TEST(readWithTimeout(accInterrupt));
    BOOST_CHECK(!accInterrupt2.readNonBlocking());

    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x10);
    if(ackRegister != "ISR") {
      BOOST_TEST(isr.readAndGet() == 0x11);
    }

    // Signal the second accessor
    isr.setAndWrite(0x21);

    dummyBackend->acknowledged[ackRegister] = 0;
    dummyInterrupt.write();
    BOOST_TEST(readWithTimeout(accInterrupt2));
    BOOST_CHECK(!accInterrupt.readNonBlocking());

    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x20);
    if(ackRegister != "ISR") {
      BOOST_TEST(isr.readAndGet() == 0x21);
    }

    // Signal both
    isr.setAndWrite(0x31);

    dummyBackend->acknowledged[ackRegister] = 0;
    dummyInterrupt.write();
    BOOST_TEST(readWithTimeout(accInterrupt));
    BOOST_TEST(readWithTimeout(accInterrupt2));

    BOOST_TEST(dummyBackend->acknowledged[ackRegister] == 0x30);
    if(ackRegister != "ISR") {
      BOOST_TEST(isr.readAndGet() == 0x31);
    }
  }
};

/**********************************************************************************************************************/

// ISR is used as acknowledge register
struct IsrTestFixture : public AcknowledgeTest {
  IsrTestFixture() : AcknowledgeTest(0, "ISR") {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testISR, IsrTestFixture) {
  run();
}

/**********************************************************************************************************************/
/*if IAR is present: INTC writes 1<<n the according bit mask to IAR and not to ISR*/
struct IarTestFixture : public AcknowledgeTest {
  IarTestFixture() : AcknowledgeTest(1, "IAR") {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIAR, IarTestFixture) {
  run();
}

/**********************************************************************************************************************/
/* if ICR is present: INTC writes 1<<n the according bit mask to ICR and not to ISR */

struct IcrTestFixture : public AcknowledgeTest {
  IcrTestFixture() : AcknowledgeTest(2, "ICR") {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testICR, IcrTestFixture) {
  run();
}

/**********************************************************************************************************************/

struct MasterEnableTest : public TestFixture {
  MasterEnableTest(uint32_t interrupt, std::string meRegister, bool enableFirst)
  : TestFixture(interrupt, enableFirst), isEnabled(enableFirst) {
    masterEnable.replace(device.getScalarRegisterAccessor<uint32_t>(
        RegisterPath("TEST" + std::to_string(interrupt)) / meRegister / "DUMMY_READABLE"));
  }

  ScalarRegisterAccessor<uint32_t> masterEnable;
  bool isEnabled;

  void run() {
    if(!isEnabled) {
      device.open();

      // MER should not be set
      BOOST_TEST(masterEnable.readAndGet() == 0x0);

      device.activateAsyncRead();
    }

    // MER should be set now
    BOOST_TEST(masterEnable.readAndGet() == 0x3); // last two bits active
  }
};
/**********************************************************************************************************************/

struct MerInactiveTestFixture : public MasterEnableTest {
  MerInactiveTestFixture() : MasterEnableTest(3, "MER", false) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMerInactive, MerInactiveTestFixture) {
  run();
}

/**********************************************************************************************************************/

struct MerActiveTestFixture : public MasterEnableTest {
  MerActiveTestFixture() : MasterEnableTest(3, "MER", true) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMerActive, MerActiveTestFixture) {
  run();
}

/**********************************************************************************************************************/
BOOST_AUTO_TEST_CASE(testIMR) { // TEST4
  ChimeraTK::Device device;

  device.open("(dummy:xdma/slot5?map=irq_test.mapp)");
  BOOST_TEST(device.isOpened() == true);

  auto imr = device.getScalarRegisterAccessor<uint32_t>("TEST4.IMR");
  imr.setAndWrite(0x7F); // 7 bits in this register (see map file)

  // IMR is not implemented yet. This is giving an exception at the moment.
  try {
    auto accInterrput = device.getScalarRegisterAccessor<int>("!4:4", 0, {ChimeraTK::AccessMode::wait_for_new_data});
    BOOST_CHECK_MESSAGE(false, "IMR not detected as invalid option.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Caught expected exeption. Print for manual check of message: " << e.what() << std::endl;
    // We skip the rest of the test for now, but leave the code below.
    return;
  }

  device.activateAsyncRead();

  BOOST_TEST(imr.readAndGet() == 0x6F);

  device.close();
}

/**********************************************************************************************************************/

struct Inactive5 : public TestFixture {
  Inactive5() : TestFixture(5, false) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testSieCie, Inactive5) {
  auto sie = device.getScalarRegisterAccessor<int>("TEST5/SIE/DUMMY_READABLE");
  auto cie = device.getScalarRegisterAccessor<int>("TEST5/CIE/DUMMY_READABLE");

  device.open();

  // pre-condition: both registers are 0
  BOOST_TEST(sie.readAndGet() == 0x0);
  BOOST_TEST(cie.readAndGet() == 0x0);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x0);
  BOOST_TEST(isr.readAndGet() == 0x0);

  // activate
  device.activateAsyncRead();

  // only SIE has been written
  BOOST_TEST(sie.readAndGet() == 0x10);
  BOOST_TEST(isr.readAndGet() == 0x10);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x10);
  BOOST_TEST(cie.readAndGet() == 0x0);

  // remove the accessor. CIE should be written
  accInterrupt.replace(VoidRegisterAccessor{});

  BOOST_TEST(cie.readAndGet() == 0x10);
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testSieCieMulti1, Inactive5) {
  // Mixed activation: the second accessor is created AFTER activateAsyncRead
  auto sie = device.getScalarRegisterAccessor<int>("TEST5/SIE/DUMMY_READABLE");
  auto cie = device.getScalarRegisterAccessor<int>("TEST5/CIE/DUMMY_READABLE");

  device.open();
  device.activateAsyncRead();
  BOOST_TEST(sie.readAndGet() == 0x10); // just to be safe, we already know this from the previous test
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x10);
  dummyBackend->acknowledged["ISR"] = 0x0;

  auto accInterrupt2 = device.getVoidRegisterAccessor("!5:5", {ChimeraTK::AccessMode::wait_for_new_data});
  sie.read();
  BOOST_CHECK_MESSAGE((sie == 0x20) || // the implementation can either only set the new bit
          (sie == 0x30),               // or send the whole mask again
      "SIE is " + std::to_string(sie) + ", but should be 0x20 or 0x30");
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x20); // we must NOT acknowledge 0x10 again!

  accInterrupt.replace(VoidRegisterAccessor{});

  // FIXME: This is the expected result if the SubDomain destructor would notify the MuxedInterruptDistributor.
  // This is not implemented yet.
#if false
  BOOST_TEST(cie.readAndGet() == 0x10);
#endif

  // at this point the Domain and with it the MuxedInterruptDistributor goe out of scope, and the distributor's
  // destructor is kicking in.
  accInterrupt2.replace(VoidRegisterAccessor{});

  // FIXME: Finally wanted behaviour: There is only one accessor left
#if false
  BOOST_TEST(cie.readAndGet() == 0x20);
#endif
  // Actual behaviour: Both flags are written at the same time.
  BOOST_TEST(cie.readAndGet() == 0x30);
}

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testSieCieMulti2, Inactive5) {
  // Create both accessors first, then call activateAsyncRead()
  // No need to check the clear section again. It's the same as above.
  auto accInterrupt2 = device.getVoidRegisterAccessor("!5:5", {ChimeraTK::AccessMode::wait_for_new_data});

  BOOST_TEST(dummyBackend->sie == 0x0);
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x0);

  device.open();
  device.activateAsyncRead();

  BOOST_TEST(dummyBackend->sie == 0x30); // both bits set, no matter whether at the same time or individually
  BOOST_TEST(dummyBackend->acknowledged["ISR"] == 0x30);
}

/**********************************************************************************************************************/

struct GieInactiveTestFixture : public MasterEnableTest {
  GieInactiveTestFixture() : MasterEnableTest(6, "GIE", false) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testGieInactive, GieInactiveTestFixture) {
  run();
}

/**********************************************************************************************************************/

struct GieActiveTestFixture : public MasterEnableTest {
  GieActiveTestFixture() : MasterEnableTest(6, "GIE", true) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testGieActive, GieActiveTestFixture) {
  run();
}

/**********************************************************************************************************************/

struct MieInactiveTestFixture : public MasterEnableTest {
  MieInactiveTestFixture() : MasterEnableTest(7, "MIE", false) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieInactive, MieInactiveTestFixture) {
  run();
}

/**********************************************************************************************************************/

struct MieActiveTestFixture : public MasterEnableTest {
  MieActiveTestFixture() : MasterEnableTest(7, "MIE", true) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieActive, MieActiveTestFixture) {
  run();
}

/**********************************************************************************************************************/
/* ERROR Scenarios */
/**********************************************************************************************************************/

struct UnknownOptionTestFixture : public ThrowTestFixture {
  UnknownOptionTestFixture() : ThrowTestFixture(10) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testUnknownOption, UnknownOptionTestFixture) {}

/**********************************************************************************************************************/

struct InvalidJson1TestFixture : public ThrowTestFixture {
  InvalidJson1TestFixture() : ThrowTestFixture(11) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testJsonErrorInGeneralStructure, InvalidJson1TestFixture) {}

/**********************************************************************************************************************/

struct InvalidJson2TestFixture : public ThrowTestFixture {
  InvalidJson2TestFixture() : ThrowTestFixture(12) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testJsonErrorInIntcSprecific, InvalidJson2TestFixture) {}

/**********************************************************************************************************************/

struct OnlySieTestFixture : public ThrowTestFixture {
  OnlySieTestFixture() : ThrowTestFixture(13) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testOnlySie, OnlySieTestFixture) {}

/**********************************************************************************************************************/

struct OnlyCieTestFixture : public ThrowTestFixture {
  OnlyCieTestFixture() : ThrowTestFixture(14) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testOnlyCie, OnlyCieTestFixture) {}

/**********************************************************************************************************************/

struct IarAndIcrTestFixture : public ThrowTestFixture {
  IarAndIcrTestFixture() : ThrowTestFixture(15) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIarAndIcr, IarAndIcrTestFixture) {}

/**********************************************************************************************************************/

struct NoIsrTestFixture : public ThrowTestFixture {
  NoIsrTestFixture() : ThrowTestFixture(16) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testNoIsr, NoIsrTestFixture) {}

/**********************************************************************************************************************/

struct NoIerTestFixture : public ThrowTestFixture {
  NoIerTestFixture() : ThrowTestFixture(17) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testNoIer, NoIerTestFixture) {}

/**********************************************************************************************************************/

struct NoPathTestFixture : public ThrowTestFixture {
  NoPathTestFixture() : ThrowTestFixture(18) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testNoPath, NoPathTestFixture) {}

/**********************************************************************************************************************/

struct NonexistendPathTestFixture : public ThrowTestFixture {
  NonexistendPathTestFixture() : ThrowTestFixture(118) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testNonexistendPath, NonexistendPathTestFixture) {}

/**********************************************************************************************************************/

// Adapt this when more versions are added
struct UnknownVersionTestFixture : public ThrowTestFixture {
  UnknownVersionTestFixture() : ThrowTestFixture(19) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testUnknownVersion, UnknownVersionTestFixture) {}

/**********************************************************************************************************************/

// Adapt this when more versions are added
struct UnknownMainKeyTestFixture : public ThrowTestFixture {
  UnknownMainKeyTestFixture() : ThrowTestFixture(20) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testUnknownMainKey, UnknownMainKeyTestFixture) {}

/**********************************************************************************************************************/

struct MieAndGieTestFixture : public ThrowTestFixture {
  MieAndGieTestFixture() : ThrowTestFixture(21) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieAndGie, MieAndGieTestFixture) {}

/**********************************************************************************************************************/

struct MieAndMerTestFixture : public ThrowTestFixture {
  MieAndMerTestFixture() : ThrowTestFixture(22) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieAndMer, MieAndMerTestFixture) {}

/**********************************************************************************************************************/

struct GieAndMerTestFixture : public ThrowTestFixture {
  GieAndMerTestFixture() : ThrowTestFixture(23) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testGieAndMer, GieAndMerTestFixture) {}

/**********************************************************************************************************************/

struct MieGieAndMerTestFixture : public ThrowTestFixture {
  MieGieAndMerTestFixture() : ThrowTestFixture(24) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieGieAndMer, MieGieAndMerTestFixture) {}

/**********************************************************************************************************************/

struct IsrReadableTestFixture : public ThrowTestFixture {
  IsrReadableTestFixture() : ThrowTestFixture(25) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIsrReadable, IsrReadableTestFixture) {}

/**********************************************************************************************************************/

// ISR must be writeable if there is no ICR/IAR
struct IsrWritableTestFixture : public ThrowTestFixture {
  IsrWritableTestFixture() : ThrowTestFixture(26) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIsrWriteable, IsrWritableTestFixture) {}

/**********************************************************************************************************************/

struct IerWritableTestFixture : public ThrowTestFixture {
  IerWritableTestFixture() : ThrowTestFixture(27) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIerWriteable, IerWritableTestFixture) {}

/**********************************************************************************************************************/

struct IcrWritableTestFixture : public ThrowTestFixture {
  IcrWritableTestFixture() : ThrowTestFixture(28) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIcrWriteable, IcrWritableTestFixture) {}

/**********************************************************************************************************************/

struct IarWritableTestFixture : public ThrowTestFixture {
  IarWritableTestFixture() : ThrowTestFixture(29) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testIarWriteable, IarWritableTestFixture) {}

/**********************************************************************************************************************/

struct MieWritableTestFixture : public ThrowTestFixture {
  MieWritableTestFixture() : ThrowTestFixture(30) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMieWriteable, MieWritableTestFixture) {}

/**********************************************************************************************************************/

struct GieWritableTestFixture : public ThrowTestFixture {
  GieWritableTestFixture() : ThrowTestFixture(31) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testGieWriteable, GieWritableTestFixture) {}

/**********************************************************************************************************************/

struct MerWritableTestFixture : public ThrowTestFixture {
  MerWritableTestFixture() : ThrowTestFixture(32) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testMerWriteable, MerWritableTestFixture) {}

/**********************************************************************************************************************/

struct SieWritableTestFixture : public ThrowTestFixture {
  SieWritableTestFixture() : ThrowTestFixture(33) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testSieWriteable, SieWritableTestFixture) {}

/**********************************************************************************************************************/

struct CieWritableTestFixture : public ThrowTestFixture {
  CieWritableTestFixture() : ThrowTestFixture(34) {}
};

/**********************************************************************************************************************/

BOOST_FIXTURE_TEST_CASE(testCieWriteable, CieWritableTestFixture) {}

/**********************************************************************************************************************/

/**
 * Derived interrupt registers from INTC ISR bit fields, end to end on a backend.
 *
 * The irq_test.mapp fixture used by the other cases is a *.mapp file and never derives interrupt registers, so this
 * case uses simpleJsonFile.jmap, whose INTC handler [0] refers to DAQ/ISR and whose DAQ/ISR carries the 1-bit
 * children DAQ_READY (bit 0), SECOND_INTERRUPT (bit 1) and THIRD_INTERRUPT (bit 2). The dummy backend exposes a
 * hidden DUMMY_INTERRUPT_0 trigger for the primary handler [0]. Setting one ISR bit and triggering the handler must
 * wake exactly the accessor of the matching derived register.
 */
BOOST_AUTO_TEST_CASE(testDerivedInterruptRegistersEndToEnd) {
  static constexpr std::string_view jsonCdd{"(dummy?map=simpleJsonFile.jmap)"};

  Device device{std::string{jsonCdd}};
  auto catalogue = device.getRegisterCatalogue();

  // The derived registers exist under their named ISR paths.
  BOOST_TEST(catalogue.hasRegister("DAQ/ISR/DAQ_READY"));
  BOOST_TEST(catalogue.hasRegister("DAQ/ISR/THIRD_INTERRUPT"));

  device.open();
  device.activateAsyncRead();

  // Read the accessors from the named ISR paths, not from the canonical interrupt paths: a canonical-path accessor
  // resolves to a synthesised no-data interrupt register and never reads the ISR bit field, so it would not exercise
  // the derived register at all. Canonical-path resolution is already covered by the parser test.
  auto accBit0 =
      device.getScalarRegisterAccessor<uint8_t>("DAQ/ISR/DAQ_READY", 0, {ChimeraTK::AccessMode::wait_for_new_data});
  auto accBit2 = device.getScalarRegisterAccessor<uint8_t>(
      "DAQ/ISR/THIRD_INTERRUPT", 0, {ChimeraTK::AccessMode::wait_for_new_data});
  // Pop the initial values sent on activation.
  accBit0.read();
  accBit2.read();

  auto isr = device.getScalarRegisterAccessor<uint32_t>("DAQ/ISR");
  auto trigger = device.getVoidRegisterAccessor("DUMMY_INTERRUPT_0");

  // Bit 0 belongs to the first derived register only.
  isr.setAndWrite(0x1);
  trigger.write();
  BOOST_TEST(readWithTimeout(accBit0));
  BOOST_TEST(accBit0 == 1);
  BOOST_TEST(!accBit2.readNonBlocking());

  // Bit 2 belongs to the third derived register (...THIRD_INTERRUPT, id [0, 2]) only.
  isr.setAndWrite(0x4);
  trigger.write();
  BOOST_TEST(readWithTimeout(accBit2));
  BOOST_TEST(accBit2 == 1);
  BOOST_TEST(!accBit0.readNonBlocking());

  device.close();
}

/**********************************************************************************************************************/

/**
 * Register-name reference in 'triggeredByInterrupt', end to end on a backend.
 *
 * A data register refers by name to the derived ISR child DAQ/ISR/DAQ_READY of simpleJsonFile.jmap, whose INTC handler
 * [0] makes that child an interrupt register with id [0, 0]. The dummy backend exposes the hidden DUMMY_INTERRUPT_0
 * trigger for the primary handler [0]. Setting ISR bit 0 and triggering the handler must wake the referring register
 * with the ISR value, confirming the resolved id reaches the distributor. The referring register is injected into the
 * fixture map from code, so no separate map file is kept.
 */
BOOST_AUTO_TEST_CASE(testInterruptReferenceEndToEnd) {
  std::ifstream base("simpleJsonFile.jmap");
  nlohmann::json map = nlohmann::json::parse(base);
  // A data register inside BSP referring by name to the derived ISR child, so its id resolves to [0, 0].
  map["addressSpace"]["BSP"]["children"]["ISR_REFERRING"] = {{"triggeredByInterrupt", "DAQ.ISR.DAQ_READY"},
      {"numberOfElements", 1}, {"bytesPerElement", 4}, {"address", {{"offset", 16}}},
      {"representation", {{"type", "fixedPoint"}, {"width", 32}}}};

  std::string tmpFile = "interruptRefEndToEnd_" + std::to_string(getpid()) + ".jmap";
  std::ofstream(tmpFile) << map.dump(2);

  {
    Device device("(dummy?map=" + tmpFile + ")");
    device.open();
    device.activateAsyncRead();

    // The referring register resolves through its named path, not a canonical path: a canonical-path accessor resolves
    // to a synthesised no-data interrupt register and never reads the ISR bit field.
    auto accReferring =
        device.getScalarRegisterAccessor<int>("BSP/ISR_REFERRING", 0, {ChimeraTK::AccessMode::wait_for_new_data});
    // Pop the initial value sent on activation.
    accReferring.read();

    auto isr = device.getScalarRegisterAccessor<uint32_t>("DAQ/ISR");
    auto trigger = device.getVoidRegisterAccessor("DUMMY_INTERRUPT_0");

    // A different ISR bit (bit 1, id [0, 1]) must not notify the referring register, which confirms the resolved id
    // [0, 0] reaches the distributor rather than any interrupt of the handler.
    isr.setAndWrite(0x2);
    trigger.write();
    BOOST_TEST(!accReferring.readNonBlocking());

    // The ISR bit DAQ_READY (bit 0) resolves the referring register to the id [0, 0], so it is notified.
    isr.setAndWrite(0x1);
    trigger.write();
    BOOST_TEST(readWithTimeout(accReferring));

    device.close();
  }
  std::remove(tmpFile.c_str());
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
