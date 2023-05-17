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

  struct ReferenceCountedUniqueLock {
    ReferenceCountedUniqueLock() = default;

    ReferenceCountedUniqueLock(std::recursive_mutex& mutex, int* useCounter)
    : _lock(mutex, std::defer_lock), _targetUseCount(useCounter) {}

    void lock() {
      _lock.lock();
      *_targetUseCount = *_targetUseCount + 1;
    }

    void unlock() {
      *_targetUseCount = *_targetUseCount - 1;
      _lock.unlock();
    }

    int useCount() const {
      assert(_lock.owns_lock());
      return *_targetUseCount;
    }

    std::unique_lock<std::recursive_mutex> _lock;
    int* _targetUseCount{nullptr};
  };

  /********************************************************************************************************************/

  template<typename UserType, typename TargetType>
  struct BitRangeAccessPluginDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>::buffer_2D;

    /********************************************************************************************************************/
    BitRangeAccessPluginDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        const boost::shared_ptr<ChimeraTK::NDRegisterAccessor<TargetType>>& target, uint64_t shift,
        uint64_t numberOfBits)
    : ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>(target), _shift(shift), _numberOfBits(numberOfBits) {
      if(_target->getNumberOfChannels() > 1 || _target->getNumberOfSamples() > 1) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPluginDecorator: " +
            TransferElement::getName() + ": Cannot target non-scalar registers.");
      }

      if(sizeof(UserType) * 8 < _numberOfBits) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPluginDecorator: UserType is too small "
                                     "for configured number of bits");
      }

      auto& map = boost::fusion::at_key<TargetType>(backend->sharedAccessorMap.table);
      RegisterPath path{target->getName()};
      path.setAltSeparator(".");
      LogicalNameMappingBackend::AccessorKey key{backend.get(), path};

      if(auto it = map.find(key); it != map.end()) {
        _lock = {it->second.mutex, &(it->second.useCount)};
      }
      else {
        assert(false);
      }

      _mask = ((1U << _numberOfBits) - 1) << _shift;
    }

    /********************************************************************************************************************/

    void doPreRead(TransferType type) override {
      _lock.lock();

      _target->preRead(type);
    }

    /********************************************************************************************************************/

    void doPostRead(TransferType type, bool hasNewData) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postRead(type, hasNewData);
      if(!hasNewData) return;

      if constexpr(std::is_same<UserType, ChimeraTK::Void>::value) {
        // This should never be reached, the decorate function should make sure that we only have integral types here
        assert(false);
      }
      else if constexpr(std::is_same<UserType, std::string>::value) {
        // This should never be reached, the decorate function should make sure that we only have integral types here
        assert(false);
      }
      else if constexpr(std::is_same<UserType, ChimeraTK::Boolean>::value) {
        assert(false);
      }
      else {
        uint64_t v{};
        auto targetValue = _target->accessData(0);
        memcpy(std::addressof(v), std::addressof(targetValue), sizeof(TargetType));
        v = (v & _mask) >> _shift;
        memcpy(buffer_2D[0].data(), &v, sizeof(UserType));
      }

      this->_versionNumber = {};
      this->_dataValidity = _target->dataValidity();
    }

    /********************************************************************************************************************/

    void doPreWrite(TransferType type, VersionNumber /*versionNumber*/) override {
      _lock.lock();

      if(!_target->isWriteable()) {
        throw ChimeraTK::logic_error("Register \"" + TransferElement::getName() + "\" with BitRange plugin is not writeable.");
      }

      uint64_t value{};
      if constexpr(std::is_same<UserType, ChimeraTK::Void>::value) {
        // This should never be reached, the decorate function should make sure that we only have integral types here
        assert(false);
      }
      else if constexpr(std::is_same<UserType, std::string>::value) {
        // This should never be reached, the decorate function should make sure that we only have integral types here
        assert(false);
      }
      else if constexpr(std::is_same<UserType, ChimeraTK::Boolean>::value) {
        assert(false);
      }
      else {
        memcpy(&value, buffer_2D[0].data(), sizeof(UserType));
      }

      // When in a transfer group, only the first accessor to write to the _target can call read() in its preWrite()
      // Otherwise it will overwrite the
      if(_target->isReadable() && (!TransferElement::_isInTransferGroup || _lock.useCount() == 1)) {
        _target->read();
      }

      _target->accessData(0) &= ~_mask;
      _target->accessData(0) |= (value << _shift);

      _temporaryVersion = {};
      _target->setDataValidity(this->_dataValidity);
      _target->preWrite(type, _temporaryVersion);
    }

    /********************************************************************************************************************/

    void doPostWrite(TransferType type, VersionNumber /*versionNumber*/) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });
      _target->postWrite(type, _temporaryVersion);
    }

    uint64_t _shift;
    uint64_t _numberOfBits;
    uint64_t _mask;

    ReferenceCountedUniqueLock _lock;
    VersionNumber _temporaryVersion;

    using ChimeraTK::NDRegisterAccessorDecorator<UserType, TargetType>::_target;
  };

  /********************************************************************************************************************/

  BitRangeAccessPlugin::BitRangeAccessPlugin(
      const LNMBackendRegisterInfo& info, const std::map<std::string, std::string>& parameters)
  : AccessorPlugin<BitRangeAccessPlugin>(info) {
    _needSharedTarget = true;

    try {
      const auto& shift = parameters.at("shift");
      auto [_, ec]{std::from_chars(shift.data(), shift.data() + shift.size(), _shift)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error(
            "LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() + R"(: Unparseable parameter "shift".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error(
          "LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() + R"(: Missing parameter "shift".)");
    }

    try {
      const auto& numberOfBits = parameters.at("numberOfBits");
      auto [_, ec]{std::from_chars(numberOfBits.data(), numberOfBits.data() + numberOfBits.size(), _numberOfBits)};
      if(ec != std::errc()) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
            R"(: Unparseable parameter "numberOfBits".)");
      }
    }
    catch(std::out_of_range&) {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessPlugin: " + info.getRegisterName() +
          R"(: Missing parameter "numberOfBits".)");
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
      const UndecoratedParams&) {

    if constexpr(std::is_integral<TargetType>::value) {
      return boost::make_shared<BitRangeAccessPluginDecorator<UserType, TargetType>>(
          backend, target, _shift, _numberOfBits);
    }

    assert(false);

    return {};
  }

} // namespace ChimeraTK::LNMBackend
