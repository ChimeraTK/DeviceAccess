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
  TransferElementTestAccessor() : NDRegisterAccessor<UserType>("someName") {}

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
  }

  void doReadTransferSynchronously(TransferType expectedType) {
    // will become an override (remove the argument)
    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == expectedType);
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
  }

  void doReadTransfer() override {
    // will be replaced by base-class functionality
    if(!_flags.has(AccessMode::wait_for_new_data)) {
      doReadTransferSynchronously(TransferType::read);
      return;
    }

    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::read);
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    _readQueue.pop_wait();
  }

  bool doReadTransferNonBlocking() override {
    // will be replaced by base-class functionality
    if(!_flags.has(AccessMode::wait_for_new_data)) {
      doReadTransferSynchronously(TransferType::readNonBlocking);
      return true;
    }

    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::readNonBlocking);
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    _hasNewDataOrDatLost = _readQueue.pop();
    return _hasNewDataOrDatLost;
  }

  bool doReadTransferLatest() override {
    // will be replaced by base-class functionality
    if(!_flags.has(AccessMode::wait_for_new_data)) {
      doReadTransferSynchronously(TransferType::readLatest);
      return true;
    }

    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK(_readTransfer_counter == 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::readLatest);
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    do {
      _hasNewDataOrDatLost = _readQueue.pop();
    } while(_hasNewDataOrDatLost);
    return _hasNewDataOrDatLost;
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
    return _hasNewDataOrDatLost;
  }
  void doPostRead(TransferType type, bool hasNewData) override {
    BOOST_CHECK(_preRead_counter == 1);
    BOOST_CHECK(_preWrite_counter == 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre) {
      BOOST_CHECK(_readTransfer_counter == 1);
    }
    else {
      // This tests the TransferElement specification B.6.1 for read operations
      BOOST_CHECK(_readTransfer_counter == 0);
    }
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postRead_counter;
    _hasNewDataOrDatLost = hasNewData;
  }

  void doPostWrite(TransferType type, bool dataLost) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK(_readTransfer_counter == 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre) {
      BOOST_CHECK(_writeTransfer_counter == 1);
    }
    else {
      // This tests the TransferElement specification B.6.1 for write operations
      BOOST_CHECK(_writeTransfer_counter == 0);
    }
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postWrite_counter;
    _hasNewDataOrDatLost = dataLost;
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

  AccessModeFlags getAccessModeFlags() const override { return _flags; }

  VersionNumber getVersionNumber() const override { return _currentVersion; }
  VersionNumber _currentVersion{nullptr};

  TransferFuture doReadTransferAsync() override { return _future; }
  cppext::future_queue<void> _readQueue{3};
  TransferFuture _future{_readQueue, this};

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
  }

  bool _throwLogicErr{false};
  bool _throwRuntimeErrInTransfer{false};
  bool _throwRuntimeErrInPre{false};
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

BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeNoExceptions) {
  // This tests the TransferElement specification B.4 (except B.4.3.2), B.5 (without sub-points) and B.7 (without B.7.4)
  // for accessors without AccessMode::wait_for_new_data
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

BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_SyncModeWithExceptions) {
  // This tests the TransferElement specification B.5.1 and B.7.4 for accessors without AccessMode::wait_for_new_data
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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::write);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = false;
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);
  */

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

  /* -> is currently not allowed by TE implementation
  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwOnceRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);
  */

  accessor.resetCounters();
  accessor._hasNewDataOrDatLost = true;
  accessor._throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == true);
  BOOST_CHECK(accessor._transferType == TransferType::writeDestructively);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_AsyncModeNoExceptions) {
  // This tests the TransferElement specification B.4 (except B.4.3.2), B.5 (without sub-points), B.7 (without B.7.4)
  // and B.8.2 for accessors with AccessMode::wait_for_new_data and read operations (write operations are not affected
  // by that flag)
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

BOOST_AUTO_TEST_CASE(testPreTransferPostSequence_AsyncModeWithExceptions) {
  // This tests the TransferElement specification B.5.1, B.7.4 and B.8.3 for accessors with
  // AccessMode::wait_for_new_data and read operations (write operations are not affected by that flag).
  // Note: since there is no difference between sync and async mode for logic_errors, only runtime_errors are tested
  //       here.
  TransferElementTestAccessor<int32_t> accessor;

  accessor._readable = true;
  accessor._writeable = false;
  accessor._flags = {AccessMode::wait_for_new_data};

  accessor.resetCounters();
  try {
    throw ChimeraTK::runtime_error("Test");
  }
  catch(...) {
    accessor._readQueue.push_exception(std::current_exception());
  }
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::read);

  accessor.resetCounters();
  try {
    throw ChimeraTK::runtime_error("Test");
  }
  catch(...) {
    accessor._readQueue.push_exception(std::current_exception());
  }
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor._postRead_counter == 1); // the other counters are checked in doPostRead
  BOOST_CHECK(accessor._hasNewDataOrDatLost == false);
  BOOST_CHECK(accessor._transferType == TransferType::readNonBlocking);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testPrePostPairingDuplicateCalls) {
  // This tests the TransferElement specification B.5.2
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
  accessor.postWrite(TransferType::write, false);
  accessor.postWrite(TransferType::write, false);
  accessor.postWrite(TransferType::write, false);
  BOOST_CHECK(accessor._postWrite_counter == 1); // the other counters are checked in doPostWrite

  // no need to test all read and write types, since the mechanism does not depend on the type.
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testPostWriteVersionNumberUpdate) {
  // This tests the TransferElement specification B.4.3.2
  std::cout << "FIXME This test case is disabled until functionality is implemented in TransferElement!" << std::endl;
  return;

  TransferElementTestAccessor<int32_t> accessor;

  // test initial value
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr));

  // test without exception
  accessor.resetCounters();
  VersionNumber v1{};
  accessor.preWrite(TransferType::write, v1);
  accessor.writeTransfer(v1);
  BOOST_CHECK(accessor.getVersionNumber() == VersionNumber(nullptr));
  accessor.postWrite(TransferType::write, false);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with logic error
  accessor.resetCounters();
  VersionNumber v2{};
  accessor._throwLogicErr = true;
  accessor.preWrite(TransferType::write, v2);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, false), ChimeraTK::logic_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  /* -> is currently not allowed by TE implementation
  // test with runtime error in preWrite
  accessor.resetCounters();
  VersionNumber v3{};
  accessor._throwRuntimeErrInPre = true;
  accessor.preWrite(TransferType::write, v3);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, false), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
*/

  // test with runtime error in doWriteTransfer
  accessor.resetCounters();
  VersionNumber v4{};
  accessor._throwRuntimeErrInTransfer = true;
  accessor.preWrite(TransferType::write, v4);
  accessor.writeTransfer(v4);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::write, false), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);

  // test with runtime error in doWriteTransferDestructively
  accessor.resetCounters();
  VersionNumber v5{};
  accessor._throwRuntimeErrInTransfer = true;
  accessor.preWrite(TransferType::writeDestructively, v5);
  accessor.writeTransferDestructively(v5);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
  BOOST_CHECK_THROW(accessor.postWrite(TransferType::writeDestructively, false), ChimeraTK::runtime_error);
  BOOST_CHECK(accessor.getVersionNumber() == v1);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testExceptionDelaying) {
  // This tests the TransferElement specification B.6 (incl. B.6.1 - which is implicitly tested by the
  // TransferElementTestAccessor counter checks)
  std::cout << "TODO!" << std::endl;
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testReadLatest) {
  // This tests the TransferElement specification B.3.1.4
  std::cout << "TODO!" << std::endl;
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDiscardValueException) {
  // This tests the TransferElement specification B.8.2.2
  std::cout << "TODO!" << std::endl;
}

/********************************************************************************************************************/
