// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ScalarRegisterAccessorTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "accessPrivateData.h"
#include "Device.h"
#include "DummyBackend.h"
#include "DummyRegisterAccessor.h"
#include "ScalarRegisterAccessor.h"
#include "WriteCountingBackend.h"
#include <unordered_map>

#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include <algorithm>
#include <math.h>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCreation) {
  setDMapFilePath("dummies.dmap");
  std::cout << "testCreation" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<DummyBackend> backend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend("DUMMYD2"));
  BOOST_CHECK(backend != NULL);

  // obtain register accessor in disconnected state
  ScalarRegisterAccessor<int> intRegisterDisconnected;
  BOOST_CHECK(intRegisterDisconnected.isInitialised() == false);
  intRegisterDisconnected.replace(device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS"));
  BOOST_CHECK(intRegisterDisconnected.isInitialised() == true);

  // obtain register accessor with integral type
  ScalarRegisterAccessor<int> intRegister = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");
  BOOST_CHECK(intRegister.isInitialised() == true);

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testIntRegisterAccessor) {
  setDMapFilePath("dummies.dmap");
  std::cout << "testRegisterAccessor" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<DummyBackend> backend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend("DUMMYD2"));
  BOOST_CHECK(backend != NULL);

  // obtain register accessor with integral type
  ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");
  BOOST_CHECK(accessor.isReadOnly() == false);
  BOOST_CHECK(accessor.isReadable());
  BOOST_CHECK(accessor.isWriteable());

  // dummy register accessor for comparison
  DummyRegisterAccessor<int> dummy(backend.get(), "APP0", "WORD_STATUS");

  // test type conversion etc. for reading
  dummy = 5;
  accessor.read();
  BOOST_CHECK(accessor == 5);
  BOOST_CHECK(int(accessor) == 5);
  BOOST_CHECK(2 * accessor == 10);
  BOOST_CHECK(accessor + 2 == 7);
  dummy = -654;
  BOOST_CHECK(accessor == 5);
  accessor.read();
  BOOST_CHECK(accessor == -654);

  // test assignment etc. for writing
  accessor = -666;
  accessor.write();
  BOOST_CHECK(dummy == -666);
  accessor = 222;
  accessor.write();
  BOOST_CHECK(dummy == 222);

  // test pre-increment operator
  ScalarRegisterAccessor<int> copy = ++accessor;

  BOOST_CHECK(accessor == 223);
  BOOST_CHECK(copy == 223);
  BOOST_CHECK(dummy == 222);
  accessor.write();
  BOOST_CHECK(dummy == 223);
  copy = 3;
  BOOST_CHECK(accessor == 3);
  copy.write();
  BOOST_CHECK(dummy == 3);

  // test pre-decrement operator
  copy.replace(--accessor);

  BOOST_CHECK(accessor == 2);
  BOOST_CHECK(copy == 2);
  BOOST_CHECK(dummy == 3);
  accessor.write();
  BOOST_CHECK(dummy == 2);
  copy = 42;
  BOOST_CHECK(accessor == 42);
  copy.write();
  BOOST_CHECK(dummy == 42);

  // test post-increment operator
  int oldValue = accessor++;

  BOOST_CHECK(accessor == 43);
  BOOST_CHECK(copy == 43);
  BOOST_CHECK(oldValue == 42);
  BOOST_CHECK(dummy == 42);
  accessor.write();
  BOOST_CHECK(dummy == 43);

  // test post-decrement operator
  accessor = 120;
  oldValue = accessor--;

  BOOST_CHECK(accessor == 119);
  BOOST_CHECK(copy == 119);
  BOOST_CHECK(oldValue == 120);
  BOOST_CHECK(dummy == 43);
  accessor.write();
  BOOST_CHECK(dummy == 119);

  // test readAndGet
  dummy = 470;
  BOOST_CHECK(accessor.readAndGet() == 470);

  // test setAndWrite
  accessor.setAndWrite(4711);
  BOOST_CHECK(dummy == 4711);

  // test correct version number handling
  VersionNumber someVersionNumber = VersionNumber();
  accessor.setAndWrite(815, someVersionNumber);
  BOOST_CHECK(accessor.getVersionNumber() == someVersionNumber);

  // test correct version number handling with default values
  VersionNumber before = VersionNumber();
  accessor.setAndWrite(77);
  VersionNumber after = VersionNumber();
  BOOST_CHECK(accessor.getVersionNumber() > before);
  BOOST_CHECK(accessor.getVersionNumber() < after);

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFloatRegisterAccessor) {
  setDMapFilePath("dummies.dmap");
  std::cout << "testFloatRegisterAccessor" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<DummyBackend> backend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend("DUMMYD2"));
  BOOST_CHECK(backend != NULL);

  // obtain register accessor with integral type
  ScalarRegisterAccessor<float> accessor = device.getScalarRegisterAccessor<float>("MODULE1/WORD_USER2");

  // dummy register accessor for comparison
  DummyRegisterAccessor<float> dummy(backend.get(), "MODULE1", "WORD_USER2");

  // test type conversion etc. for reading
  dummy = 5.3;
  float requiredVal = dummy;
  BOOST_CHECK_CLOSE(requiredVal, 5.3, 1);

  accessor.read();
  float val = accessor; // BOOST_CHECK_CLOSE requires implicit conversion in
                        // both directions, so we must help us here
  BOOST_CHECK_CLOSE(val, requiredVal, 0.01);
  BOOST_CHECK_CLOSE(float(accessor), requiredVal, 0.01);
  BOOST_CHECK_CLOSE(2. * accessor, 2 * requiredVal, 0.01);
  BOOST_CHECK_CLOSE(accessor + 2, 2 + requiredVal, 0.01);
  dummy = -10;
  BOOST_CHECK_CLOSE(float(accessor), requiredVal, 0.01);
  accessor.read();
  BOOST_CHECK_CLOSE(float(accessor), 0, 0.01);

  // test assignment etc. for writing
  accessor = -4;
  accessor.write();
  BOOST_CHECK_CLOSE(float(dummy), 0, 0.01);
  accessor = 10.3125;
  accessor.write();
  BOOST_CHECK_CLOSE(float(dummy), 10.3125, 0.01);

  device.close();
}

