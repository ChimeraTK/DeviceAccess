// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TransferGroupTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "CopyRegisterDecorator.h"
#include "Device.h"
#include "ExceptionDummyBackend.h"
#include "NDRegisterAccessorDecorator.h"
#include "NumericAddressedLowLevelTransferElement.h"
#include "TransferElementTestAccessor.h"
#include "TransferGroup.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

boost::shared_ptr<TransferElementTestAccessor<int32_t>> makeTETA(AccessModeFlags flags = {}) {
  return boost::make_shared<TransferElementTestAccessor<int32_t>>(flags);
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/** Tests for single specification points */
/**********************************************************************************************************************/
/**********************************************************************************************************************/

/**
 *  Test that replaceTransferElement() is called in addElement()
 *  * \anchor testTransferElement_B_12_1_3 \ref transferElement_B_12_1_3 "B.12.1.3"
 */
BOOST_AUTO_TEST_CASE(test_B_12_1_3) {
  std::cout << "test_B_12_1_3" << std::endl;
  std::map<TransferElementID, bool> foundIds;

  auto A = makeTETA();
  auto B = makeTETA();

  for(size_t i = 0; i < 3; ++i) A->_internalElements.push_back(makeTETA());
  for(size_t i = 0; i < 4; ++i) B->_internalElements.push_back(makeTETA());

  TransferGroup group;

  A->resetCounters();
  group.addAccessor(A);
  foundIds.clear();
  for(auto& id : A->_listReplacementElements) foundIds[id] = true;
  for(auto& e : A->_internalElements) BOOST_CHECK(foundIds[e->getId()] == true);

  A->resetCounters();
  B->resetCounters();
  group.addAccessor(B);
  foundIds.clear();
  for(auto& id : A->_listReplacementElements) foundIds[id] = true;
  for(auto& e : A->_internalElements) BOOST_CHECK(foundIds[e->getId()] == true);
  for(auto& e : B->_internalElements) BOOST_CHECK(foundIds[e->getId()] == true);
  foundIds.clear();
  for(auto& id : B->_listReplacementElements) foundIds[id] = true;
  for(auto& e : A->_internalElements) BOOST_CHECK(foundIds[e->getId()] == true);
  for(auto& e : B->_internalElements) BOOST_CHECK(foundIds[e->getId()] == true);
}

/**********************************************************************************************************************/

/**
 *  Test that individual operations are not allowed
 *  * \anchor testTransferElement_B_12_3 \ref transferElement_B_12_3 "B.12.3"
 */
BOOST_AUTO_TEST_CASE(test_B_12_3) {
  std::cout << "test_B_12_3" << std::endl;
  auto A = makeTETA();
  TransferGroup group;
  group.addAccessor(A);
  BOOST_CHECK_THROW(A->read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(A->readNonBlocking(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(A->readLatest(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(A->write(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(A->writeDestructively(), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 *  Test readOnly groups
 *  * \anchor testTransferElement_B_12_4 \ref transferElement_B_12_4 "B.12.4"
 */
BOOST_AUTO_TEST_CASE(test_B_12_4) {
  std::cout << "test_B_12_4" << std::endl;

  // situation: all accessors are read-write
  {
    auto A = makeTETA();
    auto B = makeTETA();
    TransferGroup group;
    group.addAccessor(A);
    group.addAccessor(B);
    BOOST_CHECK(group.isReadOnly() == false);
  }

  // situation: accessor read-only while adding
  {
    auto A = makeTETA();
    auto B = makeTETA();
    B->_writeable = false;
    TransferGroup group;
    group.addAccessor(A);
    BOOST_CHECK(group.isReadOnly() == false);
    group.addAccessor(B);
    BOOST_CHECK(group.isReadOnly() == true);
  }

  // situation: accessor becomes read-only only later (at runtime error)
  {
    auto A = makeTETA();
    auto B = makeTETA();
    TransferGroup group;
    group.addAccessor(A);
    group.addAccessor(B);
    B->_throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error); // no check intended, just catch
    B->_writeable = false;
    B->_throwRuntimeErrInTransfer = false;
    BOOST_CHECK(group.isReadOnly() == true);
  }
}

/**********************************************************************************************************************/

/**
 *  Test that adding same TE twice throws
 *  * \anchor testTransferElement_B_12_5 \ref transferElement_B_12_5 "B.12.5"
 */
BOOST_AUTO_TEST_CASE(test_B_12_5) {
  std::cout << "test_B_12_5" << std::endl;
  auto A = makeTETA();
  TransferGroup group;
  group.addAccessor(A);
  BOOST_CHECK_THROW(group.addAccessor(A), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 *  Test that adding same TE to two different groups throws
 *  * \anchor testTransferElement_B_12_6 \ref transferElement_B_12_6 "B.12.6"
 */
BOOST_AUTO_TEST_CASE(test_B_12_6) {
  std::cout << "test_B_12_6" << std::endl;
  auto A = makeTETA();
  TransferGroup group1;
  TransferGroup group2;
  group1.addAccessor(A);
  BOOST_CHECK_THROW(group2.addAccessor(A), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 *  Test that adding TE with wait_for_new_data throws
 *  * \anchor testTransferElement_B_12_7 \ref transferElement_B_12_7 "B.12.7"
 */
BOOST_AUTO_TEST_CASE(test_B_12_7) {
  std::cout << "test_B_12_7" << std::endl;
  auto A = makeTETA({AccessMode::wait_for_new_data});
  TransferGroup group;
  BOOST_CHECK_THROW(group.addAccessor(A), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 *  Test optimisation with copy decorator on abstractor level
 *  * \anchor testTransferElement_B_12_8 \ref transferElement_B_12_8 "B.12.8"
 */
BOOST_AUTO_TEST_CASE(test_B_12_8) {
  std::cout << "test_B_12_8" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();
  A->_listMayReplaceElements.insert(B->getId());
  TwoDRegisterAccessor<int32_t> a(A);
  TwoDRegisterAccessor<int32_t> b(B);

  TransferGroup group;
  group.addAccessor(a);
  group.addAccessor(b);

  BOOST_CHECK(b.getHighLevelImplElement() != B);

  A->_setPostReadData = 42;
  group.read();
  BOOST_CHECK_EQUAL(a[0][0], 42);
  BOOST_CHECK_EQUAL(b[0][0], 42);
}

/**********************************************************************************************************************/

// helper macros for next test
// we use macros so line numbers make more sens if checks fail
#define CHECK_COUNTERS_HIGH_LEVEL(te, isWrite, expectedOrderPreMax, expectedOrderPostMin, expectTransfer)              \
  if(!isWrite) {                                                                                                       \
    BOOST_CHECK_EQUAL(te->_preRead_counter, 1);                                                                        \
    BOOST_CHECK_EQUAL(te->_preWrite_counter, 0);                                                                       \
    BOOST_CHECK_EQUAL(te->_postRead_counter, 1);                                                                       \
    BOOST_CHECK_EQUAL(te->_postWrite_counter, 0);                                                                      \
    if(expectTransfer) {                                                                                               \
      BOOST_CHECK_EQUAL(te->_readTransfer_counter, 1);                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  else {                                                                                                               \
    BOOST_CHECK_EQUAL(te->_preRead_counter, 0);                                                                        \
    BOOST_CHECK_EQUAL(te->_preWrite_counter, 1);                                                                       \
    BOOST_CHECK_EQUAL(te->_postRead_counter, 0);                                                                       \
    BOOST_CHECK_EQUAL(te->_postWrite_counter, 1);                                                                      \
    if(expectTransfer) {                                                                                               \
      BOOST_CHECK_EQUAL(te->_writeTransfer_counter, 1);                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
  if(!expectTransfer) {                                                                                                \
    BOOST_CHECK_EQUAL(te->_readTransfer_counter, 0);                                                                   \
    BOOST_CHECK_EQUAL(te->_writeTransfer_counter, 0);                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  BOOST_CHECK(te->_preIndex <= expectedOrderPreMax);                                                                   \
  BOOST_CHECK(te->_postIndex >= expectedOrderPostMin)

#define CHECK_COUNTERS_MID_LEVEL(te)                                                                                   \
  BOOST_CHECK_EQUAL(te->_preRead_counter, 0); /* our test accessor does not delegate this...*/                         \
  BOOST_CHECK_EQUAL(te->_preWrite_counter, 0);                                                                         \
  BOOST_CHECK_EQUAL(te->_readTransfer_counter, 0);                                                                     \
  BOOST_CHECK_EQUAL(te->_writeTransfer_counter, 0);                                                                    \
  BOOST_CHECK_EQUAL(te->_postRead_counter, 0); /* our test accessor does not delegate this... */                       \
  BOOST_CHECK_EQUAL(te->_postWrite_counter, 0)

#define CHECK_COUNTERS_LOW_LEVEL(te, isWrite, expectedOrderMin, expectedOrderMax)                                      \
  BOOST_CHECK_EQUAL(te->_preRead_counter, 0); /* our test accessor does not delegate this... */                        \
  BOOST_CHECK_EQUAL(te->_preWrite_counter, 0);                                                                         \
  if(!isWrite) {                                                                                                       \
    BOOST_CHECK_EQUAL(te->_readTransfer_counter, 1);                                                                   \
    BOOST_CHECK_EQUAL(te->_writeTransfer_counter, 0);                                                                  \
  }                                                                                                                    \
  else {                                                                                                               \
    BOOST_CHECK_EQUAL(te->_readTransfer_counter, 0);                                                                   \
    BOOST_CHECK_EQUAL(te->_writeTransfer_counter, 1);                                                                  \
  }                                                                                                                    \
  BOOST_CHECK_EQUAL(te->_postRead_counter, 0); /* our test accessor does not delegate this... */                       \
  BOOST_CHECK_EQUAL(te->_postWrite_counter, 0);                                                                        \
                                                                                                                       \
  BOOST_CHECK(te->_transferIndex >= expectedOrderMin);                                                                 \
  BOOST_CHECK(te->_transferIndex <= expectedOrderMax)

/**********************************************************************************************************************/

/**
 *  Test read/write operations
 *  * \anchor testTransferElement_B_12_9 \ref transferElement_B_12_9 "B.12.9" (with sub-points)
 */
BOOST_AUTO_TEST_CASE(test_B_12_9) {
  std::cout << "test_B_12_9" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();

  // A has just one low-level transfer element
  for(size_t i = 0; i < 1; ++i) {
    A->_internalElements.push_back(makeTETA());
    A->_hardwareAccessingElements.push_back(A->_internalElements[i]);
  }
  // B has two low-level transfer elements and one additional internal
  for(size_t i = 0; i < 2; ++i) {
    B->_internalElements.push_back(makeTETA());
    B->_hardwareAccessingElements.push_back(B->_internalElements[i]);
  }
  B->_internalElements.push_back(makeTETA());

  TransferGroup group;

  group.addAccessor(A);
  group.addAccessor(B);

  A->resetCounters();
  B->resetCounters();
  for(auto& e : A->_internalElements) e->resetCounters();
  for(auto& e : B->_internalElements) e->resetCounters();
  group.read();
  CHECK_COUNTERS_HIGH_LEVEL(A, false, 1, 5, false);
  CHECK_COUNTERS_HIGH_LEVEL(B, false, 1, 5, false);
  CHECK_COUNTERS_MID_LEVEL(B->_internalElements[2]);
  CHECK_COUNTERS_LOW_LEVEL(A->_internalElements[0], false, 2, 4);
  CHECK_COUNTERS_LOW_LEVEL(B->_internalElements[0], false, 2, 4);
  CHECK_COUNTERS_LOW_LEVEL(B->_internalElements[1], false, 2, 4);
}

/**********************************************************************************************************************/

/**
 *  Test that exceptions are propagated to high-level elements
 *  * \anchor testTransferElement_B_12_10_3 \ref transferElement_B_12_10_3 "B.12.10.3"
 *
 *  INCOMPLETE: Only tested for reading. Test for writing is missing.
 */
BOOST_AUTO_TEST_CASE(test_B_12_10_3) {
  std::cout << "test_B_12_10_3" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();

  // A has just one low-level transfer element
  for(size_t i = 0; i < 1; ++i) {
    A->_internalElements.push_back(makeTETA());
    A->_hardwareAccessingElements.push_back(A->_internalElements[i]);
  }

  TransferGroup group;

  group.addAccessor(A);
  group.addAccessor(B);

  A->resetCounters();
  B->resetCounters();
  for(auto& e : A->_internalElements) e->resetCounters();
  A->_internalElements[0]->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(A->_seenActiveException == A->_internalElements[0]->_thrownException);
  BOOST_CHECK(A->_internalElements[0]->_seenActiveException == nullptr); // the test TE does not delegate the exception
}

/**********************************************************************************************************************/

/**
 *  Test that all postXxx() called even when exception thrown, and exception is thrown in the end
 *  * \anchor testTransferElement_B_12_10_4 \ref transferElement_B_12_10_4 "B.12.10.4"
 */
BOOST_AUTO_TEST_CASE(test_B_12_10_4) {
  std::cout << "test_B_12_10_4" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();

  TransferGroup group;

  group.addAccessor(A);
  group.addAccessor(B);

  // A throws in read
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, false, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, false, 1, 4, true);

  // B throws in read
  A->resetCounters();
  B->resetCounters();
  B->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, false, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, false, 1, 4, true);

  // both throw in read
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  B->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, false, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, false, 1, 4, true);

  // A throws in write
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, true, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, true, 1, 4, true);

  // B throws in write
  A->resetCounters();
  B->resetCounters();
  B->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, true, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, true, 1, 4, true);

  // both throw in write
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  B->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::runtime_error);
  CHECK_COUNTERS_HIGH_LEVEL(A, true, 1, 4, true);
  CHECK_COUNTERS_HIGH_LEVEL(B, true, 1, 4, true);
}

/**********************************************************************************************************************/

/**
 *  Test that thrown exception is identical to the first error
 *  * \anchor testTransferElement_B_12_10_5 \ref transferElement_B_12_10_5 "B.12.10.5"
 *
 *  Attention: With spec V1.RC11 this test has become incomplete! Tests for runtime_errors in preXxx are missing.
 */
BOOST_AUTO_TEST_CASE(test_B_12_10_5) {
  std::cout << "test_B_12_10_5" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();

  TransferGroup group;

  group.addAccessor(A);
  group.addAccessor(B);

  // both throw in read
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  B->_throwRuntimeErrInTransfer = true;
  std::exception_ptr e;
  try {
    group.read();
  }
  catch(...) {
    e = std::current_exception();
  }
  assert(A->_thrownException != B->_thrownException); // make sure our test is sensitive
  if(A->_transferIndex < B->_transferIndex) {
    // A has thrown first
    BOOST_CHECK(e == A->_thrownException);
  }
  else {
    // B has thrown first
    BOOST_CHECK(e == B->_thrownException);
  }

  // both throw in write
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  B->_throwRuntimeErrInTransfer = true;
  try {
    group.write();
  }
  catch(...) {
    e = std::current_exception();
  }
  assert(A->_thrownException != B->_thrownException); // make sure our test is sensitive
  if(A->_transferIndex < B->_transferIndex) {
    // A has thrown first
    BOOST_CHECK(e == A->_thrownException);
  }
  else {
    // B has thrown first
    BOOST_CHECK(e == B->_thrownException);
  }
}

/**********************************************************************************************************************/

/**
 *  Test that updateDataBuffer == false after exception
 *  * \anchor testTransferElement_B_12_11_1 \ref transferElement_B_12_11_1 "B.12.11.1"
 */
BOOST_AUTO_TEST_CASE(test_B_12_11_1) {
  std::cout << "test_B_12_11_1" << std::endl;
  auto A = makeTETA();
  auto B = makeTETA();

  TransferGroup group;

  group.addAccessor(A);
  group.addAccessor(B);

  // A throws in read
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.read(), ChimeraTK::runtime_error); // (no check intended, just catch)
  BOOST_CHECK(A->_updateDataBuffer == false);
  BOOST_CHECK(B->_updateDataBuffer == false);

  // A throws in write
  A->resetCounters();
  B->resetCounters();
  A->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::runtime_error); // (no check intended, just catch)
  BOOST_CHECK(A->_updateDataBuffer == false);
  BOOST_CHECK(B->_updateDataBuffer == false);

  // Nothing throws in read
  A->resetCounters();
  B->resetCounters();
  group.read();
  BOOST_CHECK(A->_updateDataBuffer == true);
  BOOST_CHECK(B->_updateDataBuffer == true);

  // Nothing throws in write
  A->resetCounters();
  B->resetCounters();
  group.write();
  BOOST_CHECK(A->_updateDataBuffer == true);
  BOOST_CHECK(B->_updateDataBuffer == true);
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/** Higher-level tests */
/**********************************************************************************************************************/
/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionHandling) {
  const std::string EXCEPTION_DUMMY_CDD = "(ExceptionDummy:1?map=test3.map)";
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device1;
  ChimeraTK::Device device2;
  ChimeraTK::Device device3;

  device1.open("DUMMYD1");
  auto exceptionDummy = boost::dynamic_pointer_cast<ChimeraTK::ExceptionDummy>(
      ChimeraTK::BackendFactory::getInstance().createBackend(EXCEPTION_DUMMY_CDD));
  device2.open(EXCEPTION_DUMMY_CDD);
  device3.open("DUMMYD2");

  auto accessor1 = device1.getScalarRegisterAccessor<int>("/BOARD/WORD_FIRMWARE");
  auto accessor1w = device1.getScalarRegisterAccessor<int>("/BOARD/WORD_FIRMWARE");
  // accessors 2 and 3 will be merged to a single low level transfer element
  auto accessor2 = device2.getScalarRegisterAccessor<int>("/Integers/signed32");
  auto accessor2w = device2.getScalarRegisterAccessor<int>("/Integers/signed32");
  auto accessor3 = device2.getScalarRegisterAccessor<uint32_t>("/Integers/unsigned32");
  auto accessor3w = device2.getScalarRegisterAccessor<uint32_t>("/Integers/unsigned32");
  auto accessor4 = device2.getScalarRegisterAccessor<float>("/FixedPoint/value");
  auto accessor4w = device2.getScalarRegisterAccessor<float>("/FixedPoint/value");
  auto accessor5 = device3.getScalarRegisterAccessor<int>("/BOARD/WORD_FIRMWARE");
  auto accessor5w = device3.getScalarRegisterAccessor<int>("/BOARD/WORD_FIRMWARE");

  TransferGroup tg;
  tg.addAccessor(accessor2);
  tg.addAccessor(accessor3);
  tg.addAccessor(accessor1);
  tg.addAccessor(accessor4);
  tg.addAccessor(accessor5);

  accessor1 = 1;
  accessor2 = 2;
  accessor3 = 3;
  accessor4 = 4;
  accessor5 = 5;

  accessor1w = int(0xdeadcafe);
  accessor2w = 815;
  accessor3w = 4711;
  accessor4w = 10101010;
  accessor5w = int(0xc01dcafe);
  accessor1w.write();
  accessor2w.write();
  accessor3w.write();
  accessor4w.write();
  accessor5w.write();

  exceptionDummy->throwExceptionRead = true;
  BOOST_CHECK_THROW(tg.read(), ChimeraTK::runtime_error);

  BOOST_CHECK_EQUAL(static_cast<int>(accessor1), 1);
  BOOST_CHECK_EQUAL(static_cast<int>(accessor2), 2);
  BOOST_CHECK_EQUAL(static_cast<int>(accessor3), 3);
  BOOST_CHECK_EQUAL(static_cast<int>(accessor4), 4);
  BOOST_CHECK_EQUAL(static_cast<int>(accessor5), 5);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testAdding) {
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("DUMMYD3");

  auto a1 = device.getOneDRegisterAccessor<int>("ADC/AREA_DMAABLE");
  auto a2 = device.getOneDRegisterAccessor<int>("ADC/AREA_DMAABLE");
  auto a3 = device.getOneDRegisterAccessor<int>("BOARD/WORD_STATUS");
  auto a4 = device.getOneDRegisterAccessor<unsigned int>("ADC/AREA_DMAABLE");

  // slightly redundant to do this test here, this is just a control test still
  // independent of the TransferGroup
  a1[0] = 42;
  a2[0] = 120;
  a3[0] = 123;
  a4[0] = 456;
  BOOST_CHECK(a1[0] == 42);
  a1.write();
  a3.write();
  a3[0] = 654;
  BOOST_CHECK(a2[0] == 120);
  BOOST_CHECK(a3[0] == 654);
  BOOST_CHECK(a4[0] == 456);
  a2.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 654);
  BOOST_CHECK(a4[0] == 456);
  a3.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 456);
  a4.read();
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 42);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(a1);
  BOOST_CHECK(!group.isReadOnly());
  group.addAccessor(a2);
  BOOST_CHECK(group.isReadOnly());
  group.addAccessor(a3);
  group.addAccessor(a4);
  BOOST_CHECK(group.isReadOnly());

  // check if adding an accessor to another group throws an exception
  TransferGroup group2;
  BOOST_CHECK_THROW(group2.addAccessor(a1), ChimeraTK::logic_error);

  // check that reading and writing the accessors which are part of the group
  // throws
  BOOST_CHECK_THROW(a1.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(a1.write(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(a3.read(), ChimeraTK::logic_error);
  BOOST_CHECK_THROW(a4.write(), ChimeraTK::logic_error);

  // during the replace operation, user buffers will be reset, if a replacement
  BOOST_CHECK(a1[0] == 42);
  BOOST_CHECK(a2[0] == 0); // this one was replaced
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);

  // Writing to the register accessor (cooked) buffers should not influence the
  // other accessors in the group.
  a1[0] = 333;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 0);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a2[0] = 666;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 123);
  BOOST_CHECK(a4[0] == 42);
  a3[0] = 999;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 42);
  a4[0] = 111;
  BOOST_CHECK(a1[0] == 333);
  BOOST_CHECK(a2[0] == 666);
  BOOST_CHECK(a3[0] == 999);
  BOOST_CHECK(a4[0] == 111);

  device.close();
}

/**********************************************************************************************************************/

template<typename T>
struct CountingDecorator : NDRegisterAccessorDecorator<T> {
  // if fakeLowLevel is set to true, the decorator will pretend to be the
  // low-level TransferElement.
  CountingDecorator(const boost::shared_ptr<ChimeraTK::TransferElement>& target, bool _fakeLowLevel = false)
  : NDRegisterAccessorDecorator<T>(boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(target)),
    fakeLowLevel(_fakeLowLevel) {
    assert(boost::dynamic_pointer_cast<NDRegisterAccessor<T>>(target) != nullptr); // a bit late but better than
                                                                                   // nothing...
    this->_name = "CD:" + this->_name;
  }

  void doPreRead(TransferType type) override {
    nPreRead++;
    NDRegisterAccessorDecorator<T>::doPreRead(type);
  }

  void doPostRead(TransferType type, bool hasNewData) override {
    nPostRead++;
    NDRegisterAccessorDecorator<T>::doPostRead(type, hasNewData);
  }

  void doPreWrite(TransferType type, VersionNumber versionNumber) override {
    nPreWrite++;
    NDRegisterAccessorDecorator<T>::doPreWrite(type, versionNumber);
  }

  void doPostWrite(TransferType type, VersionNumber versionNumber) override {
    nPostWrite++;
    NDRegisterAccessorDecorator<T>::doPostWrite(type, versionNumber);
  }

  void doReadTransferSynchronously() override {
    nRead++;
    NDRegisterAccessorDecorator<T>::doReadTransferSynchronously();
  }

  bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
    nWrite++;
    return NDRegisterAccessorDecorator<T>::doWriteTransfer(versionNumber);
  }

  std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override {
    if(fakeLowLevel) {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }
    else {
      return NDRegisterAccessorDecorator<T>::getHardwareAccessingElements();
    }
  }

  void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
    if(fakeLowLevel) return;
    if(_target->mayReplaceOther(newElement)) {
      _target = boost::static_pointer_cast<NDRegisterAccessor<T>>(newElement);
    }
    else {
      _target->replaceTransferElement(newElement);
    }
  }

  std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
    if(fakeLowLevel) {
      return {};
    }
    else {
      return NDRegisterAccessorDecorator<T>::getInternalElements();
    }
  }

  bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
    auto casted = boost::dynamic_pointer_cast<CountingDecorator<T> const>(other);
    if(this == casted.get()) {
      return false;
    }
    if(!casted) return false;
    if(_target == casted->_target) return true;
    if(_target->mayReplaceOther(casted->_target)) return true;
    return false;
  }

  void resetCounters() {
    nPreRead = 0;
    nPostRead = 0;
    nPreWrite = 0;
    nPostWrite = 0;
    nRead = 0;
    nReadNonBlocking = 0;
    nReadLatest = 0;
    nWrite = 0;
  }

  bool fakeLowLevel;
  size_t nPreRead{0};
  size_t nPostRead{0};
  size_t nPreWrite{0};
  size_t nPostWrite{0};
  size_t nRead{0};
  size_t nReadNonBlocking{0};
  size_t nReadLatest{0};
  size_t nWrite{0};

  using NDRegisterAccessorDecorator<T>::_target;
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCallsToPrePostFunctionsInDecorator) {
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjecent addresses, one of
  // the registers is in the group two times
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux0_2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // decorate the accessors which we will put into the transfer group, so we can
  // count how often the functions are called
  auto mux0d = boost::make_shared<CountingDecorator<int>>(mux0.getHighLevelImplElement());
  auto mux0_2d = boost::make_shared<CountingDecorator<int>>(mux0_2.getHighLevelImplElement());
  auto mux2d = boost::make_shared<CountingDecorator<int>>(mux2.getHighLevelImplElement());
  auto mux3d = boost::make_shared<CountingDecorator<int>>(mux3.getHighLevelImplElement());

  // place the decorated registers inside the abstractors
  mux0.replace(mux0d);
  mux0_2.replace(mux0_2d);
  mux2.replace(mux2d);
  mux3.replace(mux3d);

  // create the same register accessors again, so we have a second set not part
  // of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  BOOST_CHECK(mux0d->_target != mux0_2d->_target);
  BOOST_CHECK(mux0.getHighLevelImplElement() == mux0d);
  BOOST_CHECK(mux0_2.getHighLevelImplElement() == mux0_2d);

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux0_2);
  group.addAccessor(mux2);
  group.addAccessor(mux3);

  BOOST_CHECK(mux0.getHighLevelImplElement() == mux0d);
  BOOST_CHECK(mux0_2.getHighLevelImplElement() != mux0_2d);
  BOOST_CHECK(
      boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(mux0_2.getHighLevelImplElement()) != nullptr);

  // write some data to the registers (without the TransferGroup)
  mux0b = 18;
  mux0b.write();
  mux2b = 22;
  mux2b.write();
  mux3b = 23;
  mux3b.write();

  // read through transfer group
  group.read();

  BOOST_CHECK_EQUAL(int(mux0), 18);
  BOOST_CHECK_EQUAL(int(mux0_2), 18);

  // we don't know which of the accessors has been eliminated (and this is
  // actually a random choice at runtime)
  BOOST_CHECK((mux0d->nPreRead == 1 && mux0_2d->nPreRead == 0) || (mux0d->nPreRead == 0 && mux0_2d->nPreRead == 1));
  if(mux0d->nPreRead == 1) {
    BOOST_CHECK_EQUAL(mux0d->nPostRead, 1);
    BOOST_CHECK_EQUAL(mux0_2d->nPreRead, 0);
    BOOST_CHECK_EQUAL(mux0_2d->nPostRead, 0);
  }
  else {
    BOOST_CHECK_EQUAL(mux0_2d->nPostRead, 1);
    BOOST_CHECK_EQUAL(mux0d->nPreRead, 0);
    BOOST_CHECK_EQUAL(mux0d->nPostRead, 0);
  }
  BOOST_CHECK_EQUAL(mux0d->nRead, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nRead, 0);
  BOOST_CHECK_EQUAL(mux0d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux0d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux0d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux0d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux0d->nWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nWrite, 0);

  BOOST_CHECK_EQUAL(int(mux2), 22);
  BOOST_CHECK_EQUAL(mux2d->nPreRead, 1);
  BOOST_CHECK_EQUAL(mux2d->nPostRead, 1);
  BOOST_CHECK_EQUAL(mux2d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux2d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux2d->nRead, 0);
  BOOST_CHECK_EQUAL(mux2d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux2d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux2d->nWrite, 0);

  BOOST_CHECK_EQUAL(int(mux3), 23);
  BOOST_CHECK_EQUAL(mux3d->nPreRead, 1);
  BOOST_CHECK_EQUAL(mux3d->nPostRead, 1);
  BOOST_CHECK_EQUAL(mux3d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux3d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux3d->nRead, 0);
  BOOST_CHECK_EQUAL(mux3d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux3d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux3d->nWrite, 0);

  mux0d->resetCounters();
  mux0_2d->resetCounters();
  mux2d->resetCounters();
  mux3d->resetCounters();

  // write through transfer group is not possible, since it is read-only
  mux0 = 24;
  mux0_2 = 24;
  mux2 = 30;
  mux3 = 33;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testCallsToPrePostFunctionsInLowLevel) {
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("DUMMYD3");

  // create register accessors of four registers with adjacent addresses
  auto mux0 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux0_2 = mux0; // make duplicate of one accessor
  auto mux2 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3 = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  // decorate the accessors which we will put into the transfer group, so we can
  // count how often the functions are called
  auto mux0d = boost::make_shared<CountingDecorator<int>>(mux0.getHighLevelImplElement(), true);
  auto mux0_2d = boost::make_shared<CountingDecorator<int>>(mux0_2.getHighLevelImplElement(), true);
  auto mux2d = boost::make_shared<CountingDecorator<int>>(mux2.getHighLevelImplElement(), true);
  auto mux3d = boost::make_shared<CountingDecorator<int>>(mux3.getHighLevelImplElement(), true);

  // decorate another time
  auto mux0d2 = boost::make_shared<CountingDecorator<int>>(mux0d);
  auto mux0_2d2 = boost::make_shared<CountingDecorator<int>>(mux0_2d);
  auto mux2d2 = boost::make_shared<CountingDecorator<int>>(mux2d);
  auto mux3d2 = boost::make_shared<CountingDecorator<int>>(mux3d);

  // place the decorated registers inside the abstractors
  mux0.replace(mux0d2);
  mux0_2.replace(mux0_2d2);
  mux2.replace(mux2d2);
  mux3.replace(mux3d2);

  // create the same register accessors again, so we have a second set not part
  // of the transfer group
  auto mux0b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_0");
  auto mux2b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_2");
  auto mux3b = device.getScalarRegisterAccessor<int>("/ADC/WORD_CLK_MUX_3");

  BOOST_CHECK(mux0d->_target == mux0_2d->_target);
  BOOST_CHECK(mux0d2->_target == mux0d);
  BOOST_CHECK(mux0_2d2->_target == mux0_2d);
  BOOST_CHECK(mux2d2->_target == mux2d);
  BOOST_CHECK(mux3d2->_target == mux3d);

  // add accessors to the transfer group
  TransferGroup group;
  group.addAccessor(mux0);
  group.addAccessor(mux0_2);
  group.addAccessor(mux2);
  group.addAccessor(mux3);

  BOOST_CHECK(mux0d->_target == mux0_2d->_target);
  BOOST_CHECK(
      boost::dynamic_pointer_cast<ChimeraTK::CopyRegisterDecoratorTrait>(mux0_2.getHighLevelImplElement()) != nullptr);
  BOOST_CHECK(mux2d2->_target == mux2d);
  BOOST_CHECK(mux3d2->_target == mux3d);

  // write some data to the registers (without the TransferGroup)
  mux0b = 18;
  mux0b.write();
  mux2b = 22;
  mux2b.write();
  mux3b = 23;
  mux3b.write();

  // read through transfer group
  group.read();

  BOOST_CHECK_EQUAL(int(mux0), 18);
  BOOST_CHECK_EQUAL(int(mux0_2), 18);

  // we don't know which of the accessors has been eliminated (and this is
  // actually a random choice at runtime)
  BOOST_CHECK((mux0d->nRead == 1 && mux0_2d->nRead == 0) || (mux0d->nRead == 0 && mux0_2d->nRead == 1));
  if(mux0d->nRead == 1) {
    BOOST_CHECK_EQUAL(mux0d->nPreRead, 1);
    BOOST_CHECK_EQUAL(mux0d->nPostRead, 1);
    BOOST_CHECK_EQUAL(mux0_2d->nPreRead, 0);
    BOOST_CHECK_EQUAL(mux0_2d->nPostRead, 0);
  }
  else {
    BOOST_CHECK_EQUAL(mux0_2d->nPreRead, 1);
    BOOST_CHECK_EQUAL(mux0_2d->nPostRead, 1);
    BOOST_CHECK_EQUAL(mux0d->nPreRead, 0);
    BOOST_CHECK_EQUAL(mux0d->nPostRead, 0);
  }
  BOOST_CHECK_EQUAL(mux0d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux0d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux0d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux0d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux0d->nWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux0_2d->nWrite, 0);

  BOOST_CHECK_EQUAL(int(mux2), 22);
  BOOST_CHECK_EQUAL(mux2d->nPreRead, 1);
  BOOST_CHECK_EQUAL(mux2d->nPostRead, 1);
  BOOST_CHECK_EQUAL(mux2d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux2d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux2d->nRead, 1);
  BOOST_CHECK_EQUAL(mux2d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux2d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux2d->nWrite, 0);

  BOOST_CHECK_EQUAL(int(mux3), 23);
  BOOST_CHECK_EQUAL(mux3d->nPreRead, 1);
  BOOST_CHECK_EQUAL(mux3d->nPostRead, 1);
  BOOST_CHECK_EQUAL(mux3d->nPreWrite, 0);
  BOOST_CHECK_EQUAL(mux3d->nPostWrite, 0);
  BOOST_CHECK_EQUAL(mux3d->nRead, 1);
  BOOST_CHECK_EQUAL(mux3d->nReadNonBlocking, 0);
  BOOST_CHECK_EQUAL(mux3d->nReadLatest, 0);
  BOOST_CHECK_EQUAL(mux3d->nWrite, 0);

  mux0d->resetCounters();
  mux0_2d->resetCounters();
  mux2d->resetCounters();
  mux3d->resetCounters();

  // write through transfer group
  /// @todo FIXME transfer group should become read-only in this scenario!!!
  mux0 = 24;
  mux0_2 = 24;
  mux2 = 30;
  mux3 = 33;
  BOOST_CHECK_THROW(group.write(), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE(testTemporaryAbstractorWorks) {
  // Testing that adding a plain transfer element to the TransferGroup still works as expected.
  BackendFactory::getInstance().setDMapFilePath("dummies.dmap");
  ChimeraTK::Device device;

  device.open("(dummy?map=mtcadummy.map)");

  TransferGroup group;
  auto a = device.getScalarRegisterAccessor<int32_t>("BOARD.WORD_FIRMWARE");
  auto b = device.getBackend()->getRegisterAccessor<int32_t>("BOARD.WORD_FIRMWARE", 1, 0, {});

  auto c = device.getScalarRegisterAccessor<int32_t>("BOARD.WORD_FIRMWARE");

  group.addAccessor(a);
  c = 12;
  c.write();

  group.read();
  BOOST_CHECK_EQUAL(a, 12);
  group.addAccessor(b);

  c = 13;
  c.write();

  group.read();
  BOOST_CHECK_EQUAL(a, 13);
  BOOST_CHECK_EQUAL(b->accessChannel(0)[0], 13);
}

/**********************************************************************************************************************/
