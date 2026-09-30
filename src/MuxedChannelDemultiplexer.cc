// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MuxedChannelDemultiplexer.h"

#include "SupportedUserTypes.h"

#include <cstring>
#include <iterator>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  MuxedChannelDemultiplexer::MuxedChannelDemultiplexer(const MuxedChannelDemultiplexer&) {
    clear();
  }

  /********************************************************************************************************************/

  MuxedChannelDemultiplexer::MuxedChannelDemultiplexer(MuxedChannelDemultiplexer&&) noexcept {
    clear();
  }

  /********************************************************************************************************************/

  MuxedChannelDemultiplexer& MuxedChannelDemultiplexer::operator=(const MuxedChannelDemultiplexer& other) {
    if(this == &other) {
      return *this;
    }
    clear();
    return *this;
  }

  /********************************************************************************************************************/

  MuxedChannelDemultiplexer& MuxedChannelDemultiplexer::operator=(MuxedChannelDemultiplexer&& other) noexcept {
    if(this == &other) {
      return *this;
    }
    clear();
    return *this;
  }

  /********************************************************************************************************************/

  void MuxedChannelDemultiplexer::clear() {
    _groups.clear();
    _hasNewData = false;
    _rawBuffer = nullptr;
  }

  /********************************************************************************************************************/

  bool MuxedChannelDemultiplexer::empty() const {
    return _groups.empty();
  }

  /********************************************************************************************************************/

  void MuxedChannelDemultiplexer::run(const uint8_t* rawBuffer) {
    _hasNewData = false;
    _rawBuffer = rawBuffer;
    for(auto& [key, group] : _groups) {
      (void)key;
      if(group.isRaw) {
        // Raw slices are copied without conversion: one strided copy per consumer, up to its sample count.
        for(auto& consumer : group.consumers) {
          if(consumer.nSamples == 0) {
            continue;
          }
          auto* cooked = consumer.cookedBuffer();
          const auto* raw = rawBuffer + consumer.byteOffset;
          for(size_t i = 0; i < consumer.nSamples; ++i) {
            std::memcpy(cooked + i * group.cookedElementSize, raw + i * consumer.stride, group.cookedElementSize);
          }
        }
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
  void MuxedChannelDemultiplexer::doPostReadImpl(
      RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId) {
    // The groupId is the address of the group's entry within _groups, see the Registration constructor. It is valid as
    // long as the ConverterLoopHelper exists, which is exactly when this method is invoked.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto& group = *reinterpret_cast<Group*>(groupId);
    // Each consumer's staging buffer is resolved once per read (a wrapping decorator may swap it on every read), so
    // there is no per-sample indirect call in the typed conversion loop.
    for(auto& consumer : group.consumers) {
      if(consumer.nSamples == 0) {
        continue;
      }
      auto* cooked = consumer.cookedBuffer();
      const auto* raw = _rawBuffer + consumer.byteOffset;
      for(size_t i = 0; i < consumer.nSamples; ++i) {
        RawType temp;
        std::memcpy(&temp, raw + i * consumer.stride, sizeof(RawType));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<UserType*>(cooked + i * sizeof(UserType)) = converter.toCooked(temp);
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
      MuxedChannelDemultiplexer::GroupKey key, MuxedChannelDemultiplexer::Consumer consumer)
  : _owner(owner), _staging(consumer.nSamples) {
    _groupIt = _owner->_groups.emplace(key, Group{}).first;
    auto& group = _groupIt->second;
    const bool newGroup = group.consumers.empty();
    group.consumers.push_back(std::move(consumer));
    _it = std::prev(group.consumers.end());

    if(newGroup) {
      group.isRaw = key.isRaw;
      // The cooked element size describes one cooked sample regardless of grouping; raw and cooked groups both copy
      // sizeof(UserType) bytes per sample (for raw mode the UserType is the raw type itself).
      group.cookedElementSize = sizeof(UserType);
      if(!group.isRaw) {
        // The group's typed conversion loop: one ConverterLoopHelper, cloned from the 2D accessor's pattern.
        //
        // The implParameter is the address of this group's entry within _groups. Nodes of a std::map are stable in
        // memory until the node is erased, and the ConverterLoopHelper is owned by the group, so the address is valid
        // whenever the helper is invoked: when the group is erased the helper is destroyed with it. A plain index
        // (distance) would not be stable, since erasing a group that sorts earlier would shift the indices of later
        // groups.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto groupId = reinterpret_cast<size_t>(&group);
        // The demultiplexer holds no register info, only the channel fields of the group key, so the fixed-raw
        // factory is fed a ChannelInfo built from them.
        NumericAddressedRegisterInfo::ChannelInfo channel;
        channel.dataType = key.dataType;
        channel.width = key.width;
        channel.nFractionalBits = key.nFractionalBits;
        channel.signedFlag = key.signedFlag;
        channel.rawType = key.rawType;
        callForRawType(key.rawType, [&](auto x) {
          using RawType = std::make_unsigned_t<decltype(x)>;
          group.converterLoopHelper =
              RawConverter::ConverterLoopHelper::makeConverterLoopHelperFixedRaw<UserType, RawType>(
                  channel, groupId, *owner);
        });
      }
    }
  }

  /********************************************************************************************************************/

  template<typename UserType>
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
    auto& group = _groupIt->second;
    group.consumers.erase(_it);
    if(group.consumers.empty()) {
      _owner->_groups.erase(_groupIt);
    }
    _owner = nullptr;
    _staging.clear();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  bool MuxedChannelDemultiplexer::Registration<UserType>::active() const {
    return _owner != nullptr;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  std::vector<UserType>& MuxedChannelDemultiplexer::Registration<UserType>::staging() {
    return _staging;
  }

  /********************************************************************************************************************/

  // The Registration member functions are not performance critical (they only run when consumers are registered or
  // unregistered), so they are not inlined into the class definition but defined here and explicitly instantiated for
  // all supported user types.

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MuxedChannelDemultiplexer::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
