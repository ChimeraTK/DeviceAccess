// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

/**********************************************************************************************************************/
/*
 *  Tests for the TransferElement base class.
 *
 *  IMPORTANT: READ BEFORE MODIFYING!
 *
 *  These tests test whether the implementation of the TransferElement base class (and maybe potentially at some point
 *  the NDRegisterAccessor base class) behave according to the specification. The purpose of these tests is NOT to
 *  verify that the specifications are correct or complete. These tests are intentionally low-level and test exactly
 *  point-by-point the (low-level) behaviour described in the specification. It intentionally does NOT test whether the
 *  high-level ideas behind the specification works. This is outside the scope of this test, adding it here would
 *  prevent an exact identification of the tested parts of the specification.
 */
/**********************************************************************************************************************/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TransferElementTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DeviceBackendImpl.h"
#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElementTestAccessor.h"

#include <boost/make_shared.hpp>

using namespace ChimeraTK;

/**********************************************************************************************************************/

/**
 *  Test the return values of readNonBlocking()/readLatest() for synchronous reads
 *  * \anchor testTransferElement_B_3_1_2_4 \ref transferElement_B_3_1_2_4 "B.3.1.2.4"
 */
BOOST_AUTO_TEST_CASE(B_3_1_2_4) {
  TransferElementTestAccessor<int32_t> accessor({});
  bool ret;

  accessor.resetCounters();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(ret == true);

  accessor.resetCounters();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == true);
}

/**********************************************************************************************************************/

/**
 *  Test read() blocks until data available
 *  * \anchor testTransferElement_B_3_1_3_1 \ref transferElement_B_3_1_3_1 "B.3.1.3.1"
 */
BOOST_AUTO_TEST_CASE(test_B_3_1_3_1) {
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    std::atomic<bool> readCompleted{false};
    std::thread t([&] {
      accessor.read();
      readCompleted = true;
    });
    usleep(10000);
    BOOST_CHECK(!readCompleted);
    accessor.push();
    t.join();
  }
}

/**********************************************************************************************************************/

/**
 *  Test readNonBlocking() returns data availability
 *  * \anchor testTransferElement_B_3_1_3_2 \ref transferElement_B_3_1_3_2 "B.3.1.3.2"
 */
BOOST_AUTO_TEST_CASE(test_B_3_1_3_2) {
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    BOOST_CHECK(accessor.readNonBlocking() == false);
    accessor.push();
    BOOST_CHECK(accessor.readNonBlocking() == true);
    BOOST_CHECK(accessor.readNonBlocking() == false);
    accessor.push();
    accessor.push();
    BOOST_CHECK(accessor.readNonBlocking() == true);
    BOOST_CHECK(accessor.readNonBlocking() == true);
    BOOST_CHECK(accessor.readNonBlocking() == false);
  }
}

/**********************************************************************************************************************/

/**
 *  Test the convenience function readLatest().
 *
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_3_1_4 \ref transferElement_B_3_1_4 "B.3.1.4"
 */
BOOST_AUTO_TEST_CASE(B_3_1_4) {
  TransferElementTestAccessor<int32_t> accessor({});
  TransferElementTestAccessor<int32_t> asyncAccessor({AccessMode::wait_for_new_data});
  bool ret;

  // Without AccessMode::wait_for_new_data
  accessor.resetCounters();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == true);
  BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 1);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

  // With AccessMode::wait_for_new_data
  asyncAccessor.resetCounters();
  ret = asyncAccessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK_EQUAL(asyncAccessor._postRead_counter, 1); // no update -> one call to readNonBlocking()

  asyncAccessor.resetCounters();
  asyncAccessor.push();
  ret = asyncAccessor.readLatest();
  BOOST_CHECK(ret == true);
  BOOST_CHECK_EQUAL(asyncAccessor._postRead_counter, 2); // one update -> two calls to readNonBlocking()

  asyncAccessor.resetCounters();
  ret = asyncAccessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK_EQUAL(asyncAccessor._postRead_counter, 1);

  asyncAccessor.resetCounters();
  while(asyncAccessor.push()) continue; // fill the queue
  ret = asyncAccessor.readLatest();
  BOOST_CHECK(ret == true);
  BOOST_CHECK_EQUAL(asyncAccessor._postRead_counter, 4); // _readQueue.size() updates -> one more readNonBlocking() call

  asyncAccessor.resetCounters();
  ret = asyncAccessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK_EQUAL(asyncAccessor._postRead_counter, 1);
}

/**********************************************************************************************************************/

/**
 *  Helper macro for test_B_4 to avoid code duplication. Do not use for other tests.
 */
