// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ReadCoordinator.h"

#include "SupportedUserTypes.h"

#include <cstring>
#include <iterator>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  bool ReadCoordinator::empty() const {
    return _groupIndex.empty();
  }

  /********************************************************************************************************************/

  void ReadCoordinator::run(const std::vector<uint8_t>& rawBuffer) {
    _rawBuffer = &rawBuffer;
    for(auto& entry : _groups) {
      auto& group = *entry.second;
      if(group.consumersEmpty()) {
        continue; // the group lost all its consumers; it stays registered until teardown
      }
      if(group.isRaw) {
        // Raw-mode consumers are copied without conversion: one typed copy per consumer, up to its sample count.
        group.rawCopy(rawBuffer);
      }
      else {
        // One type-erased call per group per read; the typed conversion loop runs inside doPostReadImpl.
        group.converterLoopHelper->doPostRead();
      }
    }
    _rawBuffer = nullptr;
  }

  /********************************************************************************************************************/

  template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void ReadCoordinator::doPostReadImpl(
      RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId) {
    // The groupId identifies the group within _groups. It is valid for the whole lifetime of the coordinator, and the
    // group is a Group<UserType> (the helper was created with the matching UserType). Each consumer's staging buffer
    // is resolved once per read (a wrapping decorator may swap it on every read), so there is no per-sample indirect
    // call in the typed conversion loop.
    // A raw type of ChimeraTK::Void means a void register. Its low-level element has no data bytes, and the conversion
    // is a no-op (Converter::toCooked() returns a default-constructed UserType), so the buffer is not touched and the
    // loop is skipped entirely. This mirrors the guard of the accessor's former in-place loop.
    if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
      auto& group = static_cast<Group<UserType>&>(*_groups.at(groupId));
      const uint8_t* raw = _rawBuffer->data();
      for(auto& consumer : group.consumers) {
        if(consumer.nSamples == 0) {
          continue;
        }
        auto* cooked = consumer.staging.data();
        const auto* rawSample = raw + consumer.byteOffset;
        for(size_t i = 0; i < consumer.nSamples; ++i) {
          RawType temp;
          std::memcpy(&temp, rawSample + i * consumer.stride, sizeof(RawType));
          cooked[i] = converter.toCooked(temp);
        }
      }
    }
  }

  /********************************************************************************************************************/

  template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void ReadCoordinator::doPreWriteImpl(
      RawConverter::Converter<UserType, RawType, sc, fc, isSigned> /*converter*/, size_t /*groupId*/) {
    // The coordinator is read-only; the ConverterLoopHelper interface requires a doPreWrite, which is never invoked.
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ReadCoordinator::Registration<UserType>::Registration(ReadCoordinator* owner,
      const NumericAddressedRegisterInfo& info, bool isRaw, size_t byteOffset, size_t stride, size_t nSamples)
  : _owner(owner), _staging(std::make_unique<std::vector<UserType>>(nSamples)) {
    // Same grouping as the 2D accessor: converter information plus UserType; raw-mode consumers are grouped separately
    // by their UserType and copied without conversion.
    const auto& channel = info.channels[0];
    GroupKey key{channel.dataType, channel.width, channel.nFractionalBits, channel.signedFlag, channel.rawType,
        std::type_index(typeid(UserType)), isRaw};

    // Resolve the group: insert a new one if this key is not yet registered, otherwise reuse the existing group. The
    // consumer stores a reference to the heap-allocated staging vector, whose address never changes even when this
    // registration is moved.
    auto [groupIt, inserted] = _owner->_groupIndex.emplace(key, 0);
    if(inserted) {
      // Give the new group a stable, monotonically increasing groupId. It is never reused, so the implParameter of any
      // group's ConverterLoopHelper stays valid for the group's lifetime.
      _groupId = _owner->_nextGroupId++;
      groupIt->second = _groupId;
      auto group = std::make_unique<Group<UserType>>();
      group->isRaw = isRaw;
      if(!group->isRaw) {
        // The group's typed conversion loop: one ConverterLoopHelper, cloned from the 2D accessor's pattern.
        //
        // The implParameter is the group's stable groupId, so doPostReadImpl resolves the group by identity (no
        // pointer/address-based or position-based lookup).
        // The full register info (including the register name) and channel index 0 are passed on so RawConverter
        // error messages keep the register name and channel for the read path.
        //
        // The general, void-aware factory is used rather than the fixed-raw variant: it handles both
        // FundamentalType::nodata (raw type ChimeraTK::Void) and a UserType of ChimeraTK::Void, so RawConverter.h
        // stays unchanged. It is called directly, without callForRawType: that helper is a no-op for DataType::Void, so
        // a void register would leave the helper unset. The factory's own callWithConverterParams resolves the raw type
        // and covers the void cases, so the helper is always created here.
        group->converterLoopHelper =
            RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(info, 0, _groupId, *owner);
      }
      _owner->_groups.emplace(_groupId, std::move(group));
    }
    else {
      _groupId = groupIt->second;
    }
    auto& group = static_cast<Group<UserType>&>(*_owner->_groups.at(_groupId));
    group.consumers.push_back(Consumer<UserType>(byteOffset, stride, nSamples, *_staging));
    _it = std::prev(group.consumers.end());
  }

  /********************************************************************************************************************/

  template<typename UserType>
  // NOLINTNEXTLINE(clang-diagnostic-dtor-name) - linter is wrong, this is standard-conforming
  ReadCoordinator::Registration<UserType>::~Registration() {
    reset();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ReadCoordinator::Registration<UserType>::Registration(Registration&& other) noexcept
  : _owner(other._owner), _groupId(other._groupId), _it(other._it), _staging(std::move(other._staging)) {
    other._owner = nullptr;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  ReadCoordinator::Registration<UserType>& ReadCoordinator::Registration<UserType>::operator=(
      Registration&& other) noexcept {
    if(this == &other) {
      return *this;
    }
    reset();
    _owner = other._owner;
    _groupId = other._groupId;
    _it = other._it;
    _staging = std::move(other._staging);
    other._owner = nullptr;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void ReadCoordinator::Registration<UserType>::reset() {
    if(_owner == nullptr) {
      return;
    }
    // Only the consumer is removed; the group itself stays registered until teardown, so no group's groupId (and hence
    // no group's ConverterLoopHelper implParameter) can ever be invalidated.
    auto& group = static_cast<Group<UserType>&>(*_owner->_groups.at(_groupId));
    group.consumers.erase(_it);
    _owner = nullptr;
    _staging.reset();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType>& ReadCoordinator::Registration<UserType>::staging() {
    return *_staging;
  }

  /********************************************************************************************************************/

  // The Registration member functions are not performance critical (they only run when consumers are registered or
  // unregistered), so they are not inlined into the class definition but defined here and explicitly instantiated for
  // all supported user types.

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ReadCoordinator::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
