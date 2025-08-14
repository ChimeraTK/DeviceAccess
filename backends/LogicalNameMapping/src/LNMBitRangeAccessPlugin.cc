// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FixedPointConverter.h"
#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"

#include <boost/make_shared.hpp>

#include <charconv>

namespace ChimeraTK::LNMBackend {

  struct RecursiveOwnerCountingMutex {
    RecursiveOwnerCountingMutex() = default;

    RecursiveOwnerCountingMutex(std::recursive_mutex* theMutex, int* useCounter)
    : mutex(theMutex), targetUseCount(useCounter) {}

    void lock() {
      assert(mutex != nullptr);
      mutex->lock();
      *targetUseCount = *targetUseCount + 1;
      ownsLock = true;
    }

    void unlock() {
      assert(mutex != nullptr);
      *targetUseCount = *targetUseCount - 1;
      ownsLock = *targetUseCount > 0;
      mutex->lock();
    }

    [[nodiscard]] int useCount() const {
      assert(ownsLock);
      return *targetUseCount;
    }

    std::recursive_mutex* mutex{nullptr};
    int* targetUseCount{nullptr};
    bool ownsLock{false};
  };

  /********************************************************************************************************************/

  // From https://stackoverflow.com/questions/1392059/algorithm-to-generate-bit-mask
  constexpr uint64_t getMaskForNBits(uint64_t numberOfBits) {
    // Prevent warning about undefined behavior if shifting right by 64 bit below
    if(numberOfBits == 0) {
      return 0;
    }

    return (static_cast<uint64_t>(-(numberOfBits != 0)) &
        (static_cast<uint64_t>(-1) >> ((sizeof(uint64_t) * CHAR_BIT) - numberOfBits)));
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  struct BitRangeAccessPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>::buffer_2D;

    /******************************************************************************************************************/
    BitRangeAccessPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetType>>& target, const std::string& name,
        uint64_t shift, uint64_t numberOfBits, uint64_t dataInterpretationFractionalBits,
        uint64_t dataInterpretationIsSigned)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>(target), _shift(shift), _numberOfBits(numberOfBits),
      _writeable{_target->isWriteable()},
      fixedPointConverter(name, _numberOfBits, dataInterpretationFractionalBits, dataInterpretationIsSigned) {
      if(_target->getNumberOfChannels() > 1 || _target->getNumberOfSamples() > 1) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPluginDecorator: " +
            TransferElement::getName() + ": Cannot target non-scalar registers.");
      }

      auto& map = boost::fusion::at_key<TargetType>(backend->sharedAccessorMap.table);
      RegisterPath path{name};
      path.setAltSeparator(".");
      LogicalNameMappingBackend::AccessorKey key{backend.get(), path};

      auto it = map.find(key);
      if(it != map.end()) {
        _lock = {&(it->second.mutex), &(it->second.useCount)};
      }
      else {
        assert(false);
      }

      _baseBitMask = getMaskForNBits(_numberOfBits);
      _maskOnTarget = _baseBitMask << _shift;
    }

    /******************************************************************************************************************/

    void doPreRead(TransferType type) override {
      _lock.lock();

      _target->preRead(type);
    }

    /******************************************************************************************************************/

    void doPostRead(TransferType type, bool hasNewData) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postRead(type, hasNewData);
      if(!hasNewData) return;

      if constexpr(std::is_same_v<uint64_t, TargetType>) {
        auto validity = _target->dataValidity();
        uint64_t v{_target->accessData(0)};
        v = (v & _maskOnTarget) >> _shift;

        buffer_2D[0][0] = fixedPointConverter.scalarToCooked<UserType>(uint32_t(v));
        // Do a quick check if the fixed point converter clamped. Then set the
        // data validity faulty according to B.2.4.1
        // For proper implementation of this, the fixed point converter needs to signalize
        // that it had clamped. See https://redmine.msktools.desy.de/issues/12912
        auto raw = fixedPointConverter.toRaw(buffer_2D[0][0]);
        if(raw != v) {
          validity = DataValidity::faulty;
        }

        this->_versionNumber = std::max(this->_versionNumber, _target->getVersionNumber());
        this->_dataValidity = validity;
      }
      else {
        // This code should never be reached so long the the bit range plugin requires its target type to be uint64
        assert(false);
      }
    }

    /******************************************************************************************************************/

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      _lock.lock();

      if(!_writeable) {
        throw ChimeraTK::logic_error(
            "Register \"" + TransferElement::getName() + "\" with BitRange plugin is not writeable.");
      }

      auto value = fixedPointConverter.toRaw(buffer_2D[0][0]);

      // FIXME: Not setting the data validity according to the spec point B2.5.1.
      // This needs a change in the fixedpoint converter to tell us that it has clamped the value to reliably work.
      // To be revisted after fixing https://redmine.msktools.desy.de/issues/12912

      // When in a transfer group, only the first accessor to write to the _target can call read() in its preWrite()
      // Otherwise it will overwrite the
      if(_target->isReadable() && (!TransferElement::_isInTransferGroup || _lock.useCount() == 1)) {
        _target->read();
      }

      _target->accessData(0) &= ~_maskOnTarget;
      _target->accessData(0) |= (value << _shift);

      _temporaryVersion = std::max(versionNumber, _target->getVersionNumber());
      _target->setDataValidity(this->_dataValidity);
      _target->preWrite(type, _temporaryVersion);
    }

    /******************************************************************************************************************/

    void doPostWrite(TransferType type, VersionNumber /*versionNumber*/) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postWrite(type, _temporaryVersion);
    }

    /******************************************************************************************************************/

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<BitRangeAccessPluginDecorator<UserType, TargetType>>(newElement);

      // In a transfer group, we are trying to be replaced with an accessor. Check if this accessor is for the
      // same target and not us and check for overlapping bit range afterwards. If they overlap, switch us and
      // the replacement read-only which switches the transfergroup read-only since we cannot guarantee the write order
      // for overlapping bit ranges
      if(casted && casted.get() != this && casted->_target == _target) {
        if((casted->_maskOnTarget & _maskOnTarget) != 0) {
          casted->_writeable = false;
          _writeable = false;
        }
      }
      NDRegisterAccessorDecorator<UserType, TargetType>::replaceTransferElement(newElement);
    }

    /******************************************************************************************************************/

    uint64_t _shift;
    uint64_t _numberOfBits;
    uint64_t _maskOnTarget;
    uint64_t _userTypeMask{getMaskForNBits(sizeof(UserType) * CHAR_BIT)};
    uint64_t _targetTypeMask{getMaskForNBits(sizeof(TargetType) * CHAR_BIT)};
    uint64_t _baseBitMask;

    RecursiveOwnerCountingMutex _lock;
    VersionNumber _temporaryVersion;
    bool _writeable{false};
    FixedPointConverter fixedPointConverter;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>::_target;
  };

  /********************************************************************************************************************/

  BitRangeAccessPlugin::BitRangeAccessPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<BitRangeAccessPlugin>(info, pluginIndex, true) {
    try {
      const auto& shift = parameters.at("shift");

      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{std::from_chars(shift.data(), shift.data() + shift.size(), _shift)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "shift".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
          R"(: Missing parameter "shift".)");
    }

    try {
      const auto& numberOfBits = parameters.at("numberOfBits");
      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{std::from_chars(numberOfBits.data(), numberOfBits.data() + numberOfBits.size(), _numberOfBits)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "numberOfBits".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
          R"(: Unparseable parameter "numberOfBits".)");
    }

    if(const auto it = parameters.find("fractionalBits"); it != parameters.end()) {
      // This is how you are supposed to use std::from_chars with std::string
      // NOLINTNEXTLINE(unsafe-buffer-usage)
      auto [suffix, ec]{
          std::from_chars(it->second.data(), it->second.data() + it->second.size(), dataInterpretationFractionalBits)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "fractionalBits".)");
      }
    }

    if(const auto it = parameters.find("signed"); it != parameters.end()) {
      std::stringstream ss(it->second);
      Boolean value;
      ss >> value;
      dataInterpretationIsSigned = value;
    }
  }

  /********************************************************************************************************************/

  void BitRangeAccessPlugin::doRegisterInfoUpdate() {
    // We do not support wait_for_new_data with this decorator
    _info.supportedFlags.remove(AccessMode::wait_for_new_data);
    _info.supportedFlags.remove(AccessMode::raw);
    // also remove raw-type info from DataDescriptor
    _info._dataDescriptor.setRawDataType(DataType::none);
  }

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> BitRangeAccessPlugin::decorateAccessor(
      boost::shared_ptr<LogicalNameMappingBackend>& backend, boost::shared_ptr<NDRegisterAccessor<TargetType>>& target,
      const UndecoratedParams& params) {
    if constexpr(std::is_integral<TargetType>::value) {
      return boost::make_shared<BitRangeAccessPluginDecorator<UserType, TargetType>>(backend, target, params._name,
          _shift, _numberOfBits, dataInterpretationFractionalBits, dataInterpretationIsSigned);
    }

    assert(false);

    return {};
  }

} // namespace ChimeraTK::LNMBackend
