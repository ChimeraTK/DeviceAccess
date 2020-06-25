/********************************************************************************************************************/
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
/********************************************************************************************************************/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TransferElementTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/make_shared.hpp>

#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "DeviceBackendImpl.h"

using namespace ChimeraTK;

/********************************************************************************************************************/

/** Special accessor used to test the behaviour of the TransferElement base class */
template<typename UserType>
class TransferElementTestAccessor : public NDRegisterAccessor<UserType> {
 public:
  TransferElementTestAccessor(AccessModeFlags flags) : NDRegisterAccessor<UserType>("someName", flags) {
    // this accessor uses a queue length of 3
    this->_readQueue = {3};
  }

  ~TransferElementTestAccessor() override {}

  void doPreRead(TransferType type) override {
    _transferType_pre = type;
    ++_preRead_counter;
    _preIndex = _currentIndex;
    ++_currentIndex;
    if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
    if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
  }

  void doPreWrite(TransferType type, VersionNumber versionNumber) override {
    _transferType_pre = type;
    ++_preWrite_counter;
    _preIndex = _currentIndex;
    ++_currentIndex;
    _preWrite_version = versionNumber;
    if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
    if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
    if(_throwNumericCast) throw boost::numeric::bad_numeric_cast();
  }

  void doReadTransferSynchronously() override {
    ++_readTransfer_counter;
    _transferIndex = _currentIndex;
    ++_currentIndex;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
  }

  bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
    ++_writeTransfer_counter;
    _transferIndex = _currentIndex;
    ++_currentIndex;
    _writeTransfer_version = versionNumber;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    return _previousDataLost;
  }

  bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
    ++_writeTransferDestructively_counter;
    _transferIndex = _currentIndex;
    ++_currentIndex;
    _writeTransfer_version = versionNumber;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    return _previousDataLost;
  }

  void doPostRead(TransferType type, bool updateDataBuffer) override {
    _transferType_post = type;
    ++_postRead_counter;
    _postIndex = _currentIndex;
    ++_currentIndex;
    _updateDataBuffer = updateDataBuffer;
    if(_throwNumericCast) throw boost::numeric::bad_numeric_cast();
  }

  void doPostWrite(TransferType type, VersionNumber versionNumber) override {
    _transferType_post = type;
    ++_postWrite_counter;
    _postIndex = _currentIndex;
    ++_currentIndex;
    _postWrite_version = versionNumber;
  }

  bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override { return false; }
  std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }
  std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }
  void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override { (void)newElement; }

  bool isReadOnly() const override { return !_writeable && _readable; }

  bool isReadable() const override { return _readable; }

  bool isWriteable() const override { return _writeable; }

  void interrupt() override { this->interrupt_impl(this->_readQueue); }

  bool _writeable{true};
  bool _readable{true};

  TransferType _transferType_pre, _transferType_post; // TransferType as seen in pre/postXxx()
  bool _updateDataBuffer;                             // updateDataBuffer as seen in postRead() (set there)
  VersionNumber _preWrite_version{nullptr};
  VersionNumber _writeTransfer_version{nullptr};
  VersionNumber _postWrite_version{nullptr};

  size_t _preRead_counter{0};
  size_t _preWrite_counter{0};
  size_t _readTransfer_counter{0};
  size_t _writeTransfer_counter{0};
  size_t _writeTransferDestructively_counter{0};
  size_t _postRead_counter{0};
  size_t _postWrite_counter{0};
  size_t _preIndex{0};
  size_t _transferIndex{0};
  size_t _postIndex{0};
  size_t _currentIndex{0};
  void resetCounters() {
    _preRead_counter = 0;
    _preWrite_counter = 0;
    _readTransfer_counter = 0;
    _writeTransfer_counter = 0;
    _writeTransferDestructively_counter = 0;
    _postRead_counter = 0;
    _postWrite_counter = 0;
    _currentIndex = 0;
    _throwLogicErr = false;
    _throwRuntimeErrInPre = false;
    _throwRuntimeErrInTransfer = false;
    _throwNumericCast = false;
  }

  bool _previousDataLost{false}; // flag to return by writeTransfer()/writeTransferDestructively()
  bool _throwLogicErr{false};    // always in doPreXxx()
  bool _throwRuntimeErrInTransfer{false};
  bool _throwRuntimeErrInPre{false};
  bool _throwNumericCast{false}; // in doPreWrite() or doPreRead() depending on operation

  // convenience function to put exceptions onto the readQueue (see also interrupt())
  void putRuntimeErrorOnQueue() {
    try {
      throw ChimeraTK::runtime_error("Test");
    }
    catch(...) {
      this->_readQueue.push_exception(std::current_exception());
    }
  }
  void putDiscardValueOnQueue() {
    try {
      throw ChimeraTK::detail::DiscardValueException();
    }
    catch(...) {
      this->_readQueue.push_exception(std::current_exception());
    }
  }
  // simulate a reveiver thread by manually putting data into the queue
  bool push() { return this->_readQueue.push(); }

  using TransferElement::_activeException;
};

