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
  using detail::SharedAccessorKey;

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
      _fixedPointConverter(name, _numberOfBits, dataInterpretationFractionalBits, dataInterpretationIsSigned) {
      if(_target->getNumberOfChannels() > 1 || _target->getNumberOfSamples() > 1) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPluginDecorator: " +
            TransferElement::getName() + ": Cannot target non-scalar registers.");
      }

      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      RegisterPath path{name};
      path.setAltSeparator(".");
      detail::SharedAccessorKey key{backend.get(), path};

      // register the target and get the shared accessor state (is created if needed)
      sharedAccessors.addTransferElement(target->getId());
      _targetSharedState = sharedAccessors.getTargetSharedState<TargetType>(key);

      // The buffer in the shared state is a variant. It has to be of TargetType
      _sharedBuffer = std::get_if<detail::SharedAccessors::TargetSharedState::UserBuffer<TargetType>>(
          &(_targetSharedState->dataBuffer));
      assert(_sharedBuffer); // getTargetSharedState throws if the type does not match, so get_if() should always work
      _lock = std::unique_lock<detail::CountedRecursiveMutex>(_targetSharedState->mutex, std::defer_lock);

      _baseBitMask = getMaskForNBits(_numberOfBits);
      _maskOnTarget = _baseBitMask << _shift;
    }

    /******************************************************************************************************************/

    ~BitRangeAccessPluginDecorator() {
      auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
      sharedAccessorMutexes.removeTransferElement(_target->getId());
    }

    /******************************************************************************************************************/

    void doPreRead(TransferType type) override {
      _lock.lock();

      _target->preRead(type);
    }

    /******************************************************************************************************************/

    void doPostRead(TransferType type, bool hasNewData) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postRead(type, hasNewData); // only the first executed postRead() has an effect.
                                           // The target already takes care of this.
      if(!hasNewData) {
        return;
      }
      // update the shared state (only the first call within a transfer group)
      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      // the use count is counting down, so the first call has the full number
      if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
        _sharedBuffer->value[0][0] = _target->accessData(0);
        _sharedBuffer->dataValidity = _target->dataValidity();
        _sharedBuffer->versionNumber = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
      }

      if constexpr(std::is_same_v<uint64_t, TargetType>) {
        auto validity = _sharedBuffer->dataValidity;
        uint64_t v{_sharedBuffer->value[0][0]};
        v = (v & _maskOnTarget) >> _shift;

        buffer_2D[0][0] = _fixedPointConverter.scalarToCooked<UserType>(uint32_t(v));
        // Do a quick check if the fixed point converter clamped. Then set the
        // data validity faulty according to B.2.4.1
        // For proper implementation of this, the fixed point converter needs to signalize
        // that it had clamped. See https://redmine.msktools.desy.de/issues/12912
        auto raw = static_cast<uint32_t>(_fixedPointConverter.toRaw(buffer_2D[0][0]));
        if(raw != v) {
          validity = DataValidity::faulty;
        }

        this->_versionNumber = std::max(this->_versionNumber, _sharedBuffer->versionNumber);
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

      auto value = static_cast<uint32_t>(_fixedPointConverter.toRaw(buffer_2D[0][0]));

      // FIXME: Not setting the data validity according to the spec point B2.5.1.
      // This needs a change in the fixedpoint converter to tell us that it has clamped the value to reliably work.
      // To be revisted after fixing https://redmine.msktools.desy.de/issues/12912

      // When in a transfer group, only the first accessor to write to the _target can call read() in its preWrite()
      // Otherwise it will overwrite the
      if(_target->isReadable() && (!TransferElement::_isInTransferGroup || _lock.mutex()->useCount() == 1)) {
        _target->read();
        _sharedBuffer->value[0][0] = _target->accessData(0);
        _sharedBuffer->dataValidity = _target->dataValidity();
        _sharedBuffer->versionNumber = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
      }

      // update the shared buffer
      _sharedBuffer->value[0][0] &= ~_maskOnTarget;
      _sharedBuffer->value[0][0] |= (value << _shift);
      _sharedBuffer->versionNumber = std::max(versionNumber, _sharedBuffer->versionNumber);
      if((_sharedBuffer->dataValidity == DataValidity::ok) // don't overwrite faulty from previous preWrite()
                                                           // in this transfer
          || (_lock.mutex()->useCount() == 1)) {           // always set for first preWrite()
        _sharedBuffer->dataValidity = this->_dataValidity;
      }

      // only update and write the target for the last merged element in the transfer group
      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
        _temporaryVersion = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
        _target->setDataValidity(_sharedBuffer->dataValidity);
        _target->accessData(0) = _sharedBuffer->value[0][0];
        _target->preWrite(type, _temporaryVersion);
      }
    }

    /******************************************************************************************************************/

    void doPostWrite(TransferType type, VersionNumber /*versionNumber*/) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postWrite(type, _temporaryVersion);
    }

    /******************************************************************************************************************/

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> newElement) override {
      auto casted = boost::dynamic_pointer_cast<BitRangeAccessPluginDecorator<UserType, TargetType>>(newElement);

      // In a transfer group, we are trying to replaced with an accessor. Check if this accessor is for the
      // same target and not us and check for overlapping bit range afterwards. If they overlap, switch us and
      // the replacement read-only which switches the transfergroup read-only since we cannot guarantee the write order
      // for overlapping bit ranges
      if(casted && casted.get() != this && casted->_target == _target) {
        // anding the two masks will yield 0 iff there is no overlap
        if((casted->_maskOnTarget & _maskOnTarget) != 0) {
          casted->_writeable = false;
          _writeable = false;
        }
      }

      auto castedTarget = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<TargetType>>(newElement);
      if(castedTarget && newElement->mayReplaceOther(_target)) {
        if(_target != newElement) {
          // No copy decorator.
          auto oldTarget = _target->getId();
          _target = castedTarget;
          // Bookkeeping: combine the original target and the replaced target's use count
          auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
          sharedAccessorMutexes.combineTransferSharedStates(oldTarget, _target->getId());
          std::cout << "combining is " << oldTarget << "and " << _target->getId() << std::endl;
        }
      }
      else {
        _target->replaceTransferElement(newElement);
      }
      _target->setExceptionBackend(this->_exceptionBackend);
    }

    /******************************************************************************************************************/

   protected:
    uint64_t _shift;
    uint64_t _numberOfBits;
    uint64_t _maskOnTarget;
    uint64_t _userTypeMask{getMaskForNBits(sizeof(UserType) * CHAR_BIT)};
    uint64_t _targetTypeMask{getMaskForNBits(sizeof(TargetType) * CHAR_BIT)};
    uint64_t _baseBitMask;

    std::shared_ptr<detail::SharedAccessors::TargetSharedState>
        _targetSharedState; // we must hold an instance of the shared state to safely hold a lock to the mutex within,
                            // and a pointer to the shared buffer
    std::unique_lock<detail::CountedRecursiveMutex> _lock;
    NDRegisterAccessor<TargetType>::Buffer* _sharedBuffer{nullptr};

    VersionNumber _temporaryVersion;
    bool _writeable{false};
    FixedPointConverter<DEPRECATED_FIXEDPOINT_DEFAULT> _fixedPointConverter;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>::_target;
  };

  /********************************************************************************************************************/

  BitRangeAccessPlugin::BitRangeAccessPlugin(
      const LNMBackendRegisterInfo& info, size_t pluginIndex, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<BitRangeAccessPlugin>(info, pluginIndex) {
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