#define TEST_TRANSFER_SEQUENCE(transferElement, expectedType, expectTransferExecution)                                 \
  assert(expectedType != TransferType::readLatest); /* doesn't exist any more -> remove from definition! */            \
                                                                                                                       \
  /* correct stages in read */                                                                                         \
  if((expectedType == TransferType::read || expectedType == TransferType::readNonBlocking)) {                          \
    BOOST_CHECK_EQUAL(transferElement._preRead_counter, 1);                                                            \
    if(expectTransferExecution) {                                                                                      \
      BOOST_CHECK_EQUAL(transferElement._readTransfer_counter, 1);                                                     \
    }                                                                                                                  \
    else {                                                                                                             \
      BOOST_CHECK_EQUAL(transferElement._readTransfer_counter, 0);                                                     \
    }                                                                                                                  \
    BOOST_CHECK_EQUAL(transferElement._postRead_counter, 1);                                                           \
  }                                                                                                                    \
                                                                                                                       \
  /* correct stages in non-destructive write */                                                                        \
  if(expectedType == TransferType::write) {                                                                            \
    BOOST_CHECK_EQUAL(transferElement._preWrite_counter, 1);                                                           \
    if(expectTransferExecution) {                                                                                      \
      BOOST_CHECK_EQUAL(transferElement._writeTransfer_counter, 1);                                                    \
    }                                                                                                                  \
    else {                                                                                                             \
      BOOST_CHECK_EQUAL(transferElement._writeTransfer_counter, 0);                                                    \
    }                                                                                                                  \
    BOOST_CHECK_EQUAL(transferElement._postWrite_counter, 1);                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /* correct stages in destructive write */                                                                            \
  if(expectedType == TransferType::writeDestructively) {                                                               \
    BOOST_CHECK_EQUAL(transferElement._preWrite_counter, 1);                                                           \
    if(expectTransferExecution) {                                                                                      \
      BOOST_CHECK_EQUAL(transferElement._writeTransferDestructively_counter, 1);                                       \
    }                                                                                                                  \
    else {                                                                                                             \
      BOOST_CHECK_EQUAL(transferElement._writeTransferDestructively_counter, 0);                                       \
    }                                                                                                                  \
    BOOST_CHECK_EQUAL(transferElement._postWrite_counter, 1);                                                          \
  }                                                                                                                    \
                                                                                                                       \
  /* order of stages */                                                                                                \
  BOOST_CHECK_EQUAL(transferElement._preIndex, 0);                                                                     \
  if(expectTransferExecution) {                                                                                        \
    BOOST_CHECK_EQUAL(transferElement._transferIndex, 1);                                                              \
    BOOST_CHECK_EQUAL(transferElement._postIndex, 2);                                                                  \
  }                                                                                                                    \
  else {                                                                                                               \
    BOOST_CHECK_EQUAL(transferElement._postIndex, 1);                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  /* check transfer type passed to preXxx() and postXxx() */                                                           \
  BOOST_CHECK(transferElement._transferType_pre == expectedType);                                                      \
  BOOST_CHECK(transferElement._transferType_post == expectedType)

/**********************************************************************************************************************/

/**
 *  Test order of stages in high-level functions.
 *  * \anchor testTransferElement_B_4_order \ref transferElement_B_4 "B.4" (just the order of the stages)
 */
BOOST_AUTO_TEST_CASE(test_B_4) {
  {
    TransferElementTestAccessor<int32_t> accessor({});

    accessor.resetCounters();
    accessor.read();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, true);

    accessor.resetCounters();
    accessor.readNonBlocking();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, true);
    accessor.resetCounters();

    accessor.write();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true);

    accessor.resetCounters();
    accessor.write();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true);

    accessor.resetCounters();
    accessor.writeDestructively();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, true);

    accessor.resetCounters();
    accessor.writeDestructively();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, true);
  }

  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    accessor.push();
    accessor.read();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, false);

    accessor.resetCounters();
    accessor.readNonBlocking();
    TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false);
  }
}

/**********************************************************************************************************************/

/**
 *  Test preXxx
 *  * \anchor testTransferElement_B_4_1 \ref transferElement_B_4_1 "B.4.1"
 */
BOOST_AUTO_TEST_CASE(test_B_4_1) {
  TransferElementTestAccessor<int32_t> accessor({});

  accessor.resetCounters();
  accessor.preRead(TransferType::read);
  BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
  BOOST_CHECK(accessor._transferType_pre == TransferType::read);
  accessor.readTransfer();
  accessor.postRead(TransferType::read, true);

  accessor.resetCounters();
  accessor.preRead(TransferType::readNonBlocking);
  BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
  BOOST_CHECK(accessor._transferType_pre == TransferType::readNonBlocking);
  accessor.readTransfer();
  accessor.postRead(TransferType::readNonBlocking, true);

  VersionNumber v{};
  accessor.resetCounters();
  accessor.preWrite(TransferType::write, v);
  BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
  BOOST_CHECK(accessor._transferType_pre == TransferType::write);
  BOOST_CHECK(accessor._preWrite_version == v);
  accessor.writeTransfer(v);
  accessor.postWrite(TransferType::write, v);

  v = {};
  accessor.resetCounters();
  accessor.preWrite(TransferType::writeDestructively, v);
  BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
  BOOST_CHECK(accessor._transferType_pre == TransferType::writeDestructively);
  BOOST_CHECK(accessor._preWrite_version == v);
  accessor.writeTransfer(v);
  accessor.postWrite(TransferType::writeDestructively, v);
}

/**********************************************************************************************************************/

/**
 *  Test readTransfer()
 *  * \anchor testTransferElement_B_4_2_1 \ref transferElement_B_4_2_1 "B.4.2.1"
 */
