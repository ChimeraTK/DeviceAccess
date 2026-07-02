// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "CountedRecursiveMutex.h"
#include "NDRegisterAccessor.h"
#include "NDRegisterAccessorDecorator.h"
#include "RawConverter.h"
#include "SharedAccessor.h"

#include <ChimeraTK/cppext/finally.hpp>

#include <boost/make_shared.hpp>

#include <cassert>
#include <cstdint>
#include <mutex>

namespace ChimeraTK::detail {

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

  template<typename UserType, bool isRaw = false>
  struct BitRangeAccessorDecorator : ChimeraTK::NDRegisterAccessorDecorator<UserType, uint64_t> {
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, uint64_t>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, uint64_t>::_target;

    /******************************************************************************************************************/
    BitRangeAccessorDecorator(const boost::shared_ptr<DeviceBackend>& targetBackend, RegisterPath targetPath,
        boost::shared_ptr<NDRegisterAccessor<uint64_t>> target, const std::string& name, uint64_t shift,
        uint64_t numberOfBits, uint64_t dataInterpretationFractionalBits, uint64_t dataInterpretationIsSigned,
        bool isWriteable)
    : NDRegisterAccessorDecorator<UserType, uint64_t>(target), _shift(shift), _numberOfBits(numberOfBits),
      _writeable(isWriteable), _targetBackend(targetBackend) {
      // Reset the version number. The target accessor may be shared between different decorators (e.g. multiple
      // bit-range registers targeting the same physical register). In that case the target's version number may have
      // been set by operations through another decorator, but from the user's perspective this is a fresh accessor.
      // The test UnifiedBackendTest_B_6 checks this by verifying VersionNumber(nullptr).
      this->_versionNumber = VersionNumber{nullptr};

      // makeConverterLoopHelper expects a NumericAddressedBackend RegisterInfo, which we create with the relevant
      // parameters.
      _registerInfo = NumericAddressedRegisterInfo{name, 1, 0, sizeof(uint64_t), 0, uint32_t(numberOfBits),
          int32_t(dataInterpretationFractionalBits), bool(dataInterpretationIsSigned)};
      _converterLoopHelper =
          RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(_registerInfo, 0, 0, *this);

      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getAsCooked_impl);
      FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(setAsCooked_impl);

      if(_target->getNumberOfChannels() > 1 || _target->getNumberOfSamples() > 1) {
        throw ChimeraTK::logic_error("LogicalNameMappingBackend BitRangeAccessorDecorator: " +
            TransferElement::getName() + ": Cannot target non-scalar registers.");
      }

      _baseBitMask = getMaskForNBits(_numberOfBits);
      _maskOnTarget = _baseBitMask << _shift;

      // Set up shared state via SharedAccessors
      targetPath.setAltSeparator(".");
      detail::SharedAccessorKey key(targetBackend.get(), targetPath);
      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      sharedAccessors.addTransferElement(_target->getId());
      _targetSharedState = sharedAccessors.getTargetSharedState<uint64_t>(key);
      _sharedBuffer =
          std::get_if<SharedAccessors::TargetSharedState::UserBuffer<uint64_t>>(&(_targetSharedState->dataBuffer));
      assert(_sharedBuffer); // getTargetSharedState throws if type mismatch
      _lock = std::unique_lock<detail::CountedRecursiveMutex>(_targetSharedState->mutex, std::defer_lock);
    }

    /******************************************************************************************************************/
    ~BitRangeAccessorDecorator() override {
      auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
      sharedAccessorMutexes.removeTransferElement(_target->getId());
    }

    /******************************************************************************************************************/

    void doPreRead(TransferType type) override {
      _lock.lock();

      // In a transfer group: only swap the shared buffer to the target once, before calling the target's preRead().
      if(_lock.mutex()->useCount() == 1) {
        sharedBufferToTarget();
        _target->preRead(type);
      }
    }

    /******************************************************************************************************************/

