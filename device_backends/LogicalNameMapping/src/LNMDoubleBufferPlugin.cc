// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMDoubleBufferPlugin.h"

#include "BackendFactory.h"

namespace ChimeraTK { namespace LNMBackend {
  DoubleBufferPlugin::DoubleBufferPlugin(LNMBackendRegisterInfo info, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info), _parameters(std::move(parameters)) {
    _targetDeviceName = info.deviceName;
    _readerCount = boost::make_shared<std::atomic<uint32_t>>(0);
  }

  void DoubleBufferPlugin::updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>& catalogue) {
    // first update the info so we have the latest version from the catalogue.
    _info = catalogue.getBackendRegister(_info.name);
    // Change register info to read-only
    _info.writeable = false;
    _info.supportedFlags.remove(AccessMode::raw);
    auto& dd = _info.getDataDescriptor();
    // also remove raw-type info from DataDescriptor
    // TODO this does not work in general
    // _info._dataDescriptor =
    //    DataDescriptor(dd.fundamentalType(), dd.isIntegral(), dd.isSigned(), dd.nDigits(), dd.nFractionalDigits());
    catalogue.modifyRegister(_info);
  }

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DoubleBufferPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend,
      boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const {
    if constexpr(std::is_same<UserType, TargetType>::value) {
      return boost::make_shared<DoubleBufferAccessorDecorator<UserType>>(
          backend, target, _parameters, _targetDeviceName, _readerCount);
    }
    assert(false);
    return {};
  }

  /*
   *
   * TODO double buffering - some points to discuss and test about this.
   * it looks like the element number (1..3) of the registers
   * WORD_DUB_BUF_ENA, WORD_DUB_BUF_CUR  of
   * https://redmine.msktools.desy.de/projects/utca_firmware_framework/wiki/DAQ_-_Data_Acquisiton_Module
   * are not supported (because we would need offset!=0).
   * Is it important or should we rely on selection in logical name mapping to target right element?
   *
   * Also, tickets say something support for individual channels - but how would that differ from just going
   * over all channels, like here?
   * https://github.com/ChimeraTK/DeviceAccess/issues/30
   *
   * Also, raw accessMode needs to be supported and tested?
   * What about wait_for_new_data, should we support that or not?
   */
  template<typename UserType>
  DoubleBufferAccessorDecorator<UserType>::DoubleBufferAccessorDecorator(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      const std::map<std::string, std::string>& parameters, std::string targetDeviceName,
      boost::shared_ptr<std::atomic<uint32_t>> readerCount)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(target), _readerCount(readerCount) {
    std::string key; // store key searched in 'parameters' map in order to print later correctly exception message
    try {
      _enableDoubleBufferReg = backend->_devices[targetDeviceName]->getRegisterAccessor<uint32_t>(
          parameters.at(key.assign("enableDoubleBuffering")), 0, 0, {});
      _currentBufferNumberReg = backend->_devices[targetDeviceName]->getRegisterAccessor<uint32_t>(
          parameters.at(key.assign("currentBufferNumber")), 0, 0, {});
      _secondBufferReg = backend->_devices[targetDeviceName]->getRegisterAccessor<UserType>(
          parameters.at(key.assign("secondBuffer")), 0, 0, {});
    }
    catch(std::out_of_range& ex) {
      std::string message =
          "LogicalNameMappingBackend DoubleBufferPlugin: Missing parameter " + std::string("'") + key + "'.";
      throw ChimeraTK::logic_error(message);
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    (*_readerCount)++;
    // acquire a lock in firmware (disable buffer swapping)
    _enableDoubleBufferReg->accessData(0) = 0;
    _enableDoubleBufferReg->write();
    // check which buffer is now in use by the firmware
    _currentBufferNumberReg->read();
    _currentBuffer = _currentBufferNumberReg->accessData(0);
    // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use _secondBufferReg
    // (buffer 1)
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

    assert(*_readerCount > 0);
    (*_readerCount)--;
    if(*_readerCount == 0) {
      // release a lock in firmware (enable buffer swapping)
      _enableDoubleBufferReg->accessData(0) = 1;
      _enableDoubleBufferReg->write();
    }
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