/********************************************************************************************************************/

/**
 *  Helper macro to test more easily the correct sequence of stages in a transfer (i.e. B.4 except 4.3.2, and B.5
 *  without sub-points).
 * 
 *  BE CARFUL WHEN MODIFYING THIS CODE. It is used in many places, which expect the code to test exactly what it tests
 *  now. DO NOT change this function, if on some use cases it looks like the checks need to be different, without
 *  verifying that this is correct FOR ALL use cases.
 * 
 *  The sole purpose of this helper is to check wheter the stages doPreXxx, doXxxTransferYyy and doPostXxx are called
 *  in the right order and only when needed. It also checks whether the transfer type argument passed to those calls
 *  had the right value. Any other check (including checking the other arguments) is outside the scope of this helper.
 * 
 *  TODO: Convert into macro, so the line numbers in case of failures are more helpful...
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

/********************************************************************************************************************/

/**
 *  Test correctness of the pre-transfer-post sequence and the return values of the operations, for synchronous
 *  operations without exceptions.
 * 
 *  This tests the TransferElement specification:
 *  * \anchor testTransferElement_B_4_sync \ref transferElement_B_4 "B.4" (except B.4.2.4, B.4.3.1 and B.4.3.2),
 *  * \anchor testTransferElement_B_5_sync \ref transferElement_B_5 "B.5" (without sub-points) and
 *  * \anchor testTransferElement_B_7_sync \ref transferElement_B_7 "B.7" (without B.7.4)
 * 
 *  for accessors without AccessMode::wait_for_new_data.
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeNoExceptions) {
  TransferElementTestAccessor<int32_t> accessor({});
  bool ret;

  accessor._readable = true;
  accessor._writeable = false;

  accessor.resetCounters();
  accessor.read();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, true); // B.4.1, B.4.2.1, B.4.3
  BOOST_CHECK(accessor._updateDataBuffer == true);            // B.7.3 (second sentence)

  accessor.resetCounters();
  ret = accessor.readNonBlocking();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, true); // B.4.1, B.4.2.2, B.4.3
  BOOST_CHECK(ret == true);                                              // B.7.1, B.4.2.2 (second sub-point)
  BOOST_CHECK(accessor._updateDataBuffer == true);                       // B.7.3 (+B.4.2.2 second sub-point)

  accessor._readable = false;
  accessor._writeable = true;

  accessor.resetCounters();
  accessor._previousDataLost = false;
  ret = accessor.write();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true); // B.4.1, B.4.2.3, B.4.3
  BOOST_CHECK(ret == false);                                   // B.7.2

  accessor.resetCounters();
  accessor._previousDataLost = true;
  ret = accessor.write();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true); // (redundant) B.4.1, B.4.2.3, B.4.3
  BOOST_CHECK(ret == true);                                    // B.7.2

  accessor.resetCounters();
  accessor._previousDataLost = false;
  ret = accessor.writeDestructively();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, true); // B.4.1, B.4.2.3, B.4.3
  BOOST_CHECK(ret == false);                                                // B.7.2

  accessor.resetCounters();
  accessor._previousDataLost = true;
  ret = accessor.writeDestructively();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, true); // (redundant) B.4.1, B.4.2.3, B.4.3
  BOOST_CHECK(ret == true);                                                 // B.7.2
}

/********************************************************************************************************************/

