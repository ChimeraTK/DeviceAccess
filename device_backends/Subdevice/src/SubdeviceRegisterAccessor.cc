#include "SubdeviceRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  SubdeviceRegisterAccessor::SubdeviceRegisterAccessor(boost::shared_ptr<SubdeviceBackend> backend,
      const std::string& registerPathName, boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
      boost::shared_ptr<NDRegisterAccessor<int32_t>> accData, boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus,
      size_t byteOffset, size_t numberOfWords)
  : SyncNDRegisterAccessor<int32_t>(registerPathName), _backend(backend), _accAddress(accAddress), _accData(accData),
    _accStatus(accStatus), _byteOffset(byteOffset), _numberOfWords(numberOfWords) {
    NDRegisterAccessor<int32_t>::buffer_2D.resize(1);
    NDRegisterAccessor<int32_t>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /*********************************************************************************************************************/

  SubdeviceRegisterAccessor::~SubdeviceRegisterAccessor() { shutdown(); }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doReadTransfer() {
//    throw ChimeraTK::logic_error("Reading this register is not supported.");
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

  bool SubdeviceRegisterAccessor::doReadTransferNonBlocking() {
    doReadTransfer();
    return true;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::doReadTransferLatest() {
    doReadTransfer();
    return true;
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreRead(TransferType) {
    throw ChimeraTK::logic_error("Reading this register is not supported.");
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostRead(TransferType, bool hasNewData) {
    if(!hasNewData) return;
    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreWrite(TransferType) {
    assert(NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size());
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostWrite(TransferType, bool /*dataLost*/) { NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer); }

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

  AccessModeFlags SubdeviceRegisterAccessor::getAccessModeFlags() const { return {AccessMode::raw}; }

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