BOOST_AUTO_TEST_CASE(test_B_4_2_1) {
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    accessor.preRead(TransferType::read);
    std::atomic<bool> readCompleted{false};
    std::thread t([&] {
      accessor.readTransfer();
      readCompleted = true;
    });
    usleep(10000);
    BOOST_CHECK(!readCompleted);
    accessor.push();
    t.join();
    accessor.postRead(TransferType::read, true);
  }
  {
    TransferElementTestAccessor<int32_t> accessor({});

    accessor.resetCounters();
    accessor.preRead(TransferType::read);
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);
    accessor.readTransfer();
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 1);
    accessor.postRead(TransferType::read, true);
  }
}

/**********************************************************************************************************************/

/**
 *  Test readTransferNonBlocking()
 *  * \anchor testTransferElement_B_4_2_2 \ref transferElement_B_4_2_2 "B.4.2.2"
 */
BOOST_AUTO_TEST_CASE(test_B_4_2_2) {
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    accessor.preRead(TransferType::readNonBlocking);
    BOOST_CHECK(accessor.readTransferNonBlocking() == false);
    accessor.postRead(TransferType::readNonBlocking, true);

    accessor.resetCounters();
    accessor.push();
    accessor.preRead(TransferType::readNonBlocking);
    BOOST_CHECK(accessor.readTransferNonBlocking() == true);
    accessor.postRead(TransferType::readNonBlocking, true);
  }
  {
    TransferElementTestAccessor<int32_t> accessor({});

    accessor.resetCounters();
    accessor.preRead(TransferType::readNonBlocking);
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);
    BOOST_CHECK(accessor.readTransferNonBlocking() == true);
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 1);
    accessor.postRead(TransferType::readNonBlocking, true);
  }
}

/**********************************************************************************************************************/

/**
 *  Test writeTransfer()/writeTransferDestructively()
 *  * \anchor testTransferElement_B_4_2_3 \ref transferElement_B_4_2_3 "B.4.2.3"
 */
BOOST_AUTO_TEST_CASE(test_B_4_2_3) {
  TransferElementTestAccessor<int32_t> accessor({});
  VersionNumber v{nullptr};

  accessor.resetCounters();
  v = {};
  accessor.preWrite(TransferType::write, v);
  BOOST_CHECK_EQUAL(accessor._writeTransfer_counter, 0);
  accessor.writeTransfer(v);
  BOOST_CHECK_EQUAL(accessor._writeTransfer_counter, 1);
  BOOST_CHECK(accessor._writeTransfer_version == v);
  accessor.postWrite(TransferType::write, v);

  accessor.resetCounters();
  v = {};
  accessor.preWrite(TransferType::writeDestructively, v);
  BOOST_CHECK_EQUAL(accessor._writeTransferDestructively_counter, 0);
  accessor.writeTransferDestructively(v);
  BOOST_CHECK_EQUAL(accessor._writeTransferDestructively_counter, 1);
  BOOST_CHECK(accessor._writeTransfer_version == v);
  accessor.postWrite(TransferType::writeDestructively, v);
}

/**********************************************************************************************************************/

/**
 *  Test postXxx
 *  * \anchor testTransferElement_B_4_3 \ref transferElement_B_4_3 "B.4.3"
 */
BOOST_AUTO_TEST_CASE(test_B_4_3) {
  TransferElementTestAccessor<int32_t> accessor({});

  accessor.resetCounters();
  accessor.preRead(TransferType::read);
  accessor.readTransfer();
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 0);
  accessor.postRead(TransferType::read, true);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
  BOOST_CHECK(accessor._transferType_post == TransferType::read);
  BOOST_CHECK(accessor._updateDataBuffer == true);

  accessor.resetCounters();
  accessor.preRead(TransferType::read);
  accessor.readTransfer();
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 0);
  accessor.postRead(TransferType::read, false);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
  BOOST_CHECK(accessor._transferType_post == TransferType::read);
  BOOST_CHECK(accessor._updateDataBuffer == false);

  accessor.resetCounters();
  accessor.preRead(TransferType::readNonBlocking);
  accessor.readTransferNonBlocking();
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 0);
  accessor.postRead(TransferType::readNonBlocking, true);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
  BOOST_CHECK(accessor._transferType_post == TransferType::readNonBlocking);
  BOOST_CHECK(accessor._updateDataBuffer == true);

  accessor.resetCounters();
  accessor.preRead(TransferType::readNonBlocking);
  accessor.readTransferNonBlocking();
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 0);
  accessor.postRead(TransferType::readNonBlocking, false);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
  BOOST_CHECK(accessor._transferType_post == TransferType::readNonBlocking);
  BOOST_CHECK(accessor._updateDataBuffer == false);
}

/**********************************************************************************************************************/

/**
 *  Test that the current VersionNumber is updated in postWrite
 *  * \anchor testTransferElement_B_4_3_2 \ref transferElement_B_4_3_2 "B.4.3.2",
 */
BOOST_AUTO_TEST_CASE(B_4_3_2) {
  TransferElementTestAccessor<int32_t> accessor({});

  accessor.resetCounters();
  VersionNumber v1{};
  accessor.preWrite(TransferType::write, v1);
  accessor.writeTransfer(v1);
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr));
  accessor.postWrite(TransferType::write, v1);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
}