/**
 *  Test correcness of the pre-transfer-post sequence for synchronous operations which throw an exception.
 * 
 *  This tests the TransferElement specification:
 *  * \anchor testTransferElement_B_5_1_sync \ref transferElement_B_5_1 "B.5.1",
 *  * \anchor testTransferElement_B_6_sync \ref transferElement_B_6 "B.6" (without sub-point),
 *  * \anchor testTransferElement_B_6_1_sync \ref transferElement_B_6_1 "B.6.1", and
 *  * \anchor testTransferElement_B_7_4_sync \ref transferElement_B_7_4 "B.7.4"
 * 
 *  for accessors without AccessMode::wait_for_new_data.
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeWithExceptions) {
  TransferElementTestAccessor<int32_t> accessor({});

  accessor._readable = true;
  accessor._writeable = false;

  // read()
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error);  // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, false); // B.5.1, B.6 (pairing), B.6.1
  BOOST_CHECK(accessor._updateDataBuffer == false);            // B.7.4

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, false);  // B.5.1, B.6 (pairing), B.6.1
  BOOST_CHECK(accessor._updateDataBuffer == false);             // B.7.4

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, true);   // B.5.1, B.6 (pairing)
  BOOST_CHECK(accessor._updateDataBuffer == false);             // B.7.4

  // readNonBlocking()
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error);  // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false); // B.5.1, B.6 (pairing)
  BOOST_CHECK(accessor._updateDataBuffer == false);                       // B.7.4

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false);  // B.5.1, B.6 (pairing)
  BOOST_CHECK(accessor._updateDataBuffer == false);                        // B.7.4

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, true);   // B.5.1, B.6 (pairing)
  BOOST_CHECK(accessor._updateDataBuffer == false);                        // B.7.4

  // write()
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error);  // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, false); // B.5.1, B.6 (pairing)

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, false);  // B.5.1, B.6 (pairing)

  accessor.resetCounters();
  accessor._throwNumericCast = true;
  BOOST_CHECK_THROW(accessor.write(), boost::numeric::bad_numeric_cast); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, false);          // B.5.1, B.6 (pairing)

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true);   // B.5.1, B.6 (pairing)

  // writeDestructively()
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error);  // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, false); // B.5.1, B.6 (pairing)

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, false);  // B.5.1, B.6 (pairing)

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually)
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::writeDestructively, true);   // B.5.1, B.6 (pairing)
}

/********************************************************************************************************************/

