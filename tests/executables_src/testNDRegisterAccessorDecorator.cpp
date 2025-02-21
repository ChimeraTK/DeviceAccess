// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

/**********************************************************************************************************************/
/* Tests for the NDReigsterAccessorDecorator base class                                                             */
/**********************************************************************************************************************/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TransferElementTest
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "DeviceBackendImpl.h"
#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"

#include <boost/make_shared.hpp>

using namespace ChimeraTK;

/**********************************************************************************************************************/

/** Special accessor used to test the behaviour of the NDReigsterAccessorDecorator base class */
template<typename UserType>
class DecoratorTestAccessor : public NDRegisterAccessor<UserType> {
 public:
  DecoratorTestAccessor(AccessModeFlags flags) : NDRegisterAccessor<UserType>("someName", flags) {
    // this accessor uses a queue length of 3
    this->_readQueue = {3};
  }

  ~DecoratorTestAccessor() override {}

  void doPreRead(TransferType type) override {
    _transferType = type;
    if(this->_accessModeFlags.has(AccessMode::wait_for_new_data) && type == TransferType::readNonBlocking) {
      // if the access mode has wait_for_new_data then readNonBlocking can be called multiple times by readLatest,
      // without the test re-setting the counter. In this case just check that the number of calls is smaller or euqal
      // the queue size i.e. called maximum size + 1 times.
      BOOST_CHECK(_preRead_counter <= (this->_readQueue.size()));
    }
    else {
      // in all other pre-read must be called exactly once
      BOOST_CHECK_EQUAL(_preRead_counter, 0);
    }
    // in all cases, doPreRead and doPostRead must be called in pairs
    BOOST_CHECK_EQUAL(_preRead_counter, _postRead_counter);
    BOOST_CHECK(_preWrite_counter == 0);
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
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
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
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
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(this->_accessModeFlags.has(AccessMode::wait_for_new_data) == false);
    BOOST_CHECK((_transferType == TransferType::read) || (_transferType == TransferType::readNonBlocking) ||
        (_transferType == TransferType::readLatest));
    if(_throwLogicErr || _throwRuntimeErrInPre || _throwThreadInterruptedInPre) {
      BOOST_CHECK_MESSAGE(
          false, "doReadTransferSynchronously() must not be called if doPreRead() has thrown an exception.");
    }
    ++_readTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
  }

  bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::write);
    BOOST_CHECK(versionNumber == _newVersion);
    if(_throwLogicErr || _throwRuntimeErrInPre || _throwThreadInterruptedInPre || _throwNumericCast) {
      BOOST_CHECK_MESSAGE(false, "doWriteTransfer() must not be called if doPreWrite() has thrown an exception.");
    }
    ++_writeTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
    return _previousDataLost;
  }

  bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == TransferType::writeDestructively);
    BOOST_CHECK(versionNumber == _newVersion);
    if(_throwLogicErr || _throwRuntimeErrInPre || _throwThreadInterruptedInPre || _throwNumericCast) {
      BOOST_CHECK_MESSAGE(
          false, "doWriteTransferDestructively() must not be called if doPreWrite() has thrown an exception.");
    }
    ++_writeTransfer_counter;
    if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
    if(_throwThreadInterruptedInTransfer) throw boost::thread_interrupted();
    return _previousDataLost;
  }

  /**
   * This doPostRead() implementation checks partially the TransferElement specification B.4.
   *
   * \anchor testTransferElement_B_6_1_read It also tests specifically the
   * \ref transferElement_B_6_1 "TransferElement specification B.6.1" for read operations,
   * \anchor testTransferElement_B_6_3_read \ref transferElement_B_6_3 "TransferElement specification B.6.3" for read
   * operations, and \ref transferElement_B_7_4 "TransferElement specification B.7.4"
   */
  void doPostRead(TransferType type, bool updateDataBuffer) override {
    // doPreRead and doPortRead must always be called in pairs.
    // This can happen multiple times in readLatest. The absolute counting is done in doPreRead()
    BOOST_CHECK_EQUAL(_preRead_counter, _postRead_counter + 1);
    BOOST_CHECK(_preWrite_counter == 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre && !_throwThreadInterruptedInPre) {
      if(!this->_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        BOOST_CHECK_EQUAL(_readTransfer_counter, 1);
      }
      else {
        BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
      }
    }
    else {
      /* Here B.6.1 is tested for read operations */
      BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    }
    if(_throwLogicErr || _throwRuntimeErrInPre || _throwThreadInterruptedInPre || _throwRuntimeErrInTransfer ||
        _throwThreadInterruptedInTransfer) {
      BOOST_CHECK(this->_activeException != nullptr);
    }
    /* Check B.7.4 */
    if(this->_activeException) {
      BOOST_CHECK(updateDataBuffer == false);
    }
    BOOST_CHECK(_writeTransfer_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postRead_counter;
    _hasNewData = updateDataBuffer;
    if(_throwNumericCast) throw boost::numeric::bad_numeric_cast();
    if(_throwThreadInterruptedInPost) throw boost::thread_interrupted();
  }

  /**
   * This doPostWrite() implementation checks partially the TransferElement specification B.4.
   *
   * \anchor testTransferElement_B_6_1_write It also tests specifically the
   * \ref transferElement_B_6_1 "TransferElement specification B.6.1" for write operations.
   * \anchor testTransferElement_B_6_3_write \ref transferElement_B_6_3 "TransferElement specification B.6.3" for write
   * operations.
   */
  void doPostWrite(TransferType type, VersionNumber versionNumber) override {
    BOOST_CHECK(_preRead_counter == 0);
    BOOST_CHECK(_preWrite_counter == 1);
    BOOST_CHECK_EQUAL(_readTransfer_counter, 0);
    if(!_throwLogicErr && !_throwRuntimeErrInPre && !_throwNumericCast && !_throwThreadInterruptedInPre) {
      BOOST_CHECK(_writeTransfer_counter == 1);
    }
    else {
      /* Here B.6.1 is tested for write operations */
      BOOST_CHECK(_writeTransfer_counter == 0);
    }
    // Exceptions must be passed on to the level which is throwing it (B.6.3 This actually tests the
    // NDRegisterAccessorDecorator)
    if(_throwLogicErr || _throwRuntimeErrInPre || _throwThreadInterruptedInPre || _throwRuntimeErrInTransfer ||
        _throwThreadInterruptedInTransfer || _throwNumericCast) {
      BOOST_CHECK(this->_activeException != nullptr);
    }
    BOOST_CHECK(versionNumber == _newVersion);
    BOOST_CHECK(_postRead_counter == 0);
    BOOST_CHECK(_postWrite_counter == 0);
    BOOST_CHECK(_transferType == type);
    ++_postWrite_counter;
    if(_throwThreadInterruptedInPost) throw boost::thread_interrupted();
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

  TransferType _transferType;
  bool _hasNewData;              // hasNewData as seen in postRead() (set there)
  bool _previousDataLost{false}; // this value will be returned by write and writeDestructively. Not changed by the
                                 // accessor.
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
};

/**********************************************************************************************************************/

/**
 *  This test that the NDRegisterAccessorDecorator base class complies to the following specification:
 *  * \anchor testTransferElement_B_6_3 \ref transferElement_B_6_3 "TransferElement specification B.6.3" through the
 *    tests in \ref testTransferElement_B_6_3_write "doPostWrite()" and \ref testTransferElement_B_6_3_write
 * "doPostRead()"
 *
 *  FIXME: The test is done on a very high level and tests many other things as well, which are already tested else
 *         where.
 */
BOOST_AUTO_TEST_CASE(TestExceptionHandling) {
  std::cout << "TestExceptionHandling" << std::endl;
  auto targetAccessor = boost::make_shared<DecoratorTestAccessor<int32_t>>(AccessModeFlags({}));
  // an empty decorator is sufficient for the test we want to make with TransferElement.
  ChimeraTK::NDRegisterAccessorDecorator<int32_t> accessor(targetAccessor);

  targetAccessor->resetCounters();
  targetAccessor->_throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::logic_error);
  // tests for B.6.1. and B.7.4 are done in doPostRead()
  targetAccessor->resetCounters();
  targetAccessor->_throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::logic_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::logic_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwLogicErr = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::logic_error);

  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);

  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.write(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInPre = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), boost::thread_interrupted);

  // tests B.7.4 (exception thrown in read transfer), and B.6.3 (active exception is seen in the layer which raised it)
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), ChimeraTK::runtime_error);
  targetAccessor->resetCounters();
  targetAccessor->_throwRuntimeErrInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), ChimeraTK::runtime_error);

  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.read(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.readNonBlocking(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.write(), boost::thread_interrupted);
  targetAccessor->resetCounters();
  targetAccessor->_throwThreadInterruptedInTransfer = true;
  BOOST_CHECK_THROW(accessor.writeDestructively(), boost::thread_interrupted);
}