/**********************************************************************************************************************/

/**
 *  Test pre/post paring for exceptions
 *  * \anchor testTransferElement_B_5_1 \ref transferElement_B_5_1 "B.5.1",
 */
BOOST_AUTO_TEST_CASE(test_B_5_1) {
  {
    TransferElementTestAccessor<int32_t> accessor({});

    // read()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    // readNonBlocking() (there is a slightly different code path in the implementation despite identical behaviour)
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    // write()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

    // writeDestructively()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);
  }

  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    // read()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor.putRuntimeErrorOnQueue();
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    // readNonBlocking() (there is a slightly different code path in the implementation despite identical behaviour)
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

    accessor.resetCounters();
    accessor.putRuntimeErrorOnQueue();
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
    BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
  }
}

/**********************************************************************************************************************/

/**
 *  Test that duplicate calls to preXxx/postXxx are ignored.
 *  * \anchor testTransferElement_B_5_2 \ref transferElement_B_5_2 "B.5.2".
 */
BOOST_AUTO_TEST_CASE(test_B_5_2) {
  TransferElementTestAccessor<int32_t> accessor({});

  // read()
  accessor.resetCounters();
  accessor.preRead(TransferType::read);
  accessor.preRead(TransferType::read);
  accessor.preRead(TransferType::read);
  accessor.readTransfer();
  accessor.postRead(TransferType::read, true);
  accessor.postRead(TransferType::read, true);
  accessor.postRead(TransferType::read, true);
  BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

  // write()
  accessor.resetCounters();
  VersionNumber v{};
  accessor.preWrite(TransferType::write, v);
  accessor.preWrite(TransferType::write, v);
  accessor.preWrite(TransferType::write, v);
  accessor.writeTransfer(v);
  accessor.postWrite(TransferType::write, v);
  accessor.postWrite(TransferType::write, v);
  accessor.postWrite(TransferType::write, v);
  BOOST_CHECK_EQUAL(accessor._preWrite_counter, 1);
  BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);

  // no need to test all read and write types, since the mechanism does not depend on the type.
}

/**********************************************************************************************************************/

/**
 *  Test skipping transfer if exception in preXxx()
 *  * \anchor testTransferElement_B_6_1 \ref transferElement_B_6_1 "B.6.1"
 */
BOOST_AUTO_TEST_CASE(test_B_6_1) {
  {
    TransferElementTestAccessor<int32_t> accessor({});

    // read()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);

    // readNonBlocking()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readTransfer_counter, 0);

    // write()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._writeTransfer_counter, 0);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._writeTransfer_counter, 0);

    // writeDestructively()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._writeTransferDestructively_counter, 0);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._writeTransferDestructively_counter, 0);
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    // Place one element into _readQueue, which needs to stay there. If it is removed from the queue, a transfer
    // function has been executed.
    accessor.push();
    assert(accessor._readQueue.read_available() == 1);

    // read()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);

    // readNonBlocking()
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);

    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);
  }
}

/**********************************************************************************************************************/

/**
 *  Test xxxYyy() catches exceptions, not preXxx()/xxxTransferYyy() (outermost decorator sees exception)
 *  * \anchor testTransferElement_B_6_2 \ref transferElement_B_6_2 "B.6.2"
 */
