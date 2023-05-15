// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "LNMDoubleBufferPlugin.h"

#include "BackendFactory.h"

namespace ChimeraTK::LNMBackend {
  DoubleBufferPlugin::DoubleBufferPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, std::map<std::string, std::string> parameters)
  : AccessorPlugin(info, pluginIndex), _parameters(std::move(parameters)) {
    if(info.targetType == LNMBackendRegisterInfo::CHANNEL) {
      // we do not support redirectedChannel with doubleBuffer because it has no benefit and will cause
      // trouble in interplay with TransferGroup
      throw logic_error(
          "doubleBuffer plugin not supported for redirectedChannel! use it with redirectedRegister instead");
    }
    _targetDeviceName = info.deviceName;

    // We need to share _readerCount state among instances of DoubleBufferPlugin, if they refer
    // to the same control register.
    static std::map<std::string, boost::shared_ptr<ReaderCount>> readerCountMap;
    static std::mutex readerCountMapMutex;
    std::string id;
    try {
      id = _parameters.at("enableDoubleBuffering");
    }
    catch(std::out_of_range& ex) { // not relevant here since handled later anyway
    }
    std::lock_guard lg(readerCountMapMutex);
    if(readerCountMap.find(id) == readerCountMap.end()) {
      readerCountMap[id] = boost::make_shared<ReaderCount>();
    }
    _readerCount = readerCountMap[id];
  }

  void DoubleBufferPlugin::doRegisterInfoUpdate() {
    // Change register info to read-only
    _info.writeable = false;
    _info.supportedFlags.remove(AccessMode::raw);
    // also remove raw-type info from DataDescriptor
    _info._dataDescriptor.setRawDataType(DataType::none);
  }

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> DoubleBufferPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams& accessorParams) const {
    if constexpr(std::is_same<UserType, TargetType>::value) {
      return boost::make_shared<DoubleBufferAccessorDecorator<UserType>>(backend, target, *this, accessorParams);
    }
    assert(false);
    return {};
  }

  template<typename UserType>
  DoubleBufferAccessorDecorator<UserType>::DoubleBufferAccessorDecorator(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<UserType>>& target,
      const DoubleBufferPlugin& plugin, const UndecoratedParams& accessorParams)
  : ChimeraTK::NDRegisterAccessorDecorator<UserType>(target), _plugin(plugin) {
    boost::shared_ptr<DeviceBackend> dev;
    const auto& parameters = plugin._parameters;
    try {
      if(plugin._targetDeviceName != "this") {
        dev = backend->_devices.at(plugin._targetDeviceName);
      }
      else {
        dev = backend;
      }
    }
    catch(std::out_of_range& ex) {
      std::string message = "LogicalNameMappingBackend DoubleBufferPlugin: unknown targetDevice " + std::string("'") +
          plugin._targetDeviceName + "'.";
      throw ChimeraTK::logic_error(message);
    }

    size_t daqNumber = 0;
    // parameter daqNumber is optional, defines the array index offset in the control & status registers
    if(parameters.find("daqNumber") != parameters.end()) {
      try {
        daqNumber = std::stoul(parameters.at("daqNumber"));
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend DoubleBufferPlugin: parameter 'daqNumber' must be integer");
      }
    }
    // FIXME - remove testUSleep feature
    if(parameters.find("testUSleep") != parameters.end()) {
      try {
        _testUSleep = std::stoul(parameters.at("testUSleep"));
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend DoubleBufferPlugin: parameter 'testUSleep' must be integer");
      }
    }

    std::string key; // store key searched in 'parameters' map in order to print later correctly exception message
    try {
      _enableDoubleBufferReg =
          dev->getRegisterAccessor<uint32_t>(parameters.at(key.assign("enableDoubleBuffering")), 1, daqNumber, {});
      _currentBufferNumberReg =
          dev->getRegisterAccessor<uint32_t>(parameters.at(key.assign("currentBufferNumber")), 1, daqNumber, {});
      std::string secondBufName = parameters.at(key.assign("secondBuffer"));

      // take over the offset/numWords of this logical register, also for second buffer
      // we need to combine it with user-requested offset and lengths, of getRegisterAccessor()
      size_t offset = size_t(_plugin._info.firstIndex) + accessorParams._wordOffsetInRegister;
      size_t numWords =
          (accessorParams._numberOfWords > 0) ? accessorParams._numberOfWords : size_t(_plugin._info.length);
      auto flags = accessorParams._flags;
      _secondBufferReg = dev->getRegisterAccessor<UserType>(secondBufName, numWords, offset, flags);
    }
    catch(std::out_of_range& ex) {
      std::string message =
          "LogicalNameMappingBackend DoubleBufferPlugin: Missing parameter " + std::string("'") + key + "'.";
      throw ChimeraTK::logic_error(message);
    }
    if(_secondBufferReg->getNumberOfChannels() != _target->getNumberOfChannels()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend DoubleBufferPlugin: shapes of first and second buffer do "
                                   "not match, different number of channels");
    }
    if(_secondBufferReg->getNumberOfSamples() != _target->getNumberOfSamples()) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend DoubleBufferPlugin: shapes of first and second buffer do "
                                   "not match, different number of samples");
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPreRead(TransferType type) {
    {
      std::lock_guard lg{_plugin._readerCount->mutex};
      _plugin._readerCount->value++;

      // acquire a lock in firmware (disable buffer swapping)
      _enableDoubleBufferReg->accessData(0) = 0;
      _enableDoubleBufferReg->write();
    }
    if(_testUSleep) {
      // for testing, extra sleep
      // FIXME - remove testUSleep feature
      boost::this_thread::sleep_for(boost::chrono::microseconds{_testUSleep});
    }

    // check which buffer is now in use by the firmware
    _currentBufferNumberReg->read();
    _currentBuffer = _currentBufferNumberReg->accessData(0);
    // if current buffer 1, it means firmware writes now to buffer1, so use target (buffer 0), else use
    // _secondBufferReg (buffer 1)
    if(_currentBuffer) {
      _target->preRead(type);
    }
    else {
      _secondBufferReg->preRead(type);
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doReadTransferSynchronously() {
    if(_currentBuffer) {
      _target->readTransfer();
    }
    else {
      _secondBufferReg->readTransfer();
    }
  }

  template<typename UserType>
  void DoubleBufferAccessorDecorator<UserType>::doPostRead(TransferType type, bool hasNewData) {
    if(_currentBuffer) {
      _target->postRead(type, hasNewData);
    }
    else {
      _secondBufferReg->postRead(type, hasNewData);
    }

    {
      std::lock_guard lg{_plugin._readerCount->mutex};
      assert(_plugin._readerCount->value > 0);
      _plugin._readerCount->value--;
      if(_plugin._readerCount->value == 0) {
        if(_testUSleep) {
          // for testing, check safety of handshake
          // FIXME - remove testUSleep feature
          _currentBufferNumberReg->read();
          if(_currentBuffer != _currentBufferNumberReg->accessData(0)) {
            std::cout << "WARNING: buffer switch happened while reading! Expect corrupted data." << std::endl;
          }
        }
        // release a lock in firmware (enable buffer swapping)
        _enableDoubleBufferReg->accessData(0) = 1;
        _enableDoubleBufferReg->write();
      }
    }
    // set version and data validity of this object
    this->_versionNumber = {};
    if(_currentBuffer) {
      this->_dataValidity = _target->dataValidity();
    }
    else {
      this->_dataValidity = _secondBufferReg->dataValidity();
    }

    if(!hasNewData) return;

    if(_currentBuffer) {
      for(size_t i = 0; i < _target->getNumberOfChannels(); ++i) {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_target->accessChannel(i));
      }
    }
    else {
      for(size_t i = 0; i < _secondBufferReg->getNumberOfChannels(); ++i) {
        ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D[i].swap(_secondBufferReg->accessChannel(i));
      }
    }
  }

  template<typename UserType>
  std::vector<boost::shared_ptr<TransferElement>> DoubleBufferAccessorDecorator<
      UserType>::getHardwareAccessingElements() {
    // returning only this means the DoubleBufferAccessorDecorator will not be optimized when put into TransferGroup
    // optimizing would break our handshake protocol, since it reorders transfers
    return {TransferElement::shared_from_this()};
  }

  template<typename UserType>
  bool DoubleBufferAccessorDecorator<UserType>::mayReplaceOther(
      const boost::shared_ptr<const TransferElement>& other) const {
    // we need this to support merging of accessors using the same double-buffered as target.
    // If other is also double-buffered region belonging to the same plugin instance, allow the merge
    auto otherDoubleBuffer = boost::dynamic_pointer_cast<DoubleBufferAccessorDecorator const>(other);
    if(!otherDoubleBuffer) {
      return false;
    }
    return &(otherDoubleBuffer->_plugin) == &_plugin;
  }
} // namespace ChimeraTK::LNMBackend
