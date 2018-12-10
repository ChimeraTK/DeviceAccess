#include "SubdeviceRegisterAccessor.h"

namespace ChimeraTK {

/*********************************************************************************************************************/

  SubdeviceRegisterAccessor::SubdeviceRegisterAccessor(const std::string &registerPathName,
                            boost::shared_ptr<NDRegisterAccessor<int32_t>> accAddress,
                            boost::shared_ptr<NDRegisterAccessor<int32_t>> accData,
                            boost::shared_ptr<NDRegisterAccessor<int32_t>> accStatus,
                            size_t byteOffset, size_t numberOfWords,
                            bool isReadable, bool isWriteable)
  : SyncNDRegisterAccessor<int32_t>(registerPathName),
    _accAddress(accAddress),
    _accData(accData),
    _accStatus(accStatus),
    _byteOffset(byteOffset),
    _numberOfWords(numberOfWords),
    _isReadable(isReadable),
    _isWriteable(isWriteable)
  {
    NDRegisterAccessor<int32_t>::buffer_2D.resize(1);
    NDRegisterAccessor<int32_t>::buffer_2D[0].resize(numberOfWords);
    _buffer.resize(numberOfWords);
  }

  /*********************************************************************************************************************/

  SubdeviceRegisterAccessor::~SubdeviceRegisterAccessor() {shutdown();}

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doReadTransfer() {
    size_t idx = 0;
    for(size_t adr=_byteOffset; adr<_byteOffset+4*_numberOfWords; adr+=4) {
      while(true) {
        _accStatus->read();
        if(_accStatus->accessData(0) == 0) break;
        usleep(1000);             /// @todo make configurable!
      }
      _accAddress->accessData(0) = adr;
      _accAddress->write();
      _accData->read();
      _buffer[idx] = _accData->accessData(0);
      ++idx;
    }
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::doWriteTransfer(ChimeraTK::VersionNumber) {
    size_t idx = 0;
    for(size_t adr=_byteOffset; adr<_byteOffset+4*_numberOfWords; adr+=4) {
      while(true) {
        _accStatus->read();
        if(_accStatus->accessData(0) == 0) break;
        usleep(1000);             /// @todo make configurable!
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

  void SubdeviceRegisterAccessor::doPreRead() {
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostRead() {
    assert( NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size() );
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPreWrite() {
    assert( NDRegisterAccessor<int32_t>::buffer_2D[0].size() == _buffer.size() );
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::doPostWrite() {
    NDRegisterAccessor<int32_t>::buffer_2D[0].swap(_buffer);
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::mayReplaceOther(const boost::shared_ptr<TransferElement const> &) const {
    return false;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadOnly() const {
    return _isReadable && !_isWriteable;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isReadable() const {
    return _isReadable;
  }

  /*********************************************************************************************************************/

  bool SubdeviceRegisterAccessor::isWriteable() const {
    return _isWriteable;
  }

  /*********************************************************************************************************************/

  FixedPointConverter SubdeviceRegisterAccessor::getFixedPointConverter() const {
    throw ChimeraTK::logic_error("FixedPointConverterse are not available in Logical Name Mapping");
  }

  /*********************************************************************************************************************/

  AccessModeFlags SubdeviceRegisterAccessor::getAccessModeFlags() const {
    return {AccessMode::raw};
  }

  /*********************************************************************************************************************/

  std::vector< boost::shared_ptr<TransferElement> > SubdeviceRegisterAccessor::getHardwareAccessingElements() {
    return { boost::enable_shared_from_this<TransferElement>::shared_from_this() };
  }

  /*********************************************************************************************************************/

  std::list< boost::shared_ptr<TransferElement> > SubdeviceRegisterAccessor::getInternalElements() {
    return {_accAddress, _accData, _accStatus};
  }

  /*********************************************************************************************************************/

  void SubdeviceRegisterAccessor::replaceTransferElement(boost::shared_ptr<TransferElement>) {}

}    // namespace ChimeraTK
