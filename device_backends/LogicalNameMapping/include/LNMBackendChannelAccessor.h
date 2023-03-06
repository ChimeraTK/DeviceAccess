// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Device.h"
#include "LogicalNameMappingBackend.h"
#include "NDRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"

#include <algorithm>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendChannelAccessor : public NDRegisterAccessor<UserType> {
   public:
    LNMBackendChannelAccessor(const boost::shared_ptr<DeviceBackend>& dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : NDRegisterAccessor<UserType>(registerPathName, flags), _registerPathName(registerPathName) {
      // check for unknown flags
      flags.checkForUnknownFlags({AccessMode::wait_for_new_data});

      // FIXME: use right type in constructor argument...
      _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);

      // copy the register info and create the internal accessors, if needed
      _info = _dev->_catalogue_mutable.getBackendRegister(_registerPathName);

      // check for incorrect usage of this accessor
      assert(_info.targetType == LNMBackendRegisterInfo::TargetType::CHANNEL);

      // get target device and accessor
      std::string devName = _info.deviceName;
      boost::shared_ptr<DeviceBackend> targetDevice;
      if(devName != "this") {
        targetDevice = _dev->_devices[devName];
      }
      else {
        targetDevice = dev;
      }
      _accessor = targetDevice->getRegisterAccessor<UserType>(
          RegisterPath(_info.registerName), numberOfWords, wordOffsetInRegister, flags);

      // verify channel number
      if(_info.channel >= _accessor->getNumberOfChannels()) {
        throw ChimeraTK::logic_error("LNMBackendChannelAccessor: Requested channel number " +
            std::to_string(_info.channel) +
            " exceeds number of channels of target register,"
            " in accesor for register '" +
            registerPathName + "'.");
      }

      // allocate the buffer
      NDRegisterAccessor<UserType>::buffer_2D.resize(1);
      NDRegisterAccessor<UserType>::buffer_2D[0].resize(_accessor->getNumberOfSamples());

      // set readQueue
      TransferElement::_readQueue = _accessor->getReadQueue();
    }

    void doReadTransferSynchronously() override { _accessor->readTransfer(); }

    bool doWriteTransfer(ChimeraTK::VersionNumber /*versionNumber*/) override {
      assert(false); // writing not allowed
      return true;
    }

    void doPreWrite(TransferType type, VersionNumber) override {
      std::ignore = type;
      throw ChimeraTK::logic_error("Writing to channel-type registers of logical "
                                   "name mapping devices is not supported.");
    }

    void doPreRead(TransferType type) override { _accessor->preRead(type); }

    void doPostRead(TransferType type, bool hasNewData) override {
      _accessor->postRead(type, hasNewData);
      if(!hasNewData) return;
      _accessor->accessChannel(_info.channel).swap(NDRegisterAccessor<UserType>::buffer_2D[0]);
      this->_versionNumber = _accessor->getVersionNumber();
      this->_dataValidity = _accessor->dataValidity();
    }

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const LNMBackendChannelAccessor<UserType>>(other);
      if(!rhsCasted) return false;
      if(_registerPathName != rhsCasted->_registerPathName) return false;
      if(_dev != rhsCasted->_dev) return false;
      return true;
    }

    [[nodiscard]] bool isReadOnly() const override { return true; }

    [[nodiscard]] bool isReadable() const override { return true; }

    [[nodiscard]] bool isWriteable() const override { return false; }

    void setExceptionBackend(boost::shared_ptr<DeviceBackend> exceptionBackend) override {
      this->_exceptionBackend = exceptionBackend;
      _accessor->setExceptionBackend(exceptionBackend);
    }

    void interrupt() override { _accessor->interrupt(); }

   protected:
    /// pointer to underlying accessor
    boost::shared_ptr<NDRegisterAccessor<UserType>> _accessor;

    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information
    LNMBackendRegisterInfo _info;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _accessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      auto result = _accessor->getInternalElements();
      result.push_front(_accessor);
      return result;
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<NDRegisterAccessor<UserType>>(newElement);
      if(casted && _accessor->mayReplaceOther(newElement)) {
        _accessor = casted;
      }
      else {
        _accessor->replaceTransferElement(newElement);
      }
      _accessor->setExceptionBackend(this->_exceptionBackend);
    }
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendChannelAccessor);

} // namespace ChimeraTK