/**
 *  Test correcness of the pre-transfer-post sequence for asynchronous read operations without exceptions.
 * 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_4_async \ref transferElement_B_4 "B.4" (except B.4.2.4, B.4.3.1 and B.4.3.2),
 *  * \anchor testTransferElement_B_5_async \ref transferElement_B_5 "B.5" (without sub-points),
 *  * \anchor testTransferElement_B_7_async \ref transferElement_B_7 "B.7" (without B.7.4) and
 *  * \anchor testTransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2" (without sub-points)
 * 
 *  for accessors with AccessMode::wait_for_new_data and read operations (write operations are not affected by that
 *  flag).
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_AsyncModeNoExceptions) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});
  bool ret;

  accessor._readable = true;
  accessor._writeable = false;

  accessor.resetCounters();
  accessor.push();
  accessor.read();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, false); // B.4.1, B.4.2.1, B.4.3, B.8.2
  BOOST_CHECK(accessor._updateDataBuffer == true);             // B.7.3

  accessor.resetCounters();
  std::atomic<bool> readCompleted{false};
  std::thread t([&] {
    accessor.read();
    readCompleted = true;
  });
  usleep(10000);
  BOOST_CHECK(!readCompleted); // B.4.2.1
  accessor.push();
  t.join();

  accessor.resetCounters();
  accessor.push();
  ret = accessor.readNonBlocking();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false); // B.4.1, B.4.2.1, B.4.3, B.8.2
  BOOST_CHECK(ret == true);                                               // B.7.1
  BOOST_CHECK(accessor._updateDataBuffer == true);                        // B.7.3

  accessor.resetCounters();
  ret = accessor.readNonBlocking();
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false); // B.4.1, B.4.2.1, B.4.3, B.8.2
  BOOST_CHECK(ret == false);                                              // B.7.1
  BOOST_CHECK(accessor._updateDataBuffer == false);                       // B.7.3
}

/********************************************************************************************************************/

/**
 *  Test correcness of the pre-transfer-post sequence for asynchronous read operations which throw an exception.
 * 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_5_1_async \ref transferElement_B_5_1 "B.5.1",
 *  * \anchor testTransferElement_B_6_async \ref transferElement_B_6 "B.6" (without sub-point),
 *  * \anchor testTransferElement_B_7_4_async \ref transferElement_B_7_4 "B.7.4" and
 *  * \anchor testTransferElement_B_8_3 \ref transferElement_B_8_3 "B.8.3" (only second sentence)
 *
 *  for accessors with AccessMode::wait_for_new_data and read operations (write operations are not affected by that 
 *  flag).
 * 
 *  Note: since there is no difference between sync and async mode for logic_errors, only runtime_errors are tested
 *  here.
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_AsyncModeWithExceptions) {
  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  accessor._readable = true;
  accessor._writeable = false;

  accessor.resetCounters();
  accessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually), B.8.3
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, false);  // B.5.1
  BOOST_CHECK(accessor._updateDataBuffer == false);             // B.7.4

  accessor.resetCounters();
  accessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error); // B.6 (implies it throws eventually), B.8.3
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::readNonBlocking, false);  // B.5.1
  BOOST_CHECK(accessor._updateDataBuffer == false);                        // B.7.4
}

/********************************************************************************************************************/

/**
 *  Test that duplicate calls to preXxx/postXxx are ignored.
 * 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_5_2 \ref transferElement_B_5_2 "B.5.2".
 */
BOOST_AUTO_TEST_CASE(testPrePostPairingDuplicateCalls) {
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
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::read, true); // B.5.2

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
  TEST_TRANSFER_SEQUENCE(accessor, TransferType::write, true); // B.5.2

  // no need to test all read and write types, since the mechanism does not depend on the type.
}

/********************************************************************************************************************/

/**
 *  Test that the current VersionNumber is updated in postWrite, unless there has been an exception.
 *  
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_4_3_2 \ref transferElement_B_4_3_2 "B.4.3.2",
 *  * \anchor testTransferElement_B_11_3 \ref transferElement_B_11_3 "B.11.3" and
 *  * \anchor testTransferElement_B_11_5 \ref transferElement_B_11_5 "B.11.5".
 */
