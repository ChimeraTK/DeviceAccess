#include "LNMDoubleBufferPlugin.h"

#include "BackendFactory.h"

std::string getValue(const std::map<std::string, std::string>& m, std::string key) {
  auto it = m.find(key);
  if(it != m.end()) {
    return it->second;
  }
  else {
    std::string message =
        "LogicalNameMappingBackend DoubleBufferPlugin: Missing parameter " + std::string("'") + key + "'.";
    throw ChimeraTK::logic_error(message);
  }
}

namespace ChimeraTK { namespace LNMBackend {
  DoubleBufferPlugin::DoubleBufferPlugin(LNMBackendRegisterInfo info, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info), _parameters(std::move(parameters)) {
    _targetDeviceName = info.deviceName;
  }

  void DoubleBufferPlugin::updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue) {
    // first update the info so we have the latest version from the catalogue.
    _info = catalogue.getBackendRegister(_info.name);
    // Change register info to read-only
    _info.writeable = false;
    catalogue.modifyRegister(_info);
  }

  /*************************************************/
  /** Helper class to implement MultiplierPlugin::decorateAccessor (can later be realised with if constexpr) */
  template<typename UserType, typename TargetType>
  struct DoubleBufferPlugin_Helper {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& /*backend*/, boost::shared_ptr<NDRegisterAccessor<TargetType>>&,
        const std::map<std::string, std::string>&, std::string) {
      assert(false); // only specialisation is valid
      return {};
    }
  };
  template<typename UserType>
  struct DoubleBufferPlugin_Helper<UserType, UserType> {
    static boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
        const std::map<std::string, std::string>& parameters, std::string targetDeviceName) {
      return boost::make_shared<DoubleBufferAccessorDecorator<UserType>>(backend, target, parameters, targetDeviceName);
    }
  };
  /*************************************************/
  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DoubleBufferPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    return DoubleBufferPlugin_Helper<UserType, TargetType>::decorateAccessor(
        backend, target, _parameters, _targetDeviceName);
  }

  template<typename UserType>
  DoubleBufferAccessorDecorator<UserType>::DoubleBufferAccessorDecorator(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      const std::map<std::string, std::string>& parameters, std::string targetDeviceName)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(target) {
    _enableDoubleBufferReg = backend->_devices[targetDeviceName]->getRegisterAccessor<uint32_t>(
        getValue(parameters, "enableDoubleBuffering"), 0, 0, {});
    _currentBufferNumberReg = backend->_devices[targetDeviceName]->getRegisterAccessor<uint32_t>(
        getValue(parameters, "currentBufferNumber"), 0, 0, {});
    _secondBufferReg = backend->_devices[targetDeviceName]->getRegisterAccessor<UserType>(
        getValue(parameters, "secondBuffer"), 0, 0, {});
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    //accquire a lock in firmware (dissable double buffering)
    _enableDoubleBufferReg->accessData(0) = 0;
    _enableDoubleBufferReg->write();
    //check which buffer is now in use by the firmware
    _currentBufferNumberReg->read();
    _currentBuffer = _currentBufferNumberReg->accessData(0);
    // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use _secondBufferReg (buffer 1)
    if(_currentBuffer)
      _target->preRead(type);
    else
      _secondBufferReg->preRead(type);
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doReadTransferSynchronously() {
    if(_currentBuffer)
      _target->readTransfer();
    else
      _secondBufferReg->readTransfer();
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(_currentBuffer)
      _target->postRead(type, hasNewData);
    else
      _secondBufferReg->postRead(type, hasNewData);

    //release a lock in firmware (enable double buffering)
    _enableDoubleBufferReg->accessData(0) = 1;
    _enableDoubleBufferReg->write();
    // set version and data validity of this object
    this->_versionNumber = {};
    if(_currentBuffer)
      this->_dataValidity = _target->dataValidity();
    else
      this->_dataValidity = _secondBufferReg->dataValidity();

    if(!hasNewData) return;

    if(_currentBuffer) {
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i)
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_target->accessChannel(i));
    }
    else {
      for(size_t i = 0; i < _secondBufferReg->getNumberOfChannels(); ++i)
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_secondBufferReg->accessChannel(i));
    }
  }

}} // namespace ChimeraTK::LNMBackend