BOOST_AUTO_TEST_CASE(test_B_6_2) {
  TransferElementTestAccessor<int32_t> accessor({});
  std::exception_ptr myException;
  VersionNumber v{nullptr};

  // Simply check that preXxx()/xxxTransferYyy() are throwing. Since previous tests have shown that the stages are
  // correctly called even with exceptions, we can then conclude that the code calling preXxx()/xxxTransferYyy() is
  // catching the exception to delay it.

  // Note, this test uses synchronous read operations only, since asynchronous read operations are anyway executed in
  // the outermost decorator. This is ensured already by the API, so nothing can be tested there.

  // The test acts like a decorator, the "accessor" is its target.

  // test preRead
  // ------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.preRead(TransferType::read), ChimeraTK::runtime_error);
  accessor.postRead(TransferType::read, false); // just complete the sequence as required by the spec

  // test preWrite
  // ------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.preWrite(TransferType::write, {}), ChimeraTK::runtime_error);
  accessor.postWrite(TransferType::write, {}); // just complete the sequence as required by the spec

  // test readTransfer (sync)
  // ------------------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true; // target shall throw in the transfer

  // this is like doPreRead of the decorator
  accessor.preRead(TransferType::read);

  // this is like doReadTransferSynchronously of the decorator, including the exception handling normally implemented
  // in the TransferElement base class
  myException = std::exception_ptr{nullptr};
  try {
    accessor.readTransfer();
    BOOST_ERROR("readTransfer() must throw a ChimeraTK::runtime_error");
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostRead of the decorator (we are cheating here a bit and discard the exception...)
  accessor.postRead(TransferType::read, false);

  // test readTransferNonBlocking (sync) -> not sure if this is needed, decorators won't delegate to this?
  // -----------------------------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true; // target shall throw in the transfer

  // this is like doPreRead of the decorator
  accessor.preRead(TransferType::readNonBlocking);

  // this is like doReadTransferSynchronously of the decorator, including the exception handling normally implemented
  // in the TransferElement base class
  myException = std::exception_ptr{nullptr};
  try {
    accessor.readTransferNonBlocking();
    BOOST_ERROR("readTransferNonBlocking() must throw a ChimeraTK::runtime_error");
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostRead of the decorator (cheating again...)
  accessor.postRead(TransferType::readNonBlocking, false);

  // test writeTransfer
  // ------------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  v = {};

  // this is like doPreWrite of the decorator
  accessor.preWrite(TransferType::write, v);

  // this is like doWriteTransfer of the decorator
  try {
    accessor.writeTransfer(v);
    BOOST_ERROR("writeTransfer() must throw a ChimeraTK::runtime_error");
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostWrite of the decorator (cheating again...)
  accessor.postWrite(TransferType::write, v);

  // test writeTransferDestructively
  // -------------------------------
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  v = {};

  // this is like doPreWrite of the decorator
  accessor.preWrite(TransferType::writeDestructively, v);

  // this is like doWriteTransferDestructively of the decorator
  try {
    accessor.writeTransferDestructively(v);
    BOOST_ERROR("writeTransferDestructively() must throw a ChimeraTK::runtime_error");
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostWrite of the decorator (cheating again...)
  accessor.postWrite(TransferType::writeDestructively, v);
}

/**********************************************************************************************************************/

/**
 *  Test return of writeTransfer and writeTransferDestructively
 *  * \anchor testTransferElement_B_7_2 \ref transferElement_B_7_2 "B.7.2"
 */
BOOST_AUTO_TEST_CASE(test_B_7_2) {
  {
    TransferElementTestAccessor<int32_t> accessor({});
    bool ret;
    VersionNumber v{nullptr};

    accessor.resetCounters();
    v = {};
    accessor._previousDataLost = false;
    accessor.preWrite(TransferType::write, v);
    ret = accessor.writeTransfer(v);
    BOOST_CHECK(ret == false);
    accessor.postWrite(TransferType::write, v);

    accessor.resetCounters();
    v = {};
    accessor._previousDataLost = true;
    accessor.preWrite(TransferType::write, v);
    ret = accessor.writeTransfer(v);
    BOOST_CHECK(ret == true);
    accessor.postWrite(TransferType::write, v);

    accessor.resetCounters();
    v = {};
    accessor._previousDataLost = false;
    accessor.preWrite(TransferType::writeDestructively, v);
    ret = accessor.writeTransferDestructively(v);
    BOOST_CHECK(ret == false);
    accessor.postWrite(TransferType::writeDestructively, v);

    accessor.resetCounters();
    v = {};
    accessor._previousDataLost = true;
    accessor.preWrite(TransferType::writeDestructively, v);
    ret = accessor.writeTransferDestructively(v);
    BOOST_CHECK(ret == true);
    accessor.postWrite(TransferType::writeDestructively, v);
  }
}

/**********************************************************************************************************************/

/**
 *  Test updateBuffer argument in postRead without exceptions
 *  * \anchor testTransferElement_B_7_3 \ref transferElement_B_7_3 "B.7.3"
 */
BOOST_AUTO_TEST_CASE(test_B_7_3) {
  {
    TransferElementTestAccessor<int32_t> accessor({});

    accessor.resetCounters();
    accessor.read();
    BOOST_CHECK(accessor._updateDataBuffer == true);

    accessor.resetCounters();
    accessor.readNonBlocking();
    BOOST_CHECK(accessor._updateDataBuffer == true); // (cf. B.4.2.2 second sub-point)
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    accessor.resetCounters();
    accessor.push();
    accessor.read();
    BOOST_CHECK(accessor._updateDataBuffer == true);

    accessor.resetCounters();
    accessor.push();
    accessor.readNonBlocking();
    BOOST_CHECK(accessor._updateDataBuffer == true);

    accessor.resetCounters();
    accessor.readNonBlocking();
    BOOST_CHECK(accessor._updateDataBuffer == false);
  }
}

/**********************************************************************************************************************/

/**
 *  Test postRead updateDataBuffer argument in case of exceptions
 *  * \anchor testTransferElement_B_7_4 \ref transferElement_B_7_4 "B.7.4"
 */
BOOST_AUTO_TEST_CASE(test_B_7_4) {
  {
    TransferElementTestAccessor<int32_t> accessor({});

    // logic_error in preRead
    accessor.resetCounters();
    accessor._throwLogicErr = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);

    // runtime_error in preRead
    accessor.resetCounters();
    accessor._throwRuntimeErrInPre = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);

    // runtime_error in readTransfer (sync)
    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);

    // runtime_error in readTransferNonBlocking (sync)
    accessor.resetCounters();
    accessor._throwRuntimeErrInTransfer = true;
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

    // runtime_error in readTransfer (async)
    accessor.resetCounters();
    accessor.putRuntimeErrorOnQueue();
    BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);

    // runtime_error in readTransferNonBlocking (async)
    accessor.resetCounters();
    accessor.putRuntimeErrorOnQueue();
    BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
    BOOST_CHECK(accessor._updateDataBuffer == false);
  }
}

/**********************************************************************************************************************/

