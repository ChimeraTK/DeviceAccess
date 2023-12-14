// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SubdeviceRegisterAccessor.h"

#include <utility>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  SubdeviceRegisterAccessor::SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend,
      const std::string& registerPathName, boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accData, boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus,
      size_t byteOffset, size_t numberOfWords)
  : NDRegisterAccessor<int32_t>(registerPathName, {AccessMode::raw}), _backend(std::move(backend)),
    _accAddress(std::move(accAddress)), _accDataArea(std::move(accData)), _accStatus(std::move(accStatus)),
    _startAddress(byteOffset), _numberOfWords(numberOfWords) {
    NDRegisterAccessor<int32_t>::buffer_2D.resize(1);
    NDRegisterAccessor<int32_t>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doReadTransferSynchronously() {
    assert(false); // must never be called due to exception in doPreRead
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::doWriteTransfer(ChimeraTK::VersionNumber) {
    std::lock_guard<decltype(_backend->mutex)> lockGuard(_backend->mutex);
    size_t nTransfers;
    if(_backend->type == SubdeviceBackend::Type::areaHandshake) {
      // for areaHandshake case with 1D array
      // TODO FIXME shouldn't this be nTransfers = 1 ???
      nTransfers = _numberOfWords;
    }
    else {
      assert(_backend->type == SubdeviceBackend::Type::threeRegisters ||
          _backend->type == SubdeviceBackend::Type::twoRegisters);
      // This is "_numberOfWords / _accData->getNumberOfSamples()" rounded up:
      nTransfers = (_numberOfWords + _accDataArea->getNumberOfSamples() - 1) / _accDataArea->getNumberOfSamples();
    }
    try {
      size_t idx = 0;
      for(size_t adr = _startAddress; adr < _startAddress + nTransfers; ++adr) {
        // write address register (if applicable)
        if(_backend->type != SubdeviceBackend::Type::areaHandshake) {
          _accAddress->accessData(0) = static_cast<int32_t>(adr);
          _accAddress->write();
          usleep(_backend->addressToDataDelay);
        }

        // write data register
        if(_backend->type == SubdeviceBackend::Type::areaHandshake) {
          int32_t val = (idx < _numberOfWords) ? _buffer[idx] : 0;
          _accDataArea->accessData(0, idx) = val;
          ++idx;
        }
        else {
          for(size_t innerOffset = 0; innerOffset < _accDataArea->getNumberOfSamples(); ++innerOffset) {
            // pad data with zeros, if _numberOfWords isn't an integer multiple of _accData->getNumberOfSamples()
            int32_t val = (idx < _numberOfWords) ? _buffer[idx] : 0;
            _accDataArea->accessData(0, innerOffset) = val;
            ++idx;
          }
        }
        _accDataArea->write();

        // wait until transaction is complete
        if(_backend->type == SubdeviceBackend::Type::threeRegisters ||
            _backend->type == SubdeviceBackend::Type::areaHandshake) {
          // for 3regs/areaHandshake, wait until status register is 0 again
          size_t retry = 0;
          size_t max_retry = _backend->timeout * 1000 / _backend->sleepTime;
          while(true) {
            usleep(_backend->sleepTime);
            _accStatus->read();
            if(_accStatus->accessData(0) == 0) break;
            if(++retry > max_retry) {
              throw ChimeraTK::runtime_error("Write to register '" + _name +
                  "' failed: timeout waiting for cleared busy flag (" + _accStatus->getName() + ")");
            }
          }
        }
        else {
          // for 2regs, wait given time
          usleep(_backend->sleepTime);
        }
      }
    }
    catch(ChimeraTK::runtime_error& ex) {
      _exceptionBackend->setException(ex.what());
      throw;
    }
    return false;
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreRead(TransferType) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostRead(TransferType, bool) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreWrite(TransferType, VersionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(_accAddress && !_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accDataArea->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: data/area register '" +
          _accDataArea->getName() + "' is not writeable.");
    }
    if(_backend->needStatusParam()) {
      if(!_accStatus->isReadable()) {
        throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: status register '" +
            _accStatus->getName() + "' is not readable.");
      }
    }

    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
    _accDataArea->setDataValidity(this->_dataValidity);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostWrite(TransferType, VersionNumber) {
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const {
    return false;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadOnly() const {
    return false;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadable() const {
    return false;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isWriteable() const {
    return true;
  }

  /*********************************************************************************************************************/

  std::vector<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getHardwareAccessingElements() {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }

  /*********************************************************************************************************************/

  std::list<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getInternalElements() {
    return {_accAddress, _accDataArea, _accStatus};
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::replaceTransferElement(boost::shared_ptr<TransferElement>) {}

} // namespace ChimeraTK