/**********************************************************************************************************************/

/// test the scalar accessor as one value in a larger register
BOOST_AUTO_TEST_CASE(testWordOffset) {
  setDMapFilePath("dummies.dmap");
  std::cout << "testWordOffset" << std::endl;

  Device device;
  device.open("DUMMYD2");
  boost::shared_ptr<DummyBackend> backend =
      boost::dynamic_pointer_cast<DummyBackend>(BackendFactory::getInstance().createBackend("DUMMYD2"));
  BOOST_CHECK(backend != NULL);

  // The second entry in module 1 is WORD_USER2
  DummyRegisterAccessor<float> dummy(backend.get(), "MODULE1", "WORD_USER2");
  dummy = 3.5;

  // obtain register accessor with integral type. We use and offset of 1 (second
  // word in module1), and raw  mode to check that argument passing works
  ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>("APP0/MODULE1", 1, {AccessMode::raw});
  accessor.read();
  BOOST_CHECK(accessor == static_cast<int>(3.5 * (1 << 5))); // 5 fractional bits, float value 3.5

  // Just to be safe that we don't accidentally have another register with
  // content 112: modify it
  ++accessor;
  accessor.write();
  BOOST_CHECK(dummy == 3.53125);

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testUniqueID) {
  setDMapFilePath("dummies.dmap");
  std::cout << "testUniqueID" << std::endl;

  Device device;
  device.open("DUMMYD2");

  // get register accessors
  ScalarRegisterAccessor<int> accessor1 = device.getScalarRegisterAccessor<int>("APP0/MODULE0", 1, {AccessMode::raw});
  ScalarRegisterAccessor<int> accessor2 = device.getScalarRegisterAccessor<int>("APP0/MODULE1", 1, {AccessMode::raw});

  // self consistency check
  BOOST_CHECK(accessor1.getId() == accessor1.getId());
  BOOST_CHECK(!(accessor1.getId() != accessor1.getId()));
  BOOST_CHECK(accessor2.getId() == accessor2.getId());
  BOOST_CHECK(!(accessor2.getId() != accessor2.getId()));
  BOOST_CHECK(accessor1.getId() != accessor2.getId());
  BOOST_CHECK(!(accessor1.getId() == accessor2.getId()));
  BOOST_CHECK(accessor2.getId() != accessor1.getId());
  BOOST_CHECK(!(accessor2.getId() == accessor1.getId()));

  // copy the abstractor and check if unique ID stays the same
  ScalarRegisterAccessor<int> accessor1Copied;
  accessor1Copied.replace(accessor1);
  BOOST_CHECK(accessor1Copied.getId() == accessor1.getId());
  BOOST_CHECK(accessor1Copied.getId() != accessor2.getId());
  ScalarRegisterAccessor<int> accessor2Copied;
  accessor2Copied.replace(accessor2);
  BOOST_CHECK(accessor2Copied.getId() == accessor2.getId());
  BOOST_CHECK(accessor2Copied.getId() != accessor1.getId());

  // compare with accessor for same register but created another time
  ScalarRegisterAccessor<int> accessor1a = device.getScalarRegisterAccessor<int>("APP0/MODULE0", 1, {AccessMode::raw});
  BOOST_CHECK(accessor1a.getId() == accessor1a.getId());
  BOOST_CHECK(accessor1.getId() != accessor1a.getId());
  BOOST_CHECK(accessor2.getId() != accessor1a.getId());

  // test storing the ID
  TransferElementID myId;
  BOOST_CHECK(myId != myId);
  myId = accessor1.getId();
  BOOST_CHECK(myId == accessor1.getId());
  BOOST_CHECK(myId == accessor1Copied.getId());
  BOOST_CHECK(myId != accessor2.getId());
  BOOST_CHECK(myId != accessor1a.getId());

  // check if we can put the ID into an std::unordered_map as a key
  std::unordered_map<TransferElementID, std::string> map1;
  map1.insert({myId, "SomeTest"});
  BOOST_CHECK(map1[accessor1.getId()] == "SomeTest");

  // check if we can put the ID into an std::unordered_map as a value
  std::unordered_map<std::string, TransferElementID> map2;
  map2.insert({"AnotherTest", myId});
  BOOST_CHECK(map2["AnotherTest"] == accessor1.getId());

  // check if we can put the ID into an std::map as a key
  std::map<TransferElementID, std::string> map3;
  map3.insert({myId, "SomeTest"});
  BOOST_CHECK(map3[accessor1.getId()] == "SomeTest");

  // check if we can put the ID into an std::map as a value
  std::unordered_map<std::string, TransferElementID> map4;
  map4.insert({"AnotherTest", myId});
  BOOST_CHECK(map4["AnotherTest"] == accessor1.getId());

  // check if we can put the ID into an std::vector
  std::vector<TransferElementID> vector;
  vector.push_back(myId);
  BOOST_CHECK(vector[0] == accessor1.getId());

  device.close();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteIfDifferent) {
  std::cout << "testWriteIfDifferent" << std::endl;
  setDMapFilePath("dummies.dmap");

  Device device;
  device.open("(WriteCountingDummy?map=goodMapFile.map)");
  auto backend = boost::dynamic_pointer_cast<WriteCountingBackend>(
      BackendFactory::getInstance().createBackend("(WriteCountingDummy?map=goodMapFile.map)"));
  BOOST_CHECK(backend != NULL);

  // obtain register accessor with integral type
  ScalarRegisterAccessor<int> accessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");
  BOOST_CHECK(accessor.isReadOnly() == false);
  BOOST_CHECK(accessor.isReadable());
  BOOST_CHECK(accessor.isWriteable());

  // dummy register accessor for comparison
  DummyRegisterAccessor<int> dummy(backend.get(), "APP0", "WORD_STATUS");

  // Inital write and writeIfDifferent with same value
  accessor = 501;
  accessor.write();
  size_t counterBefore = backend->writeCount;
  accessor.writeIfDifferent(501); // should not write
  size_t counterAfter = backend->writeCount;
  BOOST_CHECK(counterBefore == counterAfter);

  // writeIfDifferent with different value
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent(502); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but explicit version number
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent(502, VersionNumber{}); // should not write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore);

  // writeIfDifferent with different value, and explicit version number
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent(514, VersionNumber{}); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but different DataValidity
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent(514, VersionNumber{nullptr}, DataValidity::faulty); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // writeIfDifferent with same value, but different DataValidity (now back at OK)
  counterBefore = backend->writeCount;
  accessor.writeIfDifferent(514); // should write
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterAfter == counterBefore + 1);

  // test writeIfDifferent for newly created accessor:
  ScalarRegisterAccessor<int> freshAccessor = device.getScalarRegisterAccessor<int>("APP0/WORD_STATUS");
  counterBefore = backend->writeCount;
  VersionNumber vn = freshAccessor.getVersionNumber();
  BOOST_CHECK(vn == VersionNumber{nullptr});
  freshAccessor.writeIfDifferent(0); // should write
  vn = freshAccessor.getVersionNumber();
  BOOST_CHECK(vn != VersionNumber{nullptr});
  counterAfter = backend->writeCount;
  BOOST_CHECK(counterBefore != counterAfter);

  device.close();
}

/**********************************************************************************************************************/