    void doPostRead(TransferType type, bool hasNewData) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });

      // step 1: target postRead() and swap the data into the shared state
      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
        // whether the postRead throws or we have new data: The target buffer must be swapped back to keep the shared
        // state intact
        auto swapBack = cppext::finally([this] { targetToSharedBuffer(); });
        _target->postRead(type, hasNewData);
      } // the swapBack finally will be executed with this clothing brace

      if(!hasNewData) {
        return;
      }

      // Step 2: Extract the bit range from the shared buffer into the user buffer via the converter loop
      _converterLoopHelper->doPostRead();
    }

    /******************************************************************************************************************/

    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t implParameter) {
      static_assert(std::is_same_v<UserType, CookedType>);
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        if constexpr(!isRaw) {
          auto validity = _sharedBuffer->dataValidity;
          uint64_t v{_sharedBuffer->value[0][0]};
          v = (v & _maskOnTarget) >> _shift;

          buffer_2D[0][0] = converter.toCooked(v);
          // Do a quick check if the fixed point converter clamped. Then set the
          // data validity faulty according to B.2.4.1
          // For proper implementation of this, the fixed point converter needs to signalize
          // that it had clamped. See https://redmine.msktools.desy.de/issues/12912
          auto raw = converter.toRaw(buffer_2D[0][0]);
          if(raw != v) {
            validity = DataValidity::faulty;
          }

          this->_dataValidity = validity;
        }
        else if constexpr(std::is_integral_v<UserType>) {
          // Raw mode: copy 1:1 from shared buffer to user buffer with no bit manipulation
          buffer_2D[0][0] = static_cast<UserType>(_sharedBuffer->value[0][0]);
          this->_dataValidity = _sharedBuffer->dataValidity;
        }

        this->_versionNumber = std::max(this->_versionNumber, _sharedBuffer->versionNumber);
      }
      else {
        assert(false);
      }
    }

    /******************************************************************************************************************/

    void doPreWrite(TransferType type, VersionNumber versionNumber) override {
      _lock.lock();

      if(!_writeable) {
        throw ChimeraTK::logic_error("Register \"" + _registerInfo.pathName + "\" is not writeable.");
      }

      // Read-Remember-Modify-Write:
      // Only read to the shared state once after opening the device. The content there must not change while the
      // software is connected. Reading always would be a performance penalty and subject to race condition. If the
      // content can really change on the device, the software business logic has to implement the read/modify/write and
      // be aware of the race conditions. It intentionally is not handles in the framework logic.
      if(_target->isReadable() && (_sharedBuffer->versionNumber < _targetBackend->getVersionOnOpen())) {
        sharedBufferToTarget();
        auto swapBack = cppext::finally([this] { targetToSharedBuffer(); });
        _target->read();
      }

      _converterLoopHelper->doPreWrite();

      // After doPreWriteImpl has filled the shared buffer, write it to the target.
      // Only the last accessor in a transfer group does the actual preWrite on the target.
      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
        _temporaryVersion = std::max(versionNumber, _target->getVersionNumber());
        sharedBufferToTarget();
        _target->preWrite(type, _temporaryVersion);
      }
    }

    /******************************************************************************************************************/

    template<class CookedType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<CookedType, RawType, sc, fc, isSigned> converter,
        [[maybe_unused]] size_t implParameter) {
      static_assert(std::is_same_v<UserType, CookedType>);
      if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
        if constexpr(!isRaw) {
          uint64_t value = converter.toRaw(buffer_2D[0][0]);

          // FIXME: Not setting the data validity according to the spec point B2.5.1.
          // This needs a change in the fixedpoint converter to tell us that it has clamped the value to reliably work.
          // To be revisited after fixing https://redmine.msktools.desy.de/issues/12912

          // Modify the bit range in the shared buffer
          _sharedBuffer->value[0][0] &= ~_maskOnTarget;
          _sharedBuffer->value[0][0] |= (value << _shift);
        }
        else if constexpr(std::is_integral_v<UserType>) {
          // Raw mode: copy 1:1 from user buffer to shared buffer with no bit manipulation
          _sharedBuffer->value[0][0] = static_cast<uint64_t>(buffer_2D[0][0]);
        }

        _sharedBuffer->versionNumber = std::max(this->_versionNumber, _sharedBuffer->versionNumber);
        if((_sharedBuffer->dataValidity == DataValidity::ok) || (_lock.mutex()->useCount() == 1)) {
          _sharedBuffer->dataValidity = this->_dataValidity;
        }
      }
      else {
        assert(false);
      }
    }

    /******************************************************************************************************************/

    void doPostWrite(TransferType type, VersionNumber /*versionNumber*/) override {
      auto unlock = cppext::finally([this] { this->_lock.unlock(); });

      auto& sharedAccessors = detail::SharedAccessors::getInstance();
      if(_lock.mutex()->useCount() == sharedAccessors.instanceCount(_target->getId())) {
        // even if postWrite throws: The target buffer must be swapped back to keep the shared state intact
        auto swapBack = cppext::finally([this] { targetToSharedBuffer(); });
        _target->postWrite(type, _temporaryVersion);
      }
    }

    /******************************************************************************************************************/

    template<typename COOKED_TYPE>
    COOKED_TYPE getAsCooked_impl(unsigned int channel, unsigned int sample);

    template<typename COOKED_TYPE>
    void setAsCooked_impl(unsigned int channel, unsigned int sample, COOKED_TYPE value);

    // a local typename so the DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER does
    // not get confused by the comma which separates the two template parameters
    using THIS_TYPE = BitRangeAccessorDecorator<UserType, isRaw>;

    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, getAsCooked_impl, 2);
    DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(THIS_TYPE, setAsCooked_impl, 3);

    /******************************************************************************************************************/

    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override {
      // Always call the non-destructive write. We must be able to swap back the intact buffer into the _sharedState.
      return this->_target->writeTransfer(versionNumber);
    }

    /******************************************************************************************************************/

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> newElement) override {
      auto castedToThisType = boost::dynamic_pointer_cast<BitRangeAccessorDecorator<UserType, isRaw>>(newElement);

      // In a transfer group, we are trying to replaced with an accessor. Check if this accessor is for the
      // same target and not us and check for overlapping bit range afterwards. If they overlap, switch us and
      // the replacement read-only which switches the transfergroup read-only since we cannot guarantee the write
      // order for overlapping bit ranges
      if(castedToThisType && castedToThisType.get() != this && castedToThisType->_target == _target) {
        // anding the two masks will yield 0 iff there is no overlap
        if((castedToThisType->_maskOnTarget & _maskOnTarget) != 0) {
          castedToThisType->_writeable = false;
          _writeable = false;
        }
      }

      auto castedToTargetType = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(newElement);
      if(castedToTargetType && newElement->mayReplaceOther(_target)) {
        if(_target != newElement) {
          auto oldTarget = _target->getId();
          _target = castedToTargetType;
          // Bookkeeping: combine the original target and the replaced target's use count
          auto& sharedAccessorMutexes = detail::SharedAccessors::getInstance();
          sharedAccessorMutexes.combineTransferSharedStates(oldTarget, _target->getId());
        }
      }
      else {
        _target->replaceTransferElement(newElement);
      }
      _target->setExceptionBackend(this->_exceptionBackend);
    }

    /******************************************************************************************************************/

    [[nodiscard]] bool isWriteable() const override { return _writeable; }

    /******************************************************************************************************************/

    [[nodiscard]] bool isReadOnly() const override { return !_writeable; }

    /******************************************************************************************************************/

   private:
    uint64_t _shift;
    uint64_t _numberOfBits;
    uint64_t _maskOnTarget;
    uint64_t _userTypeMask{getMaskForNBits(sizeof(UserType) * CHAR_BIT)};
    uint64_t _targetTypeMask{getMaskForNBits(sizeof(uint64_t) * CHAR_BIT)};
    uint64_t _baseBitMask;

    bool _writeable{false};
    VersionNumber _temporaryVersion;
    NumericAddressedRegisterInfo _registerInfo;
    std::unique_ptr<RawConverter::ConverterLoopHelper> _converterLoopHelper;

    std::shared_ptr<SharedAccessors::TargetSharedState> _targetSharedState;
    std::unique_lock<CountedRecursiveMutex> _lock;
    NDRegisterAccessor<uint64_t>::Buffer* _sharedBuffer{nullptr};
    boost::shared_ptr<DeviceBackend> _targetBackend;

    /******************************************************************************************************************/

    void sharedBufferToTarget() {
      _sharedBuffer->value[0].swap(_target->accessChannel(0));
      _target->setDataValidity(_sharedBuffer->dataValidity);
    }

    /******************************************************************************************************************/

    void targetToSharedBuffer() {
      _sharedBuffer->value[0].swap(_target->accessChannel(0));
      _sharedBuffer->versionNumber = std::max(_sharedBuffer->versionNumber, _target->getVersionNumber());
      _sharedBuffer->dataValidity = _target->dataValidity();
    }
  };

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  COOKED_TYPE BitRangeAccessorDecorator<UserType, isRaw>::getAsCooked_impl(unsigned int channel, unsigned int sample) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      COOKED_TYPE result{};
      uint64_t raw = static_cast<uint64_t>(buffer_2D[channel][sample]);
      // Apply bit shift to extract the actual raw value from the full register value
      raw = (raw & _maskOnTarget) >> _shift;
      RawConverter::withConverter<COOKED_TYPE, uint64_t>(
          _registerInfo, 0, [&](auto converter) { result = converter.toCooked(raw); });
      return result;
    }
    throw ChimeraTK::logic_error("Reading as cooked is not available for this accessor");
  }

  /********************************************************************************************************************/

  template<typename UserType, bool isRaw>
  template<typename COOKED_TYPE>
  void BitRangeAccessorDecorator<UserType, isRaw>::setAsCooked_impl(
      unsigned int channel, unsigned int sample, COOKED_TYPE value) {
    if constexpr(isRaw && std::is_integral_v<UserType>) {
      RawConverter::withConverter<COOKED_TYPE, uint64_t>(_registerInfo, 0, [&](auto converter) {
        // buffer_2D stores the full register word. Perform a read-modify-write on the bit range.
        auto rawField = static_cast<uint64_t>(converter.toRaw(value));
        auto regValue = static_cast<uint64_t>(buffer_2D[channel][sample]);
        regValue &= ~_maskOnTarget;
        regValue |= (rawField << _shift);
        buffer_2D[channel][sample] = static_cast<UserType>(regValue);
      });
      return;
    }
    throw ChimeraTK::logic_error("Setting as cooked is not available for this accessor");
  }

} // namespace ChimeraTK::detail
