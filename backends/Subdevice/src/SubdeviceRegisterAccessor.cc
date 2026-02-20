// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceRegisterAccessor.h"

#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::SubdeviceRegisterAccessor(
      boost::shared_ptr<SubdeviceBackend> backend, const std::string& registerPathName,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accChipSelect,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<WriteDataType>> accWriteDataArea,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accStatus,
      boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> accReadRequest,
      boost::shared_ptr<NDRegisterAccessor<uint64_t>> accReadData, size_t byteOffset, size_t numberOfWords)
  : NDRegisterAccessor<RegisterRawType>(registerPathName, {AccessMode::raw}), _backend(std::move(backend)),
    _accChipSelect(std::move(accChipSelect)), _accAddress(std::move(accAddress)),
    _accWriteDataArea(std::move(accWriteDataArea)), _accStatus(std::move(accStatus)),
    _accReadRequest(std::move(accReadRequest)), _accReadData(std::move(accReadData)), _startAddress(byteOffset),
    _numberOfWords(numberOfWords) {
    // Current limitation in the 6Reg interface: readData may only be a single word
    if(_accReadData && _accReadData->getNumberOfSamples() != 1) {
      throw ChimeraTK::logic_error("SubdeviceRegister 6reg: Unsupported map file. ReadData register " +
          _accReadData->getName() + " must have size 1.");
    }
    assert(_backend->_type == SubdeviceBackend::Type::areaHandshake ? sizeof(RegisterRawType) == 4 : true);

    NDRegisterAccessor<RegisterRawType>::buffer_2D.resize(1);
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doReadTransferSynchronously() {
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
  bool SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doWriteTransfer(ChimeraTK::VersionNumber) {
    std::lock_guard<decltype(_backend->_mutex)> lockGuard(_backend->_mutex);
    size_t nTransfers;
    if(_backend->_type == SubdeviceBackend::Type::areaHandshake) {
      // for areaHandshake case with 1D array
      // TODO FIXME shouldn't this be nTransfers = 1 ???
      nTransfers = _numberOfWords;
    }
    else {
      assert(_backend->_type == SubdeviceBackend::Type::sixRegisters ||
          _backend->_type == SubdeviceBackend::Type::threeRegisters ||
          _backend->_type == SubdeviceBackend::Type::twoRegisters);
      // This is "_numberOfWords / _accWriteData->getNumberOfSamples()" rounded up:
      nTransfers =
          (_numberOfWords + _accWriteDataArea->getNumberOfSamples() - 1) / _accWriteDataArea->getNumberOfSamples();
    }
    try {
      if(_backend->_type == SubdeviceBackend::Type::sixRegisters) {
        _accChipSelect->accessData(0) = _backend->_chipIndex;
        _accChipSelect->write();
      }

      size_t idx = 0;
      for(size_t adr = _startAddress; adr < _startAddress + nTransfers; ++adr) {
        // write address register (if applicable)
        if(_backend->_type != SubdeviceBackend::Type::areaHandshake) {
          _accAddress->accessData(0) = static_cast<uint64_t>(adr);
          _accAddress->write();
          usleep(_backend->_addressToDataDelay);
        }

        // write data register
        if(_backend->_type == SubdeviceBackend::Type::areaHandshake) {
          int32_t val = (idx < _numberOfWords) ? _buffer[idx] : 0;
          _accWriteDataArea->accessData(0, idx) = val;
          ++idx;
        }
        else {
          for(size_t innerOffset = 0; innerOffset < _accWriteDataArea->getNumberOfSamples(); ++innerOffset) {
            // pad data with zeros, if _numberOfWords isn't an integer multiple of _accData->getNumberOfSamples()
            RegisterRawType val = (idx < _numberOfWords) ? _buffer[idx] : 0;
            _accWriteDataArea->accessData(0, innerOffset) = uint64_t(val); // the area is fixed to uint64_t
            ++idx;
          }
        }
        _accWriteDataArea->write();

        // wait until transaction is complete
        if(_backend->_type == SubdeviceBackend::Type::threeRegisters ||
            _backend->_type == SubdeviceBackend::Type::sixRegisters ||
            _backend->_type == SubdeviceBackend::Type::areaHandshake) {
          // for 3regs/6regs/areaHandshake, wait until status register is 0 again
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
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doPreRead(TransferType) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: Register is not readable");
    }
    // only 6reg backends are readable
    assert(_backend->_type == SubdeviceBackend::Type::sixRegisters);

    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress && !_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accReadData->isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: read data register '" +
          _accReadData->getName() + "' is not readable.");
    }
    if(!_accStatus->isReadable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: status register '" +
          _accStatus->getName() + "' is not readable.");
    }
    if(!_accReadRequest->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: read request register '" +
          _accReadRequest->getName() + "' is not readable.");
    }

    // Apart from the tests there is nothing to do here
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
      NDRegisterAccessor<RegisterRawType>::_versionNumber = {}; // generate a new version number
      NDRegisterAccessor<RegisterRawType>::_dataValidity = DataValidity::ok;
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doPreWrite(TransferType, VersionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!_accWriteDataArea) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: Register is nor writeable");
    }
    // FIXME: Shouldn't these tests be done in the constructor?
    if(_accAddress && !_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accWriteDataArea->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: data/area register '" +
          _accWriteDataArea->getName() + "' is not writeable.");
    }
    if(_backend->needStatusParam()) {
      if(!_accStatus->isReadable()) {
        throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: status register '" +
            _accStatus->getName() + "' is not readable.");
      }
    }

    assert(NDRegisterAccessor<RegisterRawType>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
    _accWriteDataArea->setDataValidity(NDRegisterAccessor<RegisterRawType>::_dataValidity);
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::doPostWrite(TransferType, VersionNumber) {
    if(!this->_activeException) {
      NDRegisterAccessor<RegisterRawType>::buffer_2D[0].swap(_buffer);
    }
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::mayReplaceOther(
      const boost::shared_ptr<TransferElement const>& other) const {
    auto castedOther = boost::dynamic_pointer_cast<SubdeviceRegisterAccessor const>(other);
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
  bool SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::isReadOnly() const {
    return isReadable() && !isWriteable();
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::isReadable() const {
    return _accReadRequest != nullptr;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  bool SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::isWriteable() const {
    return _accWriteDataArea != nullptr;
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  std::vector<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor<RegisterRawType,
      WriteDataType>::getHardwareAccessingElements() {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }

  /********************************************************************************************************************/

  template<typename RegisterRawType, typename WriteDataType>
  std::list<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor<RegisterRawType,
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
  void SubdeviceRegisterAccessor<RegisterRawType, WriteDataType>::replaceTransferElement(
      boost::shared_ptr<TransferElement>) {
    // Nothing to replace here. The necessary read/write with the handshake cannot be merged with anything.
  }

  /********************************************************************************************************************/
  // Code instantiations for the allowed raw types

  template class SubdeviceRegisterAccessor<uint8_t>;
  template class SubdeviceRegisterAccessor<uint16_t>;
  template class SubdeviceRegisterAccessor<uint32_t>;
  template class SubdeviceRegisterAccessor<uint64_t>;

  template class SubdeviceRegisterAccessor<int8_t, int8_t>;   // backward compatibility
  template class SubdeviceRegisterAccessor<int16_t, int16_t>; // backward compatibility
  template class SubdeviceRegisterAccessor<int32_t, int32_t>; // backward compatibility
  template class SubdeviceRegisterAccessor<int64_t, int64_t>; // backward compatibility

} // namespace ChimeraTK
