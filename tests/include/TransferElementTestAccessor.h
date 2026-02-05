// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"

namespace ChimeraTK {

  /** Special accessor used to test the behaviour of the TransferElement base class and the TransferGroup. */
  template<typename UserType>
  class TransferElementTestAccessor : public NDRegisterAccessor<UserType> {
   public:
    TransferElementTestAccessor(AccessModeFlags flags) : NDRegisterAccessor<UserType>("someName", flags) {
      // this accessor uses a queue length of 3
      this->_readQueue = {3};
      this->buffer_2D.resize(1);
      this->buffer_2D[0].resize(1);
    }

    ~TransferElementTestAccessor() override {}

    void doPreRead(TransferType type) override {
      _transferType_pre = type;
      ++_preRead_counter;
      _preIndex = _currentIndex;
      ++_currentIndex;
      try {
        if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
        if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
        if(!_readable) {
          throw ChimeraTK::logic_error("Not readable!");
        }
      }
      catch(...) {
        _thrownException = std::current_exception();
        throw;
      }
    }

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      _transferType_pre = type;
      ++_preWrite_counter;
      _preIndex = _currentIndex;
      ++_currentIndex;
      _preWrite_version = versionNumber;
      try {
        if(_throwLogicErr) throw ChimeraTK::logic_error("Test");
        if(_throwRuntimeErrInPre) throw ChimeraTK::runtime_error("Test");
        if(!_writeable) {
          throw ChimeraTK::logic_error("Not writeable!");
        }
      }
      catch(...) {
        _thrownException = std::current_exception();
        throw;
      }
    }

