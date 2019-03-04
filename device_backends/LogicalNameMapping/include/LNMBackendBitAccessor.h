/*
 * LNMBackendBufferingChannelAccessor.h
 *
 *  Created on: Feb 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERA_TK_LNM_BACKEND_BIT_ACCESSOR_H
#define CHIMERA_TK_LNM_BACKEND_BIT_ACCESSOR_H

#include <algorithm>

#include "Device.h"
#include "FixedPointConverter.h"
#include "LogicalNameMappingBackend.h"
#include "SyncNDRegisterAccessor.h"
#include "TwoDRegisterAccessor.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendBitAccessor : public SyncNDRegisterAccessor<UserType> {
   public:
    LNMBackendBitAccessor(boost::shared_ptr<DeviceBackend> dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags)
    : SyncNDRegisterAccessor<UserType>(registerPathName), _registerPathName(registerPathName),
      _fixedPointConverter(registerPathName, 32, 0, 1) {
      try {
        // check for unknown flags
        flags.checkForUnknownFlags({AccessMode::raw});
        // check for illegal parameter combinations
        if(flags.has(AccessMode::raw)) {
          throw ChimeraTK::logic_error("LNMBackendBitAccessor: raw access not supported!");
        }
        _dev = boost::dynamic_pointer_cast<LogicalNameMappingBackend>(dev);
        // copy the register info and create the internal accessors, if needed
        _info = *(boost::static_pointer_cast<LNMBackendRegisterInfo>(
            _dev->getRegisterCatalogue().getRegister(_registerPathName)));
        // check for incorrect usage of this accessor
        if(_info.targetType != LNMBackendRegisterInfo::TargetType::BIT) {
          throw ChimeraTK::logic_error("LNMBackendBitAccessor used for wrong "
                                       "register type."); // LCOV_EXCL_LINE
                                                          // (impossible to
                                                          // test...)
        }
        // get target device and accessor
        std::string devName = _info.deviceName;
        boost::shared_ptr<DeviceBackend> targetDevice;
        if(devName != "this") {
          targetDevice = _dev->_devices[devName];
        }
        else {
          targetDevice = dev;
        }
        _accessor = targetDevice->getRegisterAccessor<uint64_t>(
            RegisterPath(_info.registerName), numberOfWords, wordOffsetInRegister, false);
        // allocate and initialise the buffer
        NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0].resize(1);
        NDRegisterAccessor<UserType>::buffer_2D[0][0] = _fixedPointConverter.toCooked<UserType>(false);
        // set the bit mask
        _bitMask = 1 << _info.bit;
      }
      catch(...) {
        this->shutdown();
        throw;
      }
    }

    virtual ~LNMBackendBitAccessor() { this->shutdown(); };

    void doReadTransfer() override { _accessor->doReadTransfer(); }

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber = {}) override {
      return _accessor->doWriteTransfer(versionNumber);
    }

    bool doReadTransferNonBlocking() override { return doReadTransferNonBlocking(); }

    bool doReadTransferLatest() override { return doReadTransferLatest(); }

    void doPreRead() override { _accessor->preRead(); }

    void doPostRead() override {
      _accessor->postRead();
      if(_accessor->accessData(0) & _bitMask) {
        NDRegisterAccessor<UserType>::buffer_2D[0][0] = _fixedPointConverter.toCooked<UserType>(true);
      }
      else {
        NDRegisterAccessor<UserType>::buffer_2D[0][0] = _fixedPointConverter.toCooked<UserType>(false);
      }
    }

    void doPreWrite() override {
      _accessor->readLatest();
      if(!_fixedPointConverter.toRaw<UserType>(NDRegisterAccessor<UserType>::buffer_2D[0][0])) {
        _accessor->accessData(0) &= ~(_bitMask);
      }
      else {
        _accessor->accessData(0) |= _bitMask;
      }
      _accessor->preWrite();
    }

    void doPostWrite() override { _accessor->postWrite(); }

    bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const LNMBackendBitAccessor<UserType>>(other);
      if(!rhsCasted) return false;
      if(_registerPathName != rhsCasted->_registerPathName) return false;
      if(_dev != rhsCasted->_dev) return false;
      return true;
    }

    bool isReadOnly() const override { return _accessor->isReadOnly(); }

    bool isReadable() const override { return _accessor->isReadable(); }

    bool isWriteable() const override { return _accessor->isWriteable(); }

    AccessModeFlags getAccessModeFlags() const override { return _accessor->getAccessModeFlags(); }

    VersionNumber getVersionNumber() const override { return _accessor->getVersionNumber(); }

   protected:
    /// pointer to underlying accessor
    boost::shared_ptr<NDRegisterAccessor<uint64_t>> _accessor;

    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information. We hold a copy of the RegisterInfo, since it might
    /// contain register accessors which may not be owned by the backend
    LNMBackendRegisterInfo _info;

    /// fixed point converter to handle type conversions from our "raw" type int
    /// to the requested user type. Note: no actual fixed point conversion is
    /// done, it is just used for the type conversion!
    FixedPointConverter _fixedPointConverter;

    /// bit mask for the bit we want to access
    size_t _bitMask;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return _accessor->getHardwareAccessingElements();
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override {
      auto result = _accessor->getInternalElements();
      result.push_front(_accessor);
      return result;
    }

    void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<NDRegisterAccessor<uint64_t>>(newElement);
      if(casted && _accessor->mayReplaceOther(newElement)) {
        _accessor = casted;
      }
      else {
        _accessor->replaceTransferElement(newElement);
      }
    }
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendBitAccessor);

} // namespace ChimeraTK

#endif /* CHIMERA_TK_LNM_BACKEND_BIT_ACCESSOR_H */
