// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceRegisterWindowAccessor.h"

#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename ReadWriteDataType>
  SubdeviceRegisterWindowAccessor<RegisterRawType, ReadWriteDataType>::SubdeviceRegisterWindowAccessor(
      boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accChipSelect,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<ReadWriteDataType>> accWriteDataArea,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accStatus,
      boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> accReadRequest,
      boost::shared_ptr<NDRegisterAccessor<ReadWriteDataType>> accReadData, size_t byteOffset, size_t numberOfWords)
  : NDRegisterAccessor<RegisterRawType>(registerPathName, {AccessMode::raw}), _backend(std::move(backend)),
    _accChipSelect(std::move(accChipSelect)), _accAddress(std::move(accAddress)),
    _accWriteDataArea(std::move(accWriteDataArea)), _accStatus(std::move(accStatus)),
    _accReadRequest(std::move(accReadRequest)), _accReadData(std::move(accReadData)), _startAddress(byteOffset),
    _numberOfWords(numberOfWords) {
    assert(_backend->_type == SubdeviceBackend::Type::sixRegisters ||
        _backend->_type == SubdeviceBackend::Type::threeRegisters ||
        _backend->_type == SubdeviceBackend::Type::twoRegisters);

    NDRegisterAccessor<RegisterRawType>::buffer_2D.resize(1);
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doReadTransferSynchronously() {
    std::lock_guard<decltype(_backend->_mutex)> lockGuard(_backend->_mutex);
    assert(_backend->_type == SubdeviceBackend::Type::sixRegisters);

    auto nTransfers = _numberOfWords; // At the moment, only scalar readData registers are supported.
    try {
      _accChipSelect->accessData(0) = _backend->_chipIndex;
      _accChipSelect->write();

      size_t idx = 0;
      for(size_t adr = _startAddress; adr < _startAddress + nTransfers; ++adr) {
        // write address register
        _accAddress->accessData(0) = static_cast<uint64_t>(adr);
        _accAddress->write();
        usleep(_backend->_addressToDataDelay);

        // send read request
        _accReadRequest->write();

        // wait until transaction is complete
        size_t retry = 0;
        size_t max_retry = _backend->_timeout * 1000 / _backend->_sleepTime;
        while(true) {
          usleep(_backend->_sleepTime);
          _accStatus->read();
          if(_accStatus->accessData(0) == 0) {
            break;
          }
          if(++retry > max_retry) {
            throw ChimeraTK::runtime_error("Reading from register '" + this->_name +
                "' failed: timeout waiting for cleared busy flag (" + _accStatus->getName() + ")");
          }
        }

        // read data and copy to buffer
        _accReadData->read();
        _buffer[idx++] = RegisterRawType(_accReadData->accessData(0));
      }
    }
    catch(ChimeraTK::runtime_error& ex) {
      this->_exceptionBackend->setException(ex.what());
      throw;
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doWriteTransfer(ChimeraTK::VersionNumber) {
    std::lock_guard<decltype(_backend->_mutex)> lockGuard(_backend->_mutex);

    // Fixme: find better names
    // Fixme: move to constructor. Does not change and is also needed in read.
    // TransferAddresses are in units of the target read/write data register size, not in bytes.
    size_t starTransferAddress = _startAddress / sizeof(WriteDataType); // intentionally rounded down
    auto endAddress = _startAddress + _numberOfWords * sizeof(RegisterRawType);
    auto transferSize = sizeof(WriteDataType) * _accWriteDataArea->getNumberOfSamples();
    // endTransferAddress is endAddress / transferSize rounded up
    size_t endTransferAddress = (endAddress + transferSize - 1) / transferSize;

    try {
      if(_accChipSelect) {
        _accChipSelect->accessData(0) = _backend->_chipIndex;
        _accChipSelect->write();
      }

      size_t bufferCopyOffset = 0;
      for(size_t adr = starTransferAddress; adr < endTransferAddress; ++adr) {
        // copy data from buffer into write data accessor
        auto thisTransferStartAddress = adr * transferSize;
        auto thisTransferEndAddress = thisTransferStartAddress + transferSize;
        auto copyStartAddress = std::max(thisTransferStartAddress, _startAddress);
        auto copyEndAddress = std::min(thisTransferEndAddress, endAddress);
        auto copyNBytes = copyEndAddress - copyStartAddress;

        // only for the first transfer we might not start copying to the start of the accessor buffer
        auto deviceAccessorCopyOffset =
            (_startAddress > thisTransferStartAddress ? _startAddress - thisTransferStartAddress : 0);
        // FIXME: fill start with 0s

        auto* src = std::bit_cast<std::byte*>(_buffer.data()) + bufferCopyOffset;
        auto* dest = std::bit_cast<std::byte*>(_accWriteDataArea->accessChannel(0).data()) + deviceAccessorCopyOffset;

        memcpy(dest, src, copyNBytes);

        // FIXME: fill end with 0s

        // set the transfer address
        _accAddress->accessData(0) = adr;
        _accAddress->write();
        usleep(_backend->_addressToDataDelay); // FIXME: Do we still need this?

        // Write data to device and start the transfer there
        _accWriteDataArea->write();

        // wait until transaction is complete
        if(_accStatus) {
          // for 3regs/6regs wait until status register is 0 again
          size_t retry = 0;
          size_t max_retry = _backend->_timeout * 1000 / _backend->_sleepTime;
          while(true) {
            usleep(_backend->_sleepTime);
            _accStatus->read();
            if(_accStatus->accessData(0) == 0) {
              break;
            }
            if(++retry > max_retry) {
              throw ChimeraTK::runtime_error("Write to register '" + this->_name +
                  "' failed: timeout waiting for cleared busy flag (" + _accStatus->getName() + ")");
            }
          }
        }
        else {
          // for 2regs, wait given time
          usleep(_backend->_sleepTime);
        }

        bufferCopyOffset += copyNBytes;
      }
    }
    catch(ChimeraTK::runtime_error& ex) {
      this->_exceptionBackend->setException(ex.what());
      throw;
    }
    return false;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPreRead(TransferType) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!isReadable()) {
      throw ChimeraTK::logic_error(
          "SubdeviceRegisterWindowAccessor[" + this->getName() + "]: Register is not readable");
    }
    // only 6reg backends are readable
    assert(_backend->_type == SubdeviceBackend::Type::sixRegisters);

    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress && !_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accReadData->isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: read data register '" +
          _accReadData->getName() + "' is not readable.");
    }
    if(!_accStatus->isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: status register '" +
          _accStatus->getName() + "' is not readable.");
    }
    if(!_accReadRequest->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: read request register '" +
          _accReadRequest->getName() + "' is not readable.");
    }

    // Apart from the tests there is nothing to do here
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
      NDRegisterAccessor<RegisterRawType>::_versionNumber = {}; // generate a new version number
      NDRegisterAccessor<RegisterRawType>::_dataValidity = _accReadData->dataValidity();
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPreWrite(TransferType, VersionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!_accWriteDataArea) {
      throw ChimeraTK::logic_error(
          "SubdeviceRegisterWindowAccessor[" + this->getName() + "]: Register is nor writeable");
    }
    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress && !_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accWriteDataArea->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: data/area register '" +
          _accWriteDataArea->getName() + "' is not writeable.");
    }
    if(_backend->needStatusParam()) {
      if(!_accStatus->isReadable()) {
        throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: status register '" +
            _accStatus->getName() + "' is not readable.");
      }
    }

    assert(NDRegisterAccessor<RegisterRawType>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
    _accWriteDataArea->setDataValidity(NDRegisterAccessor<RegisterRawType>::_dataValidity);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPostWrite(TransferType, VersionNumber) {
    if(!this->_activeException) {
      NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::mayReplaceOther(
      const boost::shared_ptr<TransferElement const>& other) const {
    auto castedOther = boost::dynamic_pointer_cast<SubdeviceRegisterWindowAccessor const>(other);
    if(!castedOther) {
      return false;
    }
    if(castedOther.get() == this) {
      return false;
    }

    return (_backend == castedOther->_backend) && (this->_name == castedOther->_name) &&
        (_numberOfWords == castedOther->_numberOfWords) && (_startAddress == castedOther->_startAddress);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::isReadOnly() const {
    return isReadable() && !isWriteable();
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::isReadable() const {
    return _accReadRequest != nullptr;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::isWriteable() const {
    return _accWriteDataArea != nullptr;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  std::vector<boost::shared_ptr<TransferElement>> SubdeviceRegisterWindowAccessor<RegisterRawType,
      WriteDataType>::getHardwareAccessingElements() {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  std::list<boost::shared_ptr<TransferElement>> SubdeviceRegisterWindowAccessor<RegisterRawType,
      WriteDataType>::getInternalElements() {
    std::list<boost::shared_ptr<TransferElement>> retval = {_accWriteDataArea}; // always there

    if(_accAddress) { // nullprt for areaHandshake
      retval.emplace_back(_accAddress);
    }
    if(_accStatus) { // nullprt for 2reg
      retval.emplace_back(_accStatus);
    }
    if(_accChipSelect) { // only for 6reg
      retval.emplace_back(_accChipSelect);
    }
    if(_accReadRequest) { // only for 6reg
      retval.emplace_back(_accReadRequest);
    }
    if(_accReadData) { // only for 6reg
      retval.emplace_back(_accReadData);
    }
    return retval;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::replaceTransferElement(
      boost::shared_ptr<TransferElement>) {
    // Nothing to replace here. The necessary read/write with the handshake cannot be merged with anything.
  }

  /********************************************************************************************************************/
  // Code instantiations for the allowed raw types
  // FIXME: Do we have a "for raw types" macro? Change to uint if so.

  template class SubdeviceRegisterWindowAccessor<uint8_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<uint8_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<uint8_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<uint8_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<uint16_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<uint16_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<uint16_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<uint16_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<uint32_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<uint32_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<uint32_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<uint32_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<uint64_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<uint64_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<uint64_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<uint64_t, uint64_t>;

  // FIXME: This compatibility stuff should not be necessary
  template class SubdeviceRegisterWindowAccessor<int8_t, int8_t>;
  template class SubdeviceRegisterWindowAccessor<int16_t, int16_t>;
  template class SubdeviceRegisterWindowAccessor<int32_t, int32_t>;
  template class SubdeviceRegisterWindowAccessor<int64_t, int64_t>;

} // namespace ChimeraTK
