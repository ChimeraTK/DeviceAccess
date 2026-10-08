// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MuxedChannelDemultiplexer.h"

#include "SupportedUserTypes.h"

#include <cstring>
#include <iterator>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  bool MuxedChannelDemultiplexer::empty() const {
    return _groupIndex.empty();
  }

  /********************************************************************************************************************/

  void MuxedChannelDemultiplexer::run(const std::vector<uint8_t>& rawBuffer) {
    _pendingDemultiplexing = false;
    _rawBuffer = &rawBuffer;
    for(auto& group : _groups) {
      if(!group || group->consumersEmpty()) {
        continue; // skip freed (hole) slots whose index is not currently used
      }
      if(group->isRaw) {
        // Raw slices are copied without conversion: one strided typed copy per consumer, up to its sample count.
        group->rawCopy(rawBuffer);
      }
      else {
        // One type-erased call per group per read; the typed conversion loop runs inside doPostReadImpl.
        group->converterLoopHelper->doPostRead();
      }
    }
    _rawBuffer = nullptr;
  }

  /********************************************************************************************************************/

  template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void MuxedChannelDemultiplexer::doPostReadImpl(
      RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId) {
    // The groupId is the group's index within _groups, see the Registration constructor. It is valid as long as the
    // ConverterLoopHelper exists, which is exactly when this method is invoked, and the group is then a Group<UserType>
    // (the helper was created with the matching user type). Each consumer's staging buffer is resolved once per read (a
    // wrapping decorator may swap it on every read), so there is no per-sample indirect call in the typed conversion
    // loop.
    auto& group = static_cast<Group<UserType>&>(*_groups[groupId]);
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

  /********************************************************************************************************************/

  template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void MuxedChannelDemultiplexer::doPreWriteImpl(
      RawConverter::Converter<UserType, RawType, sc, fc, isSigned> /*converter*/, size_t /*groupId*/) {
    // The demultiplexer is read-only; the ConverterLoopHelper interface requires a doPreWrite, which is never invoked.
  }

  /********************************************************************************************************************/

  template<typename UserType>
  MuxedChannelDemultiplexer::Registration<UserType>::Registration(MuxedChannelDemultiplexer* owner,
      const NumericAddressedRegisterInfo& info, bool isRaw, size_t byteOffset, size_t stride, size_t nSamples)
  : _owner(owner), _staging(std::make_unique<std::vector<UserType>>(nSamples)) {
    // Same grouping as the 2D accessor: converter information plus user type; raw slices are grouped separately by
    // their user type and copied without conversion.
    const auto& channel = info.channels[0];
    GroupKey key{channel.dataType, channel.width, channel.nFractionalBits, channel.signedFlag, channel.rawType,
        std::type_index(typeid(UserType)), isRaw};

    // Resolve the group: insert a new one if this key is not yet registered, otherwise reuse the existing group. The
    // consumer stores a reference to the heap-allocated staging vector, whose address never changes even when this
    // registration is moved.
    auto [groupIt, inserted] = _owner->_groupIndex.emplace(key, 0);
    _groupIt = groupIt;
    if(inserted) {
      // Give the new group a stable slot: reuse a freshly freed index if available, otherwise append. Appending or
      // reusing a free slot never shifts the index of an existing live group, so their ConverterLoopHelpers stay valid.
      size_t groupId;
      if(!_owner->_freeSlots.empty()) {
        groupId = _owner->_freeSlots.back();
        _owner->_freeSlots.pop_back();
        _owner->_groups[groupId] = std::make_unique<Group<UserType>>();
      }
      else {
        groupId = _owner->_groups.size();
        _owner->_groups.emplace_back(std::make_unique<Group<UserType>>());
      }
      groupIt->second = groupId;
      auto& group = static_cast<Group<UserType>&>(*_owner->_groups[groupId]);
      group.consumers.push_back(Consumer<UserType>(byteOffset, stride, nSamples, *_staging));
      _it = std::prev(group.consumers.end());

      group.isRaw = isRaw;
      if(!group.isRaw) {
        // The group's typed conversion loop: one ConverterLoopHelper, cloned from the 2D accessor's pattern.
        //
        // The implParameter is this group's stable index within _groups, so doPostReadImpl resolves the group by a
        // plain index (no pointer/address-based identity). The index is valid as long as the group exists: the helper
        // is owned by the group, and reset() frees the slot without shifting any other group's index, so the index is
        // never invalidated under a live helper.
        // The full register info (including the register name) and channel index 0 are passed on so RawConverter
        // error messages keep the register name and channel for the demultiplexing path.
        callForRawType(key.rawType, [&](auto x) {
          using RawType = std::make_unsigned_t<decltype(x)>;
          group.converterLoopHelper =
              RawConverter::ConverterLoopHelper::makeConverterLoopHelperFixedRaw<UserType, RawType>(
                  info, 0, groupId, *owner);
        });
      }
    }
    else {
      auto& group = static_cast<Group<UserType>&>(*_owner->_groups[groupIt->second]);
      group.consumers.push_back(Consumer<UserType>(byteOffset, stride, nSamples, *_staging));
      _it = std::prev(group.consumers.end());
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
  // NOLINTNEXTLINE(clang-diagnostic-dtor-name) - linter is wrong, this is standard-conforming
  MuxedChannelDemultiplexer::Registration<UserType>::~Registration() {
    reset();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  MuxedChannelDemultiplexer::Registration<UserType>::Registration(Registration&& other) noexcept
  : _owner(other._owner), _groupIt(other._groupIt), _it(other._it), _staging(std::move(other._staging)) {
    other._owner = nullptr;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  MuxedChannelDemultiplexer::Registration<UserType>& MuxedChannelDemultiplexer::Registration<UserType>::operator=(
      Registration&& other) noexcept {
    if(this == &other) {
      return *this;
    }
    reset();
    _owner = other._owner;
    _groupIt = other._groupIt;
    _it = other._it;
    _staging = std::move(other._staging);
    other._owner = nullptr;
    return *this;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  void MuxedChannelDemultiplexer::Registration<UserType>::reset() {
    if(_owner == nullptr) {
      return;
    }
    auto groupId = _groupIt->second;
    auto& group = static_cast<Group<UserType>&>(*_owner->_groups[groupId]);
    group.consumers.erase(_it);
    if(group.consumers.empty()) {
      // This group has lost its last consumer, so it is erased from the registry. Removal happens only when the group
      // is no longer referenced: a group is only erased together with its lone consumer's replacement (transfer-group
      // merges are one-way). Erasure frees the group's vector slot without shifting the index of any other live group
      // (the slot is reused by a later registration), so no ConverterLoopHelper's implParameter index goes stale.
      //
      // Assert the no-erasure-while-reused assumption: after erasure no other key in the registry may still resolve to
      // the freed slot, since that would make a later consumer read the reused slot's data or invoke a stale helper.
      _owner->_groupIndex.erase(_groupIt);
      for(const auto& indexIt : _owner->_groupIndex) {
        assert(indexIt.second != groupId);
      }
      _owner->_freeSlots.push_back(groupId);
      _owner->_groups[groupId].reset();
    }
    _owner = nullptr;
    _staging.reset();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType>& MuxedChannelDemultiplexer::Registration<UserType>::staging() {
    return *_staging;
  }

  /********************************************************************************************************************/

  // The Registration member functions are not performance critical (they only run when consumers are registered or
  // unregistered), so they are not inlined into the class definition but defined here and explicitly instantiated for
  // all supported user types. The destructor only delegates to reset(), so it is defined inline at the end of the
  // header.

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MuxedChannelDemultiplexer::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