BOOST_AUTO_TEST_CASE(testPostWriteVersionNumberUpdate) {
  TransferElementTestAccessor<int32_t> accessor({});

  // test without exception
  accessor.resetCounters();
  VersionNumber v1{};
  accessor.preWrite(TransferType::write, v1);
  accessor.writeTransfer(v1);
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr)); // B.4.3.2 (states it's done in postWrite)
  accessor.postWrite(TransferType::write, v1);
  BOOST_CHECK(accessor.getVersionNumber() == v1); // B.4.3.2, B.11.3

  // test with logic error
  accessor.resetCounters();
  VersionNumber v2{};
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(v2), ChimeraTK::logic_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);                // B.11.5

  // test with runtime error in preWrite
  accessor.resetCounters();
  VersionNumber v3{};
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(v3), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);                  // B.11.5

  // test with runtime error in doWriteTransfer
  accessor.resetCounters();
  VersionNumber v4{};
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(v4), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);                  // B.11.5

  // test with runtime error in doWriteTransferDestructively
  accessor.resetCounters();
  VersionNumber v5{};
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(v5), ChimeraTK::runtime_error); // (no test intended, just catch)
  BOOST_CHECK(accessor.getVersionNumber() == v1);                               // B.11.5
}

/********************************************************************************************************************/

/**
 *  Test the mechanism which allows decorators to delegate exceptions to their targets.
 *  
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_6_2 \ref transferElement_B_6_2 "B.6.2"
 *  * \anchor testTransferElement_C_2_1 \ref transferElement_C_2_1 "C.2.1"
 *  * \anchor testTransferElement_C_2_2 \ref transferElement_C_2_2 "C.2.2"
 *  * \anchor testTransferElement_C_2_3 \ref transferElement_C_2_3 "C.2.3" (only implementation of setActiveException()
 *    and rethrowing it in postXxx())
 */
BOOST_AUTO_TEST_CASE(testDelegateExceptionsInDecorators) {
  TransferElementTestAccessor<int32_t> accessor({});

  // Check B.6.2 -> catching exceptions happens in xxxYyy(), not in preXxx()/xxxTransferYyy()
  // ========================================================================================
  //
  // Simply check that preXxx()/xxxTransferYyy() are throwing. Since previous tests have shown that the stages are
  // correctly called even with exceptions, we can then conclude that the code calling preXxx()/xxxTransferYyy() is
  // catching the exception to delay it.

  // Check C.2.3 -> setActiveException()
  // ===================================

  // Note: both these tests are done below together. First B.6.2 is tested for xxxTransfer() together with C.2.3, then
  // B.6.2 for preXxx() is tested alone.

  // The test acts like a decorator, the "accessor" is its target.
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true; // target shall throw in the transfer

  // this is like doPreRead of the decorator
  accessor.preRead(TransferType::read);

  // this is like doReadTransferSynchronously of the decorator, including the exception handling normally implemented
  // in the TransferElement base class
  std::exception_ptr myException{nullptr};
  try {
    accessor.readTransfer();
    BOOST_ERROR("readTransfer() must throw a ChimeraTK::runtime_error"); // B.6.2
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostRead of the decorator. According to C.2.3 it has to delegate the exception to the target by
  // calling setActiveException(), and the target's TransferElement base class is then responsible for throwing it after
  // the calling target's doPostRead().
  auto myException_copy = myException;
  accessor.setActiveException(myException);
  BOOST_CHECK(accessor._activeException == myException_copy);                                // C.2.1
  BOOST_CHECK_THROW(accessor.postRead(TransferType::read, false), ChimeraTK::runtime_error); // C.2.3
  BOOST_CHECK(accessor._postRead_counter == 1);                                              // C.2.2

  // same test again, this time with write (we are testing code in postXxx).
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;

  // this is like doPreWrite of the decorator
  VersionNumber v{};

  accessor.preWrite(TransferType::write, v);
  // this is like doWriteTransfer of the decorator
  try {
    accessor.writeTransfer(v);
    BOOST_ERROR("writeTransfer() must throw a ChimeraTK::runtime_error"); // B.6.2
  }
  catch(ChimeraTK::runtime_error&) {
    myException = std::current_exception();
  }

  // this is like doPostWrite of the decorator
  accessor.setActiveException(myException);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v), ChimeraTK::runtime_error); // C.2.3
  BOOST_CHECK(accessor._postWrite_counter == 1);                                           // C.2.2

  // Now check that preRead throws directly (B.6.2)
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.preRead(TransferType::read), ChimeraTK::runtime_error); // B.6.2
  accessor.postRead(TransferType::read, false); // just complete the sequence as required by the spec

  // Now check that preWrite throws directly (B.6.2)
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.preWrite(TransferType::write, {}), ChimeraTK::runtime_error); // B.6.2
  accessor.postWrite(TransferType::write, {}); // just complete the sequence as required by the spec
}

