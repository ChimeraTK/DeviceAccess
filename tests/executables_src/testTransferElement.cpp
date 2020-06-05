/********************************************************************************************************************/
/* Tests for the TransferElement base class                                                                         */
/********************************************************************************************************************/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TransferElementTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "NDRegisterAccessor.h"
#include "DeviceBackendImpl.h"

using namespace ChimeraTK;

/********************************************************************************************************************/

/** Special accessor used to test the behaviour of the TransferElement base class */
template<typename UserType>
class TransferElementTestAccessor : public NDRegisterAccessor<UserType> {
 public:
  TransferElementTestAccessor() : NDRegisterAccessor<UserType>("someName", {}) {}

  ~TransferElementTestAccessor() override {}

  void doPreRead(TransferType type) override {
    _transferType = type;
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    ++_preRead_counter;
    if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
    if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInPre) throw boost::thread_interrupted();
  }

  void doPreWrite(TransferType type, VersionNumber versionNumber) override {
    _transferType = type;
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    ++_preWrite_counter;
    _newVersion = versionNumber;
    if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
    if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
    if(_throwNumericCast) throw boost::numeric::bad_numeric_cast();
    if(_throwThreadInterruptedInPre) throw boost::thread_interrupted();
  }

  void doReadTransferSynchronously() override {
    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::read);
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
  }

  bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::write);
    BOOST_CHECK(versionNumber == _newVersion);
    ++_writeTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
    return _hasNewDataOrDatLost;
  }

  bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::writeDestructively);
    BOOST_CHECK(versionNumber == _newVersion);
    ++_writeTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
    return _hasNewDataOrDatLost;
  }

  /** 
   * This doPostRead() implementation checks partially the TransferElement specification B.4.
   * 
   * \anchor testTransferElement_B_6_1_read It also tests specifically the
   * \ref transferElement_B_6_1 "TransferElement specification B.6.1" for read operations.
   */
  void doPostRead(TransferType type, bool hasNewData) override {
    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre && !_throwThreadInterruptedInPre) {
      if(!_flags.has(AccessMode::wait_for_new_data)) {
        BOOST_CHECK(_readTransfer_counter == 1);
      }
      else {
        BOOST_CHECK(_readTransfer_counter == 0);
      }
    }
    else {
      /* Here B.6.1 is tested for read operations */
      BOOST_CHECK(_readTransfer_counter == 0);
    }
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postRead_counter;
    _hasNewDataOrDatLost = hasNewData;
    if(_throwNumericCast) throw boost::numeric::bad_numeric_cast();
    if(_throwThreadInterruptedInPost) throw boost::thread_interrupted();
  }

  /** 
   * This doPostWrite() implementation checks partially the TransferElement specification B.4.
   * 
   * \anchor testTransferElement_B_6_1_write It also tests specifically the
   * \ref transferElement_B_6_1 "TransferElement specification B.6.1" for write operations.
   */
  void doPostWrite(TransferType type, VersionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK(_readTransfer_counter == 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre && !_throwNumericCast && !_throwThreadInterruptedInPre) {
      BOOST_CHECK(_writeTransfer_counter == 1);
    }
    else {
      /* Here B.6.1 is tested for write operations */
      BOOST_CHECK(_writeTransfer_counter == 0);
    }
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postWrite_counter;
    if(_throwThreadInterruptedInPost) throw boost::thread_interrupted();

    // Update version number: this will be moved to the base class!
    // Remove warning in testPostWriteVersionNumberUpdate once this has been done.
    if(!_throwLogicErr && !_throwRuntimeErrInPre && !_throwNumericCast && !_throwThreadInterruptedInPre &&
        !_throwRuntimeErrInTransfer && !_throwThreadInterruptedInTransfer) {
      _currentVersion = _newVersion;
    }
  }

  void interrupt() override {
    // This functionality will be moved to the base class
    if(!_flags.has(AccessMode::wait_for_new_data)) {
      throw ChimeraTK::logic_error("TransferElement::interrupt() called but AccessMode::wait_for_new_data is not set.");
    }
    try {
      throw boost::thread_interrupted();
    }
    catch(...) {
      _readQueue.push_exception(std::current_exception());
    }
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

  VersionNumber getVersionNumber() const override { return _currentVersion; }
  VersionNumber _currentVersion{nullptr};

  cppext::future_queue<void> _readQueue{3};

  AccessModeFlags _flags{};
  bool _writeable{false};
  bool _readable{false};

  TransferType _transferType;
  bool _hasNewDataOrDatLost;
  VersionNumber _newVersion{nullptr};

  size_t _preRead_counter{0};
  size_t _preWrite_counter{0};
  size_t _readTransfer_counter{0};
  size_t _writeTransfer_counter{0};
  size_t _postRead_counter{0};
  size_t _postWrite_counter{0};
  void resetCounters() {
    _preRead_counter = 0;
    _preWrite_counter = 0;
    _readTransfer_counter = 0;
    _writeTransfer_counter = 0;
    _postRead_counter = 0;
    _postWrite_counter = 0;
    _throwLogicErr = false;
    _throwRuntimeErrInPre = false;
    _throwRuntimeErrInTransfer = false;
    _throwNumericCast = false;
    _throwThreadInterruptedInPre = false;
    _throwThreadInterruptedInTransfer = false;
    _throwThreadInterruptedInPost = false;
  }

  bool _throwLogicErr{false}; // always in doPreXxx()
  bool _throwRuntimeErrInTransfer{false};
  bool _throwRuntimeErrInPre{false};
  bool _throwNumericCast{false}; // in doPreWrite() or doPreRead() depending on operation
  bool _throwThreadInterruptedInPre{false};
  bool _throwThreadInterruptedInTransfer{false};
  bool _throwThreadInterruptedInPost{false};

  // convenience function to put exceptions onto the readQueue (see also interrupt())
  void putRuntimeErrorOnQueue() {
    try {
      throw ChimeraTK::runtime_error("Test");
    }
    catch(...) {
      _readQueue.push_exception(std::current_exception());
    }
  }
  void putDiscardValueOnQueue() {
    try {
      throw ChimeraTK::detail::DiscardValueException();
    }
    catch(...) {
      _readQueue.push_exception(std::current_exception());
    }
  }
};