/**
 *  Test the _readQueue access in read()/readNonBlocking()
 *   * \anchor testTransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2"
 *
 *  Actually: this tests the access specifically in readTransfer()/readTransferNonBlocking(). The spec should be changed
 *  accordingly...
 */
BOOST_AUTO_TEST_CASE(B_8_2) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  // check with readTransferNonBlocking()
  accessor.resetCounters();
  accessor.push();
  accessor.preRead(TransferType::readNonBlocking);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);
  accessor.readTransferNonBlocking();
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 0);
  accessor.postRead(TransferType::readNonBlocking, true);

  // check with readTransfer()
  accessor.resetCounters();
  accessor.push();
  accessor.preRead(TransferType::read);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);
  accessor.readTransfer();
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 0);
  accessor.postRead(TransferType::read, true);
}

/**********************************************************************************************************************/

/**
 *  Test the DiscadValueException in async read operations
 *   * \anchor testTransferElement_B_8_2_2 \ref transferElement_B_8_2_2 "B.8.2.2"
 */
BOOST_AUTO_TEST_CASE(B_8_2_2) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});
  bool ret;

  // check with readNonBlocking()
  accessor.resetCounters();
  accessor.putDiscardValueOnQueue();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(ret == false);                    // B.8.2.2
  BOOST_CHECK(accessor._postRead_counter == 1); // B.8.2.2

  // check with blocking read()
  accessor.resetCounters();
  accessor.putDiscardValueOnQueue();
  std::atomic<bool> readFinished{false};
  std::thread t([&] { // laungh read() in another thread, since it will block
    accessor.read();
    readFinished = true;
  });
  usleep(1000000);                    // 1 second
  BOOST_CHECK(readFinished == false); // B.8.2.2
  accessor.push();
  t.join();
}

/**********************************************************************************************************************/

/**
 *  Test exceptions on _readQueue in readTransfer()/readTransferNonBlocking()
 *   * \anchor testTransferElement_B_8_3 \ref transferElement_B_8_3 "B.8.3"
 */
BOOST_AUTO_TEST_CASE(B_8_3) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  // use a special exception to make sure it works independent of the type of exception
  class MyException {};

  // check with readTransferNonBlocking()
  accessor.resetCounters();
  try {
    throw MyException();
  }
  catch(...) {
    accessor._readQueue.push_overwrite_exception(std::current_exception());
  }
  accessor.preRead(TransferType::readNonBlocking);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);
  BOOST_CHECK_THROW(accessor.readTransferNonBlocking(), MyException);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 0);
  accessor.postRead(TransferType::readNonBlocking, true); // small cheat: discard the exception

  // check with readTransfer()
  accessor.resetCounters();
  try {
    throw MyException();
  }
  catch(...) {
    accessor._readQueue.push_overwrite_exception(std::current_exception());
  }
  accessor.preRead(TransferType::read);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 1);
  BOOST_CHECK_THROW(accessor.readTransferNonBlocking(), MyException);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 0);
  accessor.postRead(TransferType::read, true); // small cheat: discard the exception
}

/**********************************************************************************************************************/

/**
 *  Test interrupt() places boost::thread_interrupted exception onto _readQueue.
 *  * \anchor testTransferElement_B_8_6_1 \ref transferElement_B_8_6_1 "B.8.6.1"
 */
BOOST_AUTO_TEST_CASE(B_8_6_1) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});
  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor._readQueue.pop(), boost::thread_interrupted);
}

/**********************************************************************************************************************/

/**
 *  Test interrupt() interrupts any read operation
 *  * \anchor testTransferElement_B_8_6_2 \ref transferElement_B_8_6_2 "B.8.6.2"
 */
BOOST_AUTO_TEST_CASE(B_8_6_2) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted);

  accessor.resetCounters();
  accessor.push();
  accessor.push();
  accessor.interrupt();
  BOOST_CHECK_NO_THROW(accessor.read());
  BOOST_CHECK_NO_THROW(accessor.read());
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);

  accessor.resetCounters();
  accessor.push();
  accessor.push();
  accessor.interrupt();
  BOOST_CHECK_NO_THROW(accessor.readNonBlocking());
  BOOST_CHECK_NO_THROW(accessor.readNonBlocking());
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted);

  accessor.resetCounters();
  accessor.push();
  accessor.push();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.readLatest(), boost::thread_interrupted);
  BOOST_CHECK_EQUAL(accessor._readQueue.read_available(), 0);
}

/**********************************************************************************************************************/

/**
 *  Test pre/post paring for interrupt()
 *  * \anchor testTransferElement_B_8_6_3 \ref transferElement_B_8_6_3 "B.8.6.3",
 */
BOOST_AUTO_TEST_CASE(test_B_8_6_3) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted); // (no test intended, just catch)
  BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted); // (no test intended, just catch)
  BOOST_CHECK_EQUAL(accessor._preRead_counter, 1);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);
}

/**********************************************************************************************************************/

/**
 *  Test normal functioning after interrupt()
 *  * \anchor testTransferElement_B_8_6_4 \ref transferElement_B_8_6_4 "B.8.6.4",
 */