/********************************************************************************************************************/

/**
 *  Test the convenience function readLatest().
 * 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_3_1_4 \ref transferElement_B_3_1_4 "B.3.1.4"
 */
BOOST_AUTO_TEST_CASE(testReadLatest) {
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

/********************************************************************************************************************/

/**
 *  Test the DiscadValueException in async read operations
 * 
 *  This tests the TransferElement specifications:
 *   * \anchor testTransferElement_B_8_2_2 \ref transferElement_B_8_2_2 "B.8.2.2"
 */
BOOST_AUTO_TEST_CASE(testDiscardValueException) {
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
  usleep(1000000); // 1 second
  BOOST_CHECK(readFinished == false); // B.8.2.2
  accessor.push();
  t.join();
}

/********************************************************************************************************************/

/**
 *  Test handling of VesionNumbers in write operations
 * 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_11_4_1 \ref transferElement_B_11_4_1 "B.11.4.1",
 *  * \anchor testTransferElement_B_11_4_2 \ref transferElement_B_11_4_2 "B.11.4.2" and
 *  * \anchor testTransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6".
 *
 *  Notes:
 *  * B.11.3/B.11.5 is already tested in testPostWriteVersionNumberUpdate.
 *  * B.11.6 might be screwed up by implementations and hence needs to be tested in the UnifiedBackendTest as well.
 */
BOOST_AUTO_TEST_CASE(testVersionNumber) {
  TransferElementTestAccessor<int32_t> accessor({});

  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber{nullptr}); // B.11.6

  VersionNumber v1;
  VersionNumber v2;
  accessor.resetCounters();
  accessor.write(v2);
  BOOST_CHECK(accessor._preWrite_version == v2);      // B.11.4.2
  BOOST_CHECK(accessor._writeTransfer_version == v2); // B.11.4.2
  BOOST_CHECK(accessor._postWrite_version == v2);     // B.11.4.2
  accessor.resetCounters();
  BOOST_CHECK_THROW(accessor.write(v1), ChimeraTK::logic_error); // B.11.4.1
  BOOST_CHECK(accessor._preWrite_version == v2);                 // B.11.4.2
  BOOST_CHECK(accessor._postWrite_version == v2);                // B.11.4.2
}

/********************************************************************************************************************/

/**
 *  Test interrupt().
 * 
 *  This tests the TransferElement specifications:
 *   * \anchor testTransferElement_B_8_6 \ref transferElement_B_8_6 "B.8.6" (with all sub-points).
 */
BOOST_AUTO_TEST_CASE(testInterrupt) {
  TransferElementTestAccessor<int32_t> snycAccessor({});

  BOOST_CHECK_THROW(snycAccessor.interrupt(), ChimeraTK::logic_error); // B.8.6.5

  TransferElementTestAccessor<int32_t> accessor({AccessMode::wait_for_new_data});

  accessor.interrupt();
  accessor.resetCounters();
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted); // B.8.6.1/B.8.6.2
  BOOST_CHECK(accessor._postRead_counter == 1);                  // B.8.6.3

  // B.8.6.4 partially tested in the following (backend-specific tests required as well)
  accessor.resetCounters();
  BOOST_CHECK(accessor.readNonBlocking() == false);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor.resetCounters();
  accessor.write();
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.push();
  accessor.resetCounters();
  accessor.read();
  BOOST_CHECK(accessor._postRead_counter == 1);
}

/********************************************************************************************************************/
