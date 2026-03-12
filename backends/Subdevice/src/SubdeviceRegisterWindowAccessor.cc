// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceRegisterWindowAccessor.h"

#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename ReadWriteDataType>
  SubdeviceRegisterWindowAccessor<RegisterRawType, ReadWriteDataType>::SubdeviceRegisterWindowAccessor(
      boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName, size_t numberOfWords,
      size_t wordOffsetInRegister)
  : NDRegisterAccessor<RegisterRawType>(registerPathName, {AccessMode::raw}), _backend(std::move(backend)),
    _numberOfWords(numberOfWords) {
    auto& targetDevice = _backend->_targetDevice;
    auto& parameters = _backend->_parameters;
    if(!parameters["chipSelectRegister"].empty()) {
      _accChipSelect.replace(targetDevice->getRegisterAccessor<uint64_t>(parameters["chipSelectRegister"], 1, 0, {}));
    }
    _accAddress.replace(targetDevice->getRegisterAccessor<uint64_t>(parameters["address"], 1, 0, {}));
    if(!parameters["writeData"].empty()) {
      _accWriteData.replace(targetDevice->getRegisterAccessor<ReadWriteDataType>(parameters["writeData"], 0, 0, {}));
    }
    else if(!parameters["data"].empty()) {
      _accWriteData.replace(targetDevice->getRegisterAccessor<ReadWriteDataType>(parameters["data"], 0, 0, {}));
    }

    if(!parameters["busy"].empty()) {
      _accBusy.replace(targetDevice->getRegisterAccessor<ChimeraTK::Boolean>(parameters["busy"], 1, 0, {}));
    }
    else if(!parameters["status"].empty()) {
      _accBusy.replace(targetDevice->getRegisterAccessor<ChimeraTK::Boolean>(parameters["status"], 1, 0, {}));
    }

    if(!parameters["readData"].empty()) {
      _accReadRequest.replace(targetDevice->getRegisterAccessor<ChimeraTK::Void>(parameters["readRequest"], 1, 0, {}));
      _accReadData.replace(targetDevice->getRegisterAccessor<ReadWriteDataType>(parameters["readData"], 0, 0, {}));
    }

    auto info = _backend->_registerMap.getBackendRegister(registerPathName);
    _startAddress = info.address + info.elementPitchBits / 8 * wordOffsetInRegister;

    assert(_backend->_type == SubdeviceBackend::Type::registerWindow);

    if(_accReadData.isInitialised() && _accWriteData.isInitialised() &&
        _accReadData.getNElements() != _accWriteData.getNElements()) {
      throw ChimeraTK::logic_error(
          "SubDeviceBackend: In RegisterWindow mode, read and write data register must have the same size!");
    }

    NDRegisterAccessor<RegisterRawType>::buffer_2D.resize(1);
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);

    _endAddress = _startAddress + _numberOfWords * sizeof(RegisterRawType);
    if(_accWriteData.isInitialised()) {
      _transferSize = sizeof(ReadWriteDataType) * _accWriteData.getNElements();
    }
    else {
      assert(_accReadData.isInitialised());
      _transferSize = sizeof(ReadWriteDataType) * _accReadData.getNElements();
    }

    // Fixme: find better names
    // TransferAddresses are in units of the target read/write data register size, not in bytes.
    _starTransferAddress = _startAddress / _transferSize; // intentionally rounded down
    // endTransferAddress is endAddress / transferSize rounded up
    _endTransferAddress = (_endAddress + _transferSize - 1) / _transferSize;

    _zeros.assign(_transferSize, std::byte{0}); // bytes with 0 to copy from
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doReadTransferSynchronously() {
    transferImpl(TransferDirection::read);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doWriteTransfer(ChimeraTK::VersionNumber) {
    transferImpl(TransferDirection::write);
    return false;
  }

  /********************************************************************************************************************/
  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::transferImpl(TransferDirection direction) {
    assert(_backend->_mutex);
    std::lock_guard<std::mutex> lockGuard(*(_backend->_mutex));

    try {
      if(_accChipSelect.isInitialised()) {
        _accChipSelect.setAndWrite(_backend->_chipIndex);
      }

      size_t bufferCopyOffset = 0;
      for(size_t adr = _starTransferAddress; adr < _endTransferAddress; ++adr) {
        // copy data between buffer and read/write data accessor
        auto thisTransfersStartAddress = adr * _transferSize;
        auto thisTransfersEndAddress = thisTransfersStartAddress + _transferSize;
        auto copyStartAddress = std::max(thisTransfersStartAddress, _startAddress);
        auto copyEndAddress = std::min(thisTransfersEndAddress, _endAddress);
        auto copyNBytes = copyEndAddress - copyStartAddress;

        // only for the first transfer we might not start copying to/from the start of the accessor buffer
        auto deviceAccessorCopyOffset =
            (_startAddress > thisTransfersStartAddress ? _startAddress - thisTransfersStartAddress : 0);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* bufferStart = reinterpret_cast<std::byte*>(_buffer.data());

        // set the transfer address before starting the read/write transaction
        _accAddress.setAndWrite(adr);
        usleep(_backend->_addressToDataDelay); // FIXME: Do we still need this?

        if(direction == TransferDirection::write) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          auto* accessorStart = reinterpret_cast<std::byte*>(_accWriteData.data());

          // Fill leading 0s. This is intentionally not read/modify write. If there is relevant data
          // in these bytes, the SubdeviceRegisterWindowAccessor must be aligned with the write accessor
          // and a SubArrayDecorator or BitRangeDecorator must be used around it.
          if(deviceAccessorCopyOffset > 0) {
            memcpy(accessorStart, _zeros.data(), deviceAccessorCopyOffset);
          }

          memcpy(accessorStart + deviceAccessorCopyOffset, bufferStart + bufferCopyOffset, copyNBytes);

          // fill trailing 0s after the copy end
          if(copyEndAddress < thisTransfersEndAddress) {
            memcpy(accessorStart + deviceAccessorCopyOffset + copyNBytes, _zeros.data(),
                thisTransfersEndAddress - copyEndAddress);
          }

          _accWriteData.write();
        }
        else {
          assert(direction == TransferDirection::read);
          _accReadRequest.write();
        }

        // wait until transaction is complete
        if(_accBusy.isInitialised()) {
          // for 3regs/regWindow wait until status register is 0 again
          size_t retry = 0;
          size_t max_retry = _backend->_timeout * 1000 / _backend->_sleepTime;
          while(true) {
            usleep(_backend->_sleepTime);
            if(!_accBusy.readAndGet()) {
              break;
            }
            if(++retry > max_retry) {
              throw ChimeraTK::runtime_error("Write to register '" + this->_name +
                  "' failed: timeout waiting for cleared busy flag (" + _accBusy.getName() + ")");
            }
          }
        }
        else {
          // for 2regs, wait given time
          usleep(_backend->_sleepTime);
        }

        if(direction == TransferDirection::read) {
          _accReadData.read();
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          auto* accessorStart = reinterpret_cast<std::byte*>(_accReadData.data());
          memcpy(bufferStart + bufferCopyOffset, accessorStart + deviceAccessorCopyOffset, copyNBytes);
        }

        bufferCopyOffset += copyNBytes;
      }
    }
    catch(ChimeraTK::runtime_error& ex) {
      this->_exceptionBackend->setException(ex.what());
      throw;
    }
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
    // only registerWindow backends are readable
    assert(_backend->_type == SubdeviceBackend::Type::registerWindow);

    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress.isInitialised() && !_accAddress.isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: address register '" +
          _accAddress.getName() + "' is not writeable.");
    }
    if(!_accReadData.isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: read data register '" +
          _accReadData.getName() + "' is not readable.");
    }
    if(_accBusy.isInitialised() && !_accBusy.isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: status register '" +
          _accBusy.getName() + "' is not readable.");
    }
    assert(_accReadRequest.isInitialised());
    if(!_accReadRequest.isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: read request register '" +
          _accReadRequest.getName() + "' is not readable.");
    }

    // Apart from the tests there is nothing to do here
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
      NDRegisterAccessor<RegisterRawType>::_versionNumber = {}; // generate a new version number
      NDRegisterAccessor<RegisterRawType>::_dataValidity = _accReadData.dataValidity();
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPreWrite(TransferType, VersionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!_accWriteData.isInitialised()) {
      throw ChimeraTK::logic_error(
          "SubdeviceRegisterWindowAccessor[" + this->getName() + "]: Register is nor writeable");
    }
    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress.isInitialised() && !_accAddress.isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: address register '" +
          _accAddress.getName() + "' is not writeable.");
    }
    if(!_accWriteData.isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: write data register '" +
          _accWriteData.getName() + "' is not writeable.");
    }
    if(_backend->needStatusParam()) {
      if(!_accBusy.isReadable()) {
        throw ChimeraTK::logic_error("SubdeviceRegisterWindowAccessor[" + this->getName() + "]: status register '" +
            _accBusy.getName() + "' is not readable.");
      }
    }

    assert(NDRegisterAccessor<RegisterRawType>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
    _accWriteData.setDataValidity(NDRegisterAccessor<RegisterRawType>::_dataValidity);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::doPostWrite(TransferType, VersionNumber) {
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
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
    return _accReadRequest.isInitialised();
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterWindowAccessor<RegisterRawType, WriteDataType>::isWriteable() const {
    return _accWriteData.isInitialised();
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
    std::list<boost::shared_ptr<TransferElement>> retval = {_accAddress.getImpl()}; // always there

    if(_accWriteData.isInitialised()) {
      retval.emplace_back(_accWriteData.getImpl());
    }
    if(_accBusy.isInitialised()) { // nullprt for 2reg
      retval.emplace_back(_accBusy.getImpl());
    }
    if(_accChipSelect.isInitialised()) {
      retval.emplace_back(_accChipSelect.getImpl());
    }
    if(_accReadRequest.isInitialised()) {
      retval.emplace_back(_accReadRequest.getImpl());
    }
    if(_accReadData.isInitialised()) {
      retval.emplace_back(_accReadData.getImpl());
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
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(SubdeviceRegisterWindowAccessor, uint8_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(SubdeviceRegisterWindowAccessor, uint16_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(SubdeviceRegisterWindowAccessor, uint32_t);
  INSTANTIATE_MULTI_TEMPLATE_FOR_CHIMERATK_RAW_TYPES(SubdeviceRegisterWindowAccessor, uint64_t);

  // Compatibility stuff
  template class SubdeviceRegisterWindowAccessor<int8_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<int8_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<int8_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<int8_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<int16_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<int16_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<int16_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<int16_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<int32_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<int32_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<int32_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<int32_t, uint64_t>;

  template class SubdeviceRegisterWindowAccessor<int64_t, uint8_t>;
  template class SubdeviceRegisterWindowAccessor<int64_t, uint16_t>;
  template class SubdeviceRegisterWindowAccessor<int64_t, uint32_t>;
  template class SubdeviceRegisterWindowAccessor<int64_t, uint64_t>;

} // namespace ChimeraTK