BOOST_AUTO_TEST_CASE(test_B_8_6_4) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted); // (no test intended, just catch)

  accessor.resetCounters();
  accessor.push();
  BOOST_CHECK_NO_THROW(accessor.read());

  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted); // (no test intended, just catch)

  accessor.resetCounters();
  accessor.push();
  BOOST_CHECK_NO_THROW(accessor.readNonBlocking());
}

/**********************************************************************************************************************/

/**
 *  Test getVersionNumber()
 *  * \anchor testTransferElement_B_11_3 \ref transferElement_B_11_3 "B.11.3"
 */
BOOST_AUTO_TEST_CASE(test_B_11_3) {
  TransferElementTestAccessor<int32_t> accessor({});
  TransferElementTestAccessor<int32_t> asyncAccessor({AccessMode::wait_for_new_data});
  VersionNumber v{nullptr};

  accessor.resetCounters();
  v = {};
  BOOST_CHECK(accessor.getVersionNumber() != v);
  accessor.write(v);
  BOOST_CHECK(accessor.getVersionNumber() == v);

  accessor.resetCounters();
  v = {};
  accessor._setPostReadVersion = v;
  accessor.read();
  BOOST_CHECK(accessor.getVersionNumber() == v);

  // Test also with async accessor. As of now this is not a different code path, but this test might help to remember
  // to modify this test if in future the implementation-specific data transport queue is changed into a fixed
  // typed _readQueue in the base classes (which then will also transport the VersionNumber).
  asyncAccessor.resetCounters();
  v = {};
  asyncAccessor.push();
  asyncAccessor._setPostReadVersion = v;
  asyncAccessor.read();
  BOOST_CHECK(asyncAccessor.getVersionNumber() == v);
}

/**********************************************************************************************************************/

/**
 *  Test write with older version number throws
 *  * \anchor testTransferElement_B_11_4_1 \ref transferElement_B_11_4_1 "B.11.4.1"
 */
BOOST_AUTO_TEST_CASE(B_11_4_1) {
  TransferElementTestAccessor<int32_t> accessor({});

  VersionNumber v1;
  VersionNumber v2;
  accessor.resetCounters();
  accessor.write(v2);
  accessor.resetCounters();
  BOOST_CHECK_THROW(accessor.write(v1), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 *  Test VersionNumber argument of doPreWrite and doWriteTransfer
 *  * \anchor testTransferElement_B_11_4_2 \ref transferElement_B_11_4_2 "B.11.4.2"
 */
BOOST_AUTO_TEST_CASE(B_11_4_2) {
  TransferElementTestAccessor<int32_t> accessor({});
  VersionNumber v{nullptr};

  v = {};
  accessor.resetCounters();
  accessor.write(v);
  BOOST_CHECK(accessor._preWrite_version == v);
  BOOST_CHECK(accessor._writeTransfer_version == v);
  BOOST_CHECK(accessor._postWrite_version == v);

  v = {};
  accessor.resetCounters();
  accessor.writeDestructively(v);
  BOOST_CHECK(accessor._preWrite_version == v);
  BOOST_CHECK(accessor._writeTransfer_version == v);
  BOOST_CHECK(accessor._postWrite_version == v);
}

/**********************************************************************************************************************/

/**
 *  Test getVersionNumber() does not change under exception in a write opreration (for reads this must be checked in
 *  the UnifiedBackendTest).
 *  * \anchor testTransferElement_B_11_5 \ref transferElement_B_11_5 "B.11.5"
 */
BOOST_AUTO_TEST_CASE(test_B_11_5) {
  TransferElementTestAccessor<int32_t> accessor({});
  VersionNumber v{nullptr};

  VersionNumber v1;
  accessor.resetCounters();
  accessor.write(v1);

  // test with logic error
  accessor.resetCounters();
  v = {};
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(v), ChimeraTK::logic_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in preWrite
  accessor.resetCounters();
  v = {};
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(v), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in doWriteTransfer
  accessor.resetCounters();
  v = {};
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(v), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in doWriteTransferDestructively
  accessor.resetCounters();
  v = {};
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(v), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);
}

/**********************************************************************************************************************/

/**
 *  Test value after construction for the version number
 *  * \anchor testTransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6"
 *
 *  Notes:
 *  * B.11.6 might be screwed up by implementations and hence needs to be tested in the UnifiedBackendTest as well.
 */
BOOST_AUTO_TEST_CASE(test_B_11_6) {
  TransferElementTestAccessor<int32_t> accessor({});
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber{nullptr});
}

/**********************************************************************************************************************/

/**
 *  Test getAccessModeFlags()
 *  * \anchor testTransferElement_B_15_2 \ref transferElement_B_15_2 "B.15.2"
 */
BOOST_AUTO_TEST_CASE(test_B_15_2) {
  TransferElementTestAccessor<int32_t> accessor({});

  accessor._accessModeFlags = AccessModeFlags({});
  BOOST_CHECK(accessor.getAccessModeFlags() == AccessModeFlags({}));

  accessor._accessModeFlags = AccessModeFlags({AccessMode::wait_for_new_data});
  BOOST_CHECK(accessor.getAccessModeFlags() == AccessModeFlags({AccessMode::wait_for_new_data}));

  accessor._accessModeFlags = AccessModeFlags({AccessMode::raw});
  BOOST_CHECK(accessor.getAccessModeFlags() == AccessModeFlags({AccessMode::raw}));

  accessor._accessModeFlags = AccessModeFlags({AccessMode::wait_for_new_data, AccessMode::raw});
  BOOST_CHECK(accessor.getAccessModeFlags() == AccessModeFlags({AccessMode::wait_for_new_data, AccessMode::raw}));
}