    void doReadTransferSynchronously() override {
      ++_readTransfer_counter;
      _transferIndex = _currentIndex;
      ++_currentIndex;
      try {
        if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
      }
      catch(...) {
        _thrownException = std::current_exception();
        throw;
      }
    }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override {
      ++_writeTransfer_counter;
      _transferIndex = _currentIndex;
      ++_currentIndex;
      _writeTransfer_version = versionNumber;
      try {
        if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
      }
      catch(...) {
        _thrownException = std::current_exception();
        throw;
      }
      return _previousDataLost;
    }

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
      ++_writeTransferDestructively_counter;
      _transferIndex = _currentIndex;
      ++_currentIndex;
      _writeTransfer_version = versionNumber;
      try {
        if(_throwRuntimeErrInTransfer) throw ChimeraTK::runtime_error("Test");
      }
      catch(...) {
        _thrownException = std::current_exception();
        throw;
      }
      return _previousDataLost;
    }

    void doPostRead(TransferType type, bool updateDataBuffer) override {
      _transferType_post = type;
      ++_postRead_counter;
      _postIndex = _currentIndex;
      ++_currentIndex;
      _updateDataBuffer = updateDataBuffer;
      _seenActiveException = _activeException;

      if(_setPostReadVersion == VersionNumber{nullptr}) {
        TransferElement::_versionNumber = {};
      }
      else {
        TransferElement::_versionNumber = _setPostReadVersion;
      }

      this->buffer_2D[0][0] = _setPostReadData;
    }

    void doPostWrite(TransferType type, VersionNumber versionNumber) override {
      _transferType_post = type;
      ++_postWrite_counter;
      _postIndex = _currentIndex;
      ++_currentIndex;
      _postWrite_version = versionNumber;
      _seenActiveException = _activeException;
    }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      if(this == other.get()) {
        return false;
      }

      return _listMayReplaceElements.count(other->getId());
    }
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      if(_hardwareAccessingElements.size() == 0) {
        // cannot call shared_from_this() in constructor, so we cannot put it by default into the list...
        return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
      }
      return _hardwareAccessingElements;
    }
    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      std::list<boost::shared_ptr<TransferElement>> r;
      for(auto& e : _internalElements) r.push_back(e);
      return r;
    }
    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      _listReplacementElements.push_back(newElement->getId());
    }

    bool isReadOnly() const override { return !_writeable && _readable; }

    bool isReadable() const override { return _readable; }

    bool isWriteable() const override { return _writeable; }

    void interrupt() override { this->interrupt_impl(this->_readQueue); }

    bool _writeable{true};
    bool _readable{true};

    // counter flags to check which functions have been called how many times and in which order (via index)
    size_t _preRead_counter{0};
    size_t _preWrite_counter{0};
    size_t _readTransfer_counter{0};
    size_t _writeTransfer_counter{0};
    size_t _writeTransferDestructively_counter{0};
    size_t _postRead_counter{0};
    size_t _postWrite_counter{0};
    size_t _preIndex{999999};
    size_t _transferIndex{999999};
    size_t _postIndex{999999};
    static std::atomic<size_t> _currentIndex; // allows comparison across transfer elements

    // recorded function arguments etc.
    TransferType _transferType_pre, _transferType_post; // TransferType as seen in pre/postXxx()
    bool _updateDataBuffer;                             // updateDataBuffer as seen in postRead() (set there)
    VersionNumber _preWrite_version{nullptr};
    VersionNumber _writeTransfer_version{nullptr};
    VersionNumber _postWrite_version{nullptr};
    std::exception_ptr _seenActiveException{nullptr}; // _activeException as seen in postXxx()
    std::exception_ptr _thrownException{nullptr};     // the exception thrown by request (via command flags below)

    // command flags to steer behaviour of this TE
    bool _previousDataLost{false}; // flag to return by writeTransfer()/writeTransferDestructively()
    bool _throwLogicErr{false};    // always in doPreXxx()
    bool _throwRuntimeErrInTransfer{false};
    bool _throwRuntimeErrInPre{false};
    bool _throwNumericCast{false};              // in doPreWrite() or doPreRead() depending on operation
    VersionNumber _setPostReadVersion{nullptr}; // if nullptr, a new version will be generated
    UserType _setPostReadData{UserType()};      // data to be copied into the user buffer in postRead

    // lists, counters etc. used for the TransferGroup tests
    std::list<TransferElementID> _listReplacementElements; // list of all arguments of replaceTransferElement()
    std::vector<boost::shared_ptr<TransferElementTestAccessor<UserType>>> _internalElements; // returned by
                                                                                             // getInternalElements()
    std::vector<boost::shared_ptr<TransferElement>> _hardwareAccessingElements;              // returned by
                                                                                // getHardwareAccessingElements()
    std::set<TransferElementID> _listMayReplaceElements; // mayReplaceOther() returns true if ID is found in this set

    // reset all counters and revert command flags to defaults
    void resetCounters() {
      _preRead_counter = 0;
      _preWrite_counter = 0;
      _readTransfer_counter = 0;
      _writeTransfer_counter = 0;
      _writeTransferDestructively_counter = 0;
      _postRead_counter = 0;
      _postWrite_counter = 0;
      _preIndex = 999;
      _transferIndex = 999;
      _postIndex = 999;
      _currentIndex = 0;
      _throwLogicErr = false;
      _throwRuntimeErrInPre = false;
      _throwRuntimeErrInTransfer = false;
      _throwNumericCast = false;
      _preWrite_version = VersionNumber{nullptr};
      _writeTransfer_version = VersionNumber{nullptr};
      _postWrite_version = VersionNumber{nullptr};
      _seenActiveException = nullptr;
      _thrownException = nullptr;
      _previousDataLost = false;
      _throwLogicErr = false;
      _throwRuntimeErrInTransfer = false;
      _throwRuntimeErrInPre = false;
      _throwNumericCast = false;
      _setPostReadVersion = VersionNumber{nullptr};
      _listReplacementElements.clear();
    }

    // convenience function to put exceptions onto the readQueue (see also interrupt())
    void putRuntimeErrorOnQueue() {
      try {
        throw ChimeraTK::runtime_error("Test");
      }
      catch(...) {
        _thrownException = std::current_exception();
        this->_readQueue.push_exception(std::current_exception());
      }
    }
    void putDiscardValueOnQueue() {
      try {
        throw ChimeraTK::detail::DiscardValueException();
      }
      catch(...) {
        _thrownException = std::current_exception();
        this->_readQueue.push_exception(std::current_exception());
      }
    }
    // simulate a reveiver thread by manually putting data into the queue
    bool push() { return this->_readQueue.push(); }

    using TransferElement::_activeException;
    using TransferElement::_readQueue;
    using TransferElement::_accessModeFlags;
  };

  template<typename T>
  std::atomic<size_t> TransferElementTestAccessor<T>::_currentIndex(0);

} // namespace ChimeraTK