#if 0 // currently not needed, maybe later?
/** Special backend used to test the behaviour of the TransferElement base class */
class TransferElementTestBackend : public DeviceBackendImpl {
 public:
  TransferElementTestBackend(std::map<std::string, std::string> parameters);
  ~TransferElementTestBackend() override {}

  void open() override;

  void close() override;

  std::string readDeviceInfo() override { return std::string("TransferElementTestBackend"); }

  static boost::shared_ptr<DeviceBackend> createInstance(
      std::string address, std::map<std::string, std::string> parameters);

  bool isFunctional() const override { return true; }

 protected:
  friend class TransferElementTestAccessor;

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);
  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(SubdeviceBackend, getRegisterAccessor_impl, 4);
};
#endif

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specification:
 *  * \anchor testTransferElement_B_4_sync \ref transferElement_B_4 "B.4" (except B.4.3.2),
 *  * \anchor testTransferElement_B_5_sync \ref transferElement_B_5 "B.5" (without sub-points) and
 *  * \anchor testTransferElement_B_7_sync \ref transferElement_B_7 "B.7" (without B.7.4)
 * 
 *  for accessors without AccessMode::wait_for_new_data
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeNoExceptions) {
  TransferElementTestAccessor<int32_t> accessor;
  bool ret;

  accessor._readable = true;
  accessor._writeable = false;

  accessor.resetCounters();
  accessor.read();
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor._readable = false;
  accessor._writeable = true;

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  ret = accessor.write();
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  ret = accessor.write();
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  ret = accessor.writeDestructively();
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  ret = accessor.writeDestructively();
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specification:
 *  * \anchor testTransferElement_B_5_1_sync \ref transferElement_B_5_1 "B.5.1" and
 *  * \anchor testTransferElement_B_7_4_sync \ref transferElement_B_7_4 "B.7.4"
 * 
 *  for accessors without AccessMode::wait_for_new_data
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeWithExceptions) {
  TransferElementTestAccessor<int32_t> accessor;

  accessor._readable = true;
  accessor._writeable = false;

  // read()
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  // readNonBlocking() with hasNewData == true
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  // readNonBlocking() with hasNewData == false
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  // write() with dataLost == false
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  // write() with dataLost == true
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);

  // writeDestructively() with dataLost == false
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  // write() with dataLost == true
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_4_async \ref transferElement_B_4 "B.4" (except B.4.3.2),
 *  * \anchor testTransferElement_B_5_async \ref transferElement_B_5 "B.5" (without sub-points),
 *  * \anchor testTransferElement_B_7_async \ref transferElement_B_7 "B.7" (without B.7.4) and
 *  * \anchor testTransferElement_B_8_2 \ref transferElement_B_8_2 "B.8.2" (without sub-points)
 * 
 *  for accessors with AccessMode::wait_for_new_data and read operations (write operations are not affected by that
 *  flag).
 */
BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_AsyncModeNoExceptions) {
  TransferElementTestAccessor<int32_t> accessor;
  bool ret;

  accessor._readable = true;
  accessor._writeable = false;
  accessor._flags = {AccessMode::wait_for_new_data};

  accessor.resetCounters();
  accessor._readQueue.push();
  accessor.read();
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  accessor._readQueue.push();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);

  accessor.resetCounters();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_5_1_async \ref transferElement_B_5_1 "B.5.1",
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
  TransferElementTestAccessor<int32_t> accessor;

  accessor._readable = true;
  accessor._writeable = false;
  accessor._flags = {AccessMode::wait_for_new_data};

  accessor.resetCounters();
  accessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  accessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_5_2 \ref transferElement_B_5_2 "B.5.2".
 */
BOOST_AUTO_TEST_CASE(testPrePostPairingDuplicateCalls) {
  TransferElementTestAccessor<int32_t> accessor;

  // read()
  accessor.resetCounters();
  accessor.preRead(TransferType::read);
  accessor.preRead(TransferType::read);
  accessor.preRead(TransferType::read);
  accessor.readTransfer();
  accessor.postRead(TransferType::read, true);
  accessor.postRead(TransferType::read, true);
  accessor.postRead(TransferType::read, true);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead

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
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite

  // no need to test all read and write types, since the mechanism does not depend on the type.
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_4_3_2 \ref transferElement_B_4_3_2 "B.4.3.2",
 *  * \anchor testTransferElement_B_11_3 \ref transferElement_B_11_3 "B.11.3" and
 *  * \anchor testTransferElement_B_11_5 \ref transferElement_B_11_5 "B.11.5".
 */
BOOST_AUTO_TEST_CASE(testPostWriteVersionNumberUpdate) {
  std::cout << "WARNING: This test (testPostWriteVersionNumberUpdate) is currently testing functionality which is part "
               "of the test implementation, not the base class!"
            << std::endl;
  TransferElementTestAccessor<int32_t> accessor;

  // test initial value
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr));

  // test without exception
  accessor.resetCounters();
  VersionNumber v1{};
  accessor.preWrite(TransferType::write, v1);
  accessor.writeTransfer(v1);
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr));
  accessor.postWrite(TransferType::write, v1);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with logic error
  accessor.resetCounters();
  VersionNumber v2{};
  accessor._throwLogicErr = true;
  accessor.preWrite(TransferType::write, v2);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v2), ChimeraTK::logic_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in preWrite
  accessor.resetCounters();
  VersionNumber v3{};
  accessor._throwRuntimeErrInPre = true;
  accessor.preWrite(TransferType::write, v3);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v3), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in doWriteTransfer
  accessor.resetCounters();
  VersionNumber v4{};
  accessor._throwRuntimeErrInTransfer = true;
  accessor.preWrite(TransferType::write, v4);
  accessor.writeTransfer(v4);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v4), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in doWriteTransferDestructively
  accessor.resetCounters();
  VersionNumber v5{};
  accessor._throwRuntimeErrInTransfer = true;
  accessor.preWrite(TransferType::writeDestructively, v5);
  accessor.writeTransferDestructively(v5);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::writeDestructively, v5), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_6 \ref transferElement_B_6 "B.6" (sub-point implicitly included)
 */
BOOST_AUTO_TEST_CASE(testExceptionDelaying) {
  TransferElementTestAccessor<int32_t> accessor;

  // test exceptions in preRead
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor.resetCounters();
  accessor._throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postRead_counter == 1);

  // test exceptions in preWrite
  accessor.resetCounters();
  accessor._throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error);
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.resetCounters();
  accessor._throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.resetCounters();
  accessor._throwNumericCast = true;
  BOOST_CHECK_THROW(accessor.write(), boost::numeric::bad_numeric_cast);
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.resetCounters();
  accessor._throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.write(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postWrite_counter == 1);

  // test exceptions in read transfer (without wait_for_new_data)
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor.resetCounters();
  accessor._throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postRead_counter == 1);

  // test exceptions in read transfer (with wait_for_new_data, exception received through queue)
  accessor.resetCounters();
  accessor._flags = {AccessMode::wait_for_new_data};
  accessor.putRuntimeErrorOnQueue();
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor.resetCounters();
  accessor.interrupt();
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postRead_counter == 1);
  accessor._flags = {};

  // test exceptions in write transfer
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.resetCounters();
  accessor._throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postWrite_counter == 1);

  // test exceptions in destructive write transfer
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1);
  accessor.resetCounters();
  accessor._throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), boost::thread_interrupted);
  BOOST_CHECK(accessor._postWrite_counter == 1);

  // check (at the example of the runtime error) that the exception is thrown by postRead() resp. postWrite() itself
  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  accessor.preRead(TransferType::read);
  accessor.readTransfer();
  BOOST_CHECK_THROW(accessor.postRead(TransferType::read, false), ChimeraTK::runtime_error);

  accessor.resetCounters();
  accessor._throwRuntimeErrInTransfer = true;
  VersionNumber v{};
  accessor.preWrite(TransferType::write, v);
  accessor.writeTransfer(v);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, v), ChimeraTK::runtime_error);
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_3_1_4 \ref transferElement_B_3_1_4 "B.3.1.4"
 */
