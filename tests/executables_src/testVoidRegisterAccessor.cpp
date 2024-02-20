// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE VoidRegisterAccessorTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "Device.h"
#include "DummyBackend.h"
#include "ExceptionDummyBackend.h"
using namespace ChimeraTK;

#include <future>

void testAsyncRO(RegisterPath name, unsigned int interruptNumber) {
  // The typical use case: The underlying register is a read-only interrput
  Device d("(dummy?map=goodMapFile.map)");
  d.open();
  d.activateAsyncRead();

  // don't use auto here to check that we get the right type
  // The typical use case: The underlying register is a void interrput
  VoidRegisterAccessor asyncAccessor = d.getVoidRegisterAccessor(name, {AccessMode::wait_for_new_data});
  BOOST_CHECK(asyncAccessor.isReadOnly());
  BOOST_CHECK(asyncAccessor.isReadable());
  BOOST_CHECK(!asyncAccessor.isWriteable());

  asyncAccessor.read(); // the initial value has arrived

  auto isReadFinished = std::async(std::launch::async, [&] { asyncAccessor.read(); });

  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);

  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(d.getBackend());

  dummy->triggerInterrupt(interruptNumber);
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(3)) == std::future_status::ready);

  // check that the implementations for readNonBlocking() and readLatest() delegate to the right function (the return
  // value still contains information) Trigger twice, then evaluate
  dummy->triggerInterrupt(interruptNumber);
  dummy->triggerInterrupt(interruptNumber);
  BOOST_CHECK(asyncAccessor.readNonBlocking());
  BOOST_CHECK(asyncAccessor.readNonBlocking());
  BOOST_CHECK(!asyncAccessor.readNonBlocking()); // third readNonBlocking return false

  dummy->triggerInterrupt(interruptNumber);
  dummy->triggerInterrupt(interruptNumber);
  BOOST_CHECK(asyncAccessor.readLatest());
  BOOST_CHECK(!asyncAccessor.readLatest()); // second readLatest return false
}

BOOST_AUTO_TEST_CASE(TestAsyncRO) {
  testAsyncRO("MODULE0/INTERRUPT_VOID1", 3);
  testAsyncRO("MODULE0/INTERRUPT_TYPE", 6);
}

BOOST_AUTO_TEST_CASE(TestAsyncRW) {
  Device d("(ExceptionDummy?map=goodMapFile.map)");
  d.open();
  d.activateAsyncRead();

  auto writeableAsyncAccessor =
      d.getVoidRegisterAccessor("MODULE0/INTERRUPT_TYPE/DUMMY_WRITEABLE"); //, {AccessMode::wait_for_new_data});
  BOOST_CHECK(!writeableAsyncAccessor.isReadOnly());
  // BOOST_CHECK(writeableAsyncAccessor.isReadable());
  BOOST_CHECK(writeableAsyncAccessor.isWriteable());

  auto writeableIntAccessor = d.getScalarRegisterAccessor<int>("MODULE0/INTERRUPT_TYPE/DUMMY_WRITEABLE");
  writeableIntAccessor = 42;
  writeableIntAccessor.write();
  /*
  // finally check that async read still works
  writeableAsyncAccessor.read();
  auto isReadFinished = std::async(std::launch::async, [&] { writeableAsyncAccessor.read(); });
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(1)) == std::future_status::timeout);
  auto dummy = boost::dynamic_pointer_cast<DummyBackend>(d.getBackend());
  dummy->triggerInterrupt(5, 6);
  BOOST_CHECK(isReadFinished.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
*/
  // writing always writes 0, although the variable was read/triggerd when there was a 42 on the hardware
  writeableAsyncAccessor.write();
  writeableIntAccessor.read();
  BOOST_CHECK_EQUAL(int(writeableIntAccessor), 0);
}

BOOST_AUTO_TEST_CASE(TestSyncRO) {
  // Void registers without wait_for_new_data don't make sense if they are not writeable
  Device d("(dummy?map=goodMapFile.map)");
  d.open();

  BOOST_CHECK_THROW(std::ignore = d.getVoidRegisterAccessor("MODULE0/INTERRUPT_VOID1"), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(TestSyncW) {
  // Just take a normal RW register, get a void register and write to it
  Device d("(dummy?map=goodMapFile.map)");
  d.open();

  auto voidAccessor = d.getVoidRegisterAccessor("MODULE1/TEST_AREA");
  BOOST_CHECK(!voidAccessor.isReadOnly());
  BOOST_CHECK(!voidAccessor.isReadable());
  BOOST_CHECK(voidAccessor.isWriteable());
  BOOST_CHECK_THROW(voidAccessor.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(voidAccessor.readNonBlocking(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(voidAccessor.readLatest(), ChimeraTK::logic_error);

  // data accessor to the same register
  auto intAccessor = d.getOneDRegisterAccessor<unsigned int>("MODULE1/TEST_AREA");

  for(unsigned int i = 0; i < intAccessor.getNElements(); ++i) {
    intAccessor[i] = i + 42;
  }
  intAccessor.write(); // write non-zero to the device

  voidAccessor.write(); // write 0 (which is void converted to int

  intAccessor.read(); // read back what is on the device

  for(auto val : intAccessor) {
    BOOST_CHECK_EQUAL(val, 0);
  }
}