/**********************************************************************************************************************/

template<typename UserType>
class TestNDRegisterAccessorDecorator : public NDRegisterAccessorDecorator<UserType, UserType> {
 public:
  using NDRegisterAccessorDecorator<UserType, UserType>::NDRegisterAccessorDecorator;
  using NDRegisterAccessorDecorator<UserType, UserType>::_target;
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(TestDecorateDeepInside) {
  std::cout << "TestDecorateDeepInside" << std::endl;
  auto target = boost::make_shared<DecoratorTestAccessor<int32_t>>(AccessModeFlags({}));
  auto deco1 = boost::make_shared<TestNDRegisterAccessorDecorator<int32_t>>(target);
  auto deco2 = boost::make_shared<TestNDRegisterAccessorDecorator<int32_t>>(deco1);
  auto deco3 = boost::make_shared<TestNDRegisterAccessorDecorator<int32_t>>(deco2);

  // first rest with a factory which never actually decorates, to see if the factory is called in the right sequence
  std::vector<boost::shared_ptr<NDRegisterAccessor<int32_t>>> paramsSeen;
  auto deco0 = deco3->decorateDeepInside(
      [&](const boost::shared_ptr<NDRegisterAccessor<int32_t>>& t) -> boost::shared_ptr<NDRegisterAccessor<int32_t>> {
        paramsSeen.push_back(t);
        return {};
      });
  BOOST_TEST(paramsSeen.size() == 3);
  BOOST_TEST(paramsSeen[0] == target);
  BOOST_TEST(paramsSeen[1] == deco1);
  BOOST_TEST(paramsSeen[2] == deco2);
  BOOST_TEST(!deco0);

  // test the actual decoration
  boost::shared_ptr<TestNDRegisterAccessorDecorator<int32_t>> decoCreated;
  auto decoReturned = deco3->decorateDeepInside(
      [&](const boost::shared_ptr<NDRegisterAccessor<int32_t>>& t) -> boost::shared_ptr<NDRegisterAccessor<int32_t>> {
        decoCreated = boost::make_shared<TestNDRegisterAccessorDecorator<int32_t>>(t);
        return decoCreated;
      });
  BOOST_TEST(decoReturned == decoCreated);
  BOOST_TEST(decoCreated->_target == target);
  BOOST_TEST(deco1->_target == decoCreated);
}

/**********************************************************************************************************************/
