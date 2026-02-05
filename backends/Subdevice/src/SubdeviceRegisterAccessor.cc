// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceRegisterAccessor.h"

#include <utility>

namespace ChimeraTK {

  /********************************************************************************************************************/

  SubdeviceRegisterAccessor::SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend,
      const std::string& registerPathName, boost::shared_ptr<NDRegisterAccessor<int32_t>> accChipSelect,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accWriteDataArea,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus,
      boost::shared_ptr<NDRegisterAccessor<ChimeraTK::Void>> accReadRequest,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accReadData, size_t byteOffset, size_t numberOfWords)
  : NDRegisterAccessor<int32_t>(registerPathName, {AccessMode::raw}), _backend(std::move(backend)),
    _accChipSelect(std::move(accChipSelect)), _accAddress(std::move(accAddress)),
    _accWriteDataArea(std::move(accWriteDataArea)), _accStatus(std::move(accStatus)),
    _accReadRequest(std::move(accReadRequest)), _accReadData(std::move(accReadData)), _startAddress(byteOffset),
    _numberOfWords(numberOfWords) {
    NDRegisterAccessor<int32_t>::buffer_2D.resize(1);
    NDRegisterAccessor<int32_t>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doReadTransferSynchronously() {
    assert(false); // must never be called due to exception in doPreRead
  }

  /********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::doWriteTransfer(ChimeraTK::VersionNumber) {
    std::lock_guard<decltype(_backend->_mutex)> lockGuard(_backend->_mutex);
    size_t nTransfers;
    if(_backend->_type == SubdeviceBackend::Type::areaHandshake) {
      // for areaHandshake case with 1D array
      // TODO FIXME shouldn't this be nTransfers = 1 ???
      nTransfers = _numberOfWords;
    }
    else {
      assert(_backend->_type == SubdeviceBackend::Type::threeRegisters ||
          _backend->_type == SubdeviceBackend::Type::twoRegisters);
      // This is "_numberOfWords / _accData->getNumberOfSamples()" rounded up:
      nTransfers =
          (_numberOfWords + _accWriteDataArea->getNumberOfSamples() - 1) / _accWriteDataArea->getNumberOfSamples();
    }
    try {
      size_t idx = 0;
      for(size_t adr = _startAddress; adr < _startAddress + nTransfers; ++adr) {
        // write address register (if applicable)
        if(_backend->_type != SubdeviceBackend::Type::areaHandshake) {
          _accAddress->accessData(0) = static_cast<int32_t>(adr);
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
            int32_t val = (idx < _numberOfWords) ? _buffer[idx] : 0;
            _accWriteDataArea->accessData(0, innerOffset) = val;
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
              throw ChimeraTK::runtime_error("Write to register '" + _name +
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
      _exceptionBackend->setException(ex.what());
      throw;
    }
    return false;
  }

  /********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreRead(TransferType) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostRead(TransferType, bool) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreWrite(TransferType, VersionNumber) {
    // FIXME: Isn't this check done in TransferElement?
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

    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
    _accWriteDataArea->setDataValidity(this->_dataValidity);
  }

  /********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostWrite(TransferType, VersionNumber) {
    if(!this->_activeException) {
      NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
    }
  }

  /********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const {
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

  bool SubdeviceRegisterAccessor::isReadOnly() const {
    return isReadable() && !isWriteable();
  }

  /********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadable() const {
    return _accReadRequest != nullptr;
  }

  /********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isWriteable() const {
    return _accWriteDataArea != nullptr;
  }

  /********************************************************************************************************************/

  std::vector<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getHardwareAccessingElements() {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }

  /********************************************************************************************************************/

  std::list<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getInternalElements() {
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

  void SubdeviceRegisterAccessor::replaceTransferElement(boost::shared_ptr<TransferElement>) {
    // Nothing to replace here. The necessary read/write with the handshake cannot be merged with anything.
  }

} // namespace ChimeraTK
