
#include "SubdeviceRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  SubdeviceRegisterAccessor::SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend,
      const std::string& registerPathName, boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accData, boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus,
      size_t byteOffset, size_t numberOfWords)
  : NDRegisterAccessor<int32_t>(registerPathName, {AccessMode::raw}), _backend(backend), _accAddress(accAddress),
    _accData(accData), _accStatus(accStatus), _byteOffset(byteOffset), _numberOfWords(numberOfWords) {
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
    size_t idx = 0;
    for(size_t adr = _byteOffset; adr < _byteOffset + 4 * _numberOfWords; adr += 4) {
      if(_backend->type == SubdeviceBackend::Type::threeRegisters) {
        while(true) {
          _accStatus->read();
          if(_accStatus->accessData(0) == 0) break;
          usleep(_backend->sleepTime);
        }
      }
      else {
        usleep(_backend->sleepTime);
      }
      _accAddress->accessData(0) = adr;
      _accAddress->write();
      _accData->accessData(0) = _buffer[idx];
      _accData->write();
      ++idx;
    }
    return false;
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreRead(TransferType) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostRead(TransferType, bool hasNewData) {
    assert(!hasNewData);
    if(!hasNewData) return;
    // FIXME: Code cleanup. This following code is never executed:
    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
    // dont't care about version number or data validity. This code is nerver executed  anyway.
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreWrite(TransferType, VersionNumber) {
    if(!_backend->isOpen()) {
      throw ChimeraTK::logic_error("Device is not opened.");
    }

    if(!_accAddress->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: address register '" +
          _accAddress->getName() + "' is not writeable.");
    }
    if(!_accData->isWriteable()) {
      throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: data register '" +
          _accData->getName() + "' is not writeable.");
    }
    if(_backend->type == SubdeviceBackend::Type::threeRegisters) {
      if(!_accStatus->isReadable()) {
        throw ChimeraTK::logic_error("SubdeviceRegisterAccessor[" + this->getName() + "]: status register '" +
            _accStatus->getName() + "' is not readable.");
      }
    }

    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
    _accData->setDataValidity(this->_dataValidity);
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

  bool SubdeviceRegisterAccessor::isReadOnly() const { return false; }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadable() const { return false; }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isWriteable() const { return true; }

  /*********************************************************************************************************************/

  std::vector<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getHardwareAccessingElements() {
    return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
  }

  /*********************************************************************************************************************/

  std::list<boost::shared_ptr<TransferElement>> SubdeviceRegisterAccessor::getInternalElements() {
    return {_accAddress, _accData, _accStatus};
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::replaceTransferElement(boost::shared_ptr<TransferElement>) {}

} // namespace ChimeraTK