/**********************************************************************************************************************/

/**
 *  Test access mode flags via constructor
 *  * \anchor testTransferElement_B_15_3 \ref transferElement_B_15_3 "B.15.3"
 */
BOOST_AUTO_TEST_CASE(test_B_15_3) {
  {
    TransferElementTestAccessor<int32_t> accessor({});
    BOOST_CHECK(accessor._accessModeFlags == AccessModeFlags({}));
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});
    BOOST_CHECK(accessor._accessModeFlags == AccessModeFlags({AccessMode::wait_for_new_data}));
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::raw});
    BOOST_CHECK(accessor._accessModeFlags == AccessModeFlags({AccessMode::raw}));
  }
  {
    TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data, AccessMode::raw});
    BOOST_CHECK(accessor._accessModeFlags == AccessModeFlags({AccessMode::wait_for_new_data, AccessMode::raw}));
  }
}

/**********************************************************************************************************************/

/**
 *  Test TransferElement has no default constructor
 *  * \anchor testTransferElement_B_15_3_1 \ref transferElement_B_15_3_1 "B.15.3.1"
 */
BOOST_AUTO_TEST_CASE(test_B_15_3_1) {
  BOOST_CHECK(std::is_default_constructible<TransferElement>::value == false);
}

/**********************************************************************************************************************/

/**
 *  Test _activeException
 *  * \anchor testTransferElement_B_16_1 \ref transferElement_B_16_1 "B.16.1"
 */
BOOST_AUTO_TEST_CASE(test_B_16_1) {
  TransferElementTestAccessor<int32_t> accessor({});
  TransferElementTestAccessor<int32_t> asyncAccessor({AccessMode::wait_for_new_data});

  // test logic_error in preRead
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in preRead
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in readTransfer (sync)
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in readTransferNonBlocking (sync)
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in readTransfer (async)
  asyncAccessor.resetCounters();
  asyncAccessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(asyncAccessor.read(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(asyncAccessor._seenActiveException == asyncAccessor._thrownException);

  // test runtime_error in readTransferNonBlocking (async)
  asyncAccessor.resetCounters();
  asyncAccessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(asyncAccessor.readNonBlocking(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(asyncAccessor._seenActiveException == asyncAccessor._thrownException);

  // no test for bad_numeric_cast in postRead: not applicable, exception is not caught and rethrown, but directly thrown

  // test logic_error in preWrite
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in preWrite
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in writeTransfer
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);

  // test runtime_error in writeTransferDestructively
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor._seenActiveException == accessor._thrownException);
}

/**********************************************************************************************************************/

/**
 *  Test exception rethrowing in postXxx()
 *  * \anchor testTransferElement_B_16_2 \ref transferElement_B_16_2 "B.16.2"
 */
BOOST_AUTO_TEST_CASE(test_B_16_2) {
  TransferElementTestAccessor<int32_t> accessor({});

  // Using a special exception class excludes any restriction to a specific exception type.
  class MySpecialException {};

  // test postRead
  accessor.preRead(TransferType::read);
  accessor.readTransfer();

  try {
    throw MySpecialException();
  }
  catch(...) {
    auto myException = std::current_exception();
    auto myException_copy = myException;
    accessor.setActiveException(myException);
  }

  BOOST_CHECK_EQUAL(accessor._postRead_counter, 0);
  BOOST_CHECK_THROW(accessor.postRead(TransferType::read, false), MySpecialException);
  BOOST_CHECK_EQUAL(accessor._postRead_counter, 1);

  // test postWrite
  VersionNumber v;
  accessor.preWrite(TransferType::write, v);
  accessor.writeTransfer(v);

  try {
    throw MySpecialException();
  }
  catch(...) {
    auto myException = std::current_exception();
    auto myException_copy = myException;
    accessor.setActiveException(myException);
  }

  BOOST_CHECK_EQUAL(accessor._postWrite_counter, 0);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v), MySpecialException);
  BOOST_CHECK_EQUAL(accessor._postWrite_counter, 1);
}

/**********************************************************************************************************************/

/**
 *  Test setActiveException()
 *  * \anchor testTransferElement_B_16_3 \ref transferElement_B_16_3 "B.16.3" (only setActiveException())
 */
BOOST_AUTO_TEST_CASE(test_B_16_3) {
  TransferElementTestAccessor<int32_t> accessor({});

  // Using a special exception class excludes any restriction to a specific exception type.
  class MySpecialException {};

  try {
    throw MySpecialException();
  }
  catch(...) {
    auto myException = std::current_exception();
    auto myException_copy = myException;
    accessor.setActiveException(myException);
    BOOST_CHECK(myException == nullptr);
    BOOST_CHECK(accessor._activeException == myException_copy);
  }
}

/**********************************************************************************************************************/