BOOST_AUTO_TEST_CASE(testReadLatest) {
  TransferElementTestAccessor<int32_t> accessor;
  bool ret;

  // Without AccessMode::wait_for_new_data
  accessor.resetCounters();
  accessor.readLatest();
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 1 checked in doPostRead

  // With AccessMode::wait_for_new_data
  accessor._flags = {AccessMode::wait_for_new_data};
  accessor.resetCounters();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 0 checked in doPostRead

  accessor.resetCounters();
  accessor._readQueue.push();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 0 checked in doPostRead
  accessor.resetCounters();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 0 checked in doPostRead

  accessor.resetCounters();
  while(accessor._readQueue.push()) continue; // fill the queue
  ret = accessor.readLatest();
  BOOST_CHECK(ret == true);
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 0 checked in doPostRead
  accessor.resetCounters();
  ret = accessor.readLatest();
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._postRead_counter == 1); // accessor._readTransfer_counter == 0 checked in doPostRead
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_8_2_2 \ref transferElement_B_8_2_2 "B.8.2.2"
 */
BOOST_AUTO_TEST_CASE(testDiscardValueException) {
  TransferElementTestAccessor<int32_t> accessor;
  bool ret;
  accessor._flags = {AccessMode::wait_for_new_data};

  // check with readNonBlocking()
  accessor.resetCounters();
  accessor.putDiscardValueOnQueue();
  ret = accessor.readNonBlocking();
  BOOST_CHECK(ret == false);
  BOOST_CHECK(accessor._postRead_counter == 1);

  // check with blocking read()
  accessor.resetCounters();
  accessor.putDiscardValueOnQueue();
  std::atomic<bool> readFinished{false};
  boost::thread t([&] { // laungh read() in another thread, since it will block
    accessor.read();
    readFinished = true;
  });
  usleep(1000000); // 1 second
  BOOST_CHECK(readFinished == false);
  accessor._readQueue.push();
  t.join();
  BOOST_CHECK(readFinished == true); // thread was not interrupted
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_11_4_1 \ref transferElement_B_11_4_1 "B.11.4.1",
 *  * \anchor testTransferElement_B_11_4_2 \ref transferElement_B_11_4_2 "B.11.4.2" and
 *  * \anchor testTransferElement_B_11_6 \ref transferElement_B_11_6 "B.11.6" (partially).
 *
 *  Notes:
 *  * B.11.3/B.11.5 is already tested in testPostWriteVersionNumberUpdate.
 *  * B.11.6 might be screwed up by implementations and hence needs to be tested in the UnifiedBackendTest as well.
 */
BOOST_AUTO_TEST_CASE(testVersionNumber) {
  TransferElementTestAccessor<int32_t> accessor;

  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber{nullptr}); // partial check for B.11.6

  VersionNumber v1;
  VersionNumber v2;
  accessor.resetCounters();
  accessor.write(v2);
  BOOST_CHECK(accessor._newVersion == v2); // B.11.4.2
  accessor.resetCounters();
  BOOST_CHECK_THROW(accessor.write(v1), ChimeraTK::logic_error); // B.11.4.1
  BOOST_CHECK(accessor._newVersion == v2);                       // B.11.4.2
}

/********************************************************************************************************************/

/** 
 *  This tests the TransferElement specifications:
 *  * \anchor testTransferElement_B_8_6_1 \ref transferElement_B_8_6_1 "B.8.6.1",
 *  * \anchor testTransferElement_B_8_6_2 \ref transferElement_B_8_6_2 "B.8.6.2",
 *  * \anchor testTransferElement_B_8_6_3 \ref transferElement_B_8_6_3 "B.8.6.3",
 *  * \anchor testTransferElement_B_8_6_4 \ref transferElement_B_8_6_4 "B.8.6.4" (base-class part) and
 *  * \anchor testTransferElement_B_8_6_5 \ref transferElement_B_8_6_5 "B.8.6.5".
 */
BOOST_AUTO_TEST_CASE(testInterrupt) {
  TransferElementTestAccessor<int32_t> accessor;

  BOOST_CHECK_THROW(accessor.interrupt(), ChimeraTK::logic_error); // B.8.6.5

  accessor._flags = {AccessMode::wait_for_new_data};

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
  accessor._readQueue.push();
  accessor.resetCounters();
  accessor.read();
  BOOST_CHECK(accessor._postRead_counter == 1);
}

/********************************************************************************************************************/
