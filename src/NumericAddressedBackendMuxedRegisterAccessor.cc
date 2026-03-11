// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "NumericAddressedBackendMuxedRegisterAccessor.h"

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<class UserType>
  NumericAddressedBackendMuxedRegisterAccessor<UserType>::NumericAddressedBackendMuxedRegisterAccessor(
      const RegisterPath& registerPathName, size_t numberOfElements, size_t elementsOffset,
      const boost::shared_ptr<DeviceBackend>& _backend)
  : NDRegisterAccessor<UserType>(registerPathName, {}),
    _ioDevice(boost::dynamic_pointer_cast<NumericAddressedBackend>(_backend)) {
    // Obtain information about the area
    _registerInfo = _ioDevice->_registerMap.getBackendRegister(registerPathName);
    assert(!_registerInfo.channels.empty());

    // check information
    if(_registerInfo.elementPitchBits % 8 != 0) {
      throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: blocks must be byte aligned.");
    }

    // Helper struct containing everything in ChannelInfo except the bitOffset. This helps to identify which channels
    // can share the RawConverter.
    struct ConverterInfo {
      NumericAddressedRegisterInfo::Type dataType;
      uint32_t width;
      int32_t nFractionalBits;
      bool signedFlag;
      DataType rawType;

      // Comparison is required to use objects as std::map key
      auto operator<=>(const ConverterInfo& rhs) const = default;

      // construct from ChannelInfo
      explicit ConverterInfo(const NumericAddressedRegisterInfo::ChannelInfo& o)
      : dataType(o.dataType), width(o.width), nFractionalBits(o.nFractionalBits), signedFlag(o.signedFlag),
        rawType(o.rawType) {}
    };

    // Identify groups of channels with identical raw conversion parameters. All channels in one group can hence share
    // the same RawConverter. This allows for some performance optimisation in the de-multiplexing and conversion step.
    std::map<ConverterInfo, size_t> groupInfoMap;
    for(size_t channelIndex = 0; channelIndex < _registerInfo.getNumberOfChannels(); ++channelIndex) {
      const auto& info = _registerInfo.channels[channelIndex];

      if(info.bitOffset % 8 != 0) {
        throw ChimeraTK::logic_error("NumericAddressedBackendMuxedRegisterAccessor: elements must be byte aligned.");
      }

      // Check if a matching group already exists in the groupInfoMap
      const auto converterInfo = ConverterInfo(info);
      auto it = groupInfoMap.find(converterInfo);
      if(it == groupInfoMap.end()) {
        // No matching group exists yet, so create new ChannelGroup containing the converter
        auto newGroupId = groupInfoMap.size();
        groupInfoMap[converterInfo] = newGroupId;

        ChannelGroup newGroup;
        newGroup.channels.emplace_back(
            channelIndex, 0, typename std::vector<UserType>::iterator()); // offsetToNext is set later
        newGroup.converterLoopHelper = RawConverter::ConverterLoopHelper::makeConverterLoopHelper<UserType>(
            _registerInfo, channelIndex, newGroupId, *this);
        newGroup.startOffset = info.bitOffset / 8;

        // Store the group and update the group map
        _channelGroups.emplace_back(std::move(newGroup));
      }
      else {
        // Matching group found: Add channel to group and update the group map
        _channelGroups[it->second].channels.emplace_back(
            channelIndex, 0, typename std::vector<UserType>::iterator()); // offsetToNext is set later
      }
    }
    assert(_channelGroups.size() == groupInfoMap.size());

    // Fill the "offsetToNext" field of the ChannelGroup::Channel struct. This cannot be done in the above loop, since
    // we need to know already which is the next channel in the group.
    for(auto& group : _channelGroups) {
      assert(group.channels.size() >= 1);
      // fill offsets for all channels in group except the last one
      for(size_t i = 0; i < group.channels.size() - 1; ++i) {
        auto bitOffset = _registerInfo.channels[group.channels[i + 1].index].bitOffset -
            _registerInfo.channels[group.channels[i].index].bitOffset;
        assert(bitOffset % 8 == 0);
        group.channels[i].offsetToNext = bitOffset / 8;
      }
      // offset for the last channel in the group needs to point to the first channel in group (next sample)
      auto lastBitOffset = _registerInfo.elementPitchBits -
          _registerInfo.channels[group.channels.back().index].bitOffset +
          _registerInfo.channels[group.channels.front().index].bitOffset;
      assert(lastBitOffset % 8 == 0);
      group.channels.back().offsetToNext = lastBitOffset / 8;
    }

    // compute effective numberOfElements
    if(numberOfElements == 0) {
      numberOfElements = _registerInfo.nElements;
    }

    // check number of words
    if(numberOfElements + elementsOffset > _registerInfo.nElements) {
      throw ChimeraTK::logic_error("Requested number of elements (" + std::to_string(numberOfElements) + " + " +
          std::to_string(elementsOffset) + ") exceeds the size (" + std::to_string(_registerInfo.nElements) +
          ") of the register '" + registerPathName + "'!");

      throw ChimeraTK::logic_error("Requested number of elements exceeds the size of the register! Requested end: " +
          std::to_string(numberOfElements + elementsOffset) +
          ", register length: " + std::to_string(_registerInfo.nElements));
    }

    // update register info
    _registerInfo.nElements = numberOfElements;
    assert(_registerInfo.elementPitchBits % 8 == 0);
    _registerInfo.address += elementsOffset * _registerInfo.elementPitchBits / 8;

    // allocate the buffer for the converted data
    NDRegisterAccessor<UserType>::buffer_2D.resize(_registerInfo.getNumberOfChannels());
    for(auto& buf : NDRegisterAccessor<UserType>::buffer_2D) {
      buf.resize(_registerInfo.nElements);
    }

    // allocate the raw io buffer. Make it one element larger to make sure we can access the last byte via int32_t*
    _ioBuffer.resize(
        static_cast<size_t>(_registerInfo.elementPitchBits) / 8 * _registerInfo.nElements / sizeof(int32_t) + 1);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doReadTransferSynchronously() {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto nbt = _registerInfo.elementPitchBits / 8 * _registerInfo.nElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->read(_registerInfo.bar, _registerInfo.address, _ioBuffer.data(), nbt);
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostRead(TransferType, bool hasNewData) {
    if(hasNewData) {
      // This will call doPostReadImpl (see below) with the proper converter for each channel group
      for(auto& group : _channelGroups) {
        group.converterLoopHelper->doPostRead();
      }

      // it is acceptable to create the version number in post read because this accessor does not have
      // wait_for_new_data. It is basically synchronous.
      this->_versionNumber = {};

      // we just read good data. Set validity back to ok if someone marked it faulty for writing.
      this->_dataValidity = DataValidity::ok;
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPostReadImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelGroupId) {
    static_assert(std::is_same_v<UserType, UserType2>);
    if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
      auto& group = _channelGroups[channelGroupId];

      // initialise the cooked iterators with beginning of the buffer
      for(auto& channel : group.channels) {
        channel.cookedIterator = buffer_2D[channel.index].begin();
      }

      // Using a raw pointer for the raw buffer... We need to move the pointer byte-wise and will do a memcpy later,
      // hence this is actually safe.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* rawIterator = reinterpret_cast<std::byte*>(_ioBuffer.data()) + group.startOffset;

      for(size_t i = 0; i < this->getNumberOfSamples(); ++i) {
        for(auto& channel : group.channels) {
          // acquire the raw value safely from the buffer, independent of alignment etc.
          RawType rawValue;
          std::memcpy(&rawValue, rawIterator, sizeof(RawType));

          // perform conversion and store to cooked buffer
          *channel.cookedIterator = converter.toCooked(rawValue);

          // increment iterators
          ++channel.cookedIterator;
          rawIterator += channel.offsetToNext;
        }
      }
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  bool NumericAddressedBackendMuxedRegisterAccessor<UserType>::doWriteTransfer(VersionNumber) {
    assert(_registerInfo.elementPitchBits % 8 == 0);

    auto nbt = _registerInfo.elementPitchBits / 8 * _registerInfo.nElements;
    nbt = ((nbt - 1) / 4 + 1) * 4; // round up to multiple of 4 bytes

    _ioDevice->write(_registerInfo.bar, _registerInfo.address, &(_ioBuffer[0]), nbt);
    return false;
  }

  /********************************************************************************************************************/

  template<class UserType>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWrite(TransferType, VersionNumber) {
    if(!_ioDevice->isOpen()) {
      throw ChimeraTK::logic_error("Device not opened.");
    }

    for(auto& group : _channelGroups) {
      group.converterLoopHelper->doPreWrite();
    }
  }

  /********************************************************************************************************************/

  template<class UserType>
  template<class UserType2, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
      bool isSigned>
  void NumericAddressedBackendMuxedRegisterAccessor<UserType>::doPreWriteImpl(
      RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelGroupId) {
    if constexpr(!std::is_same_v<RawType, ChimeraTK::Void>) {
      auto& group = _channelGroups[channelGroupId];

      // Vector with pairs of source iterator and destination iterator
      for(auto& channel : group.channels) {
        channel.cookedIterator = buffer_2D[channel.index].begin();
      }

      // Using a raw pointer for the raw buffer... We need to move the pointer byte-wise and will do a memcpy later,
      // hence this is actually safe.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto* rawIterator = reinterpret_cast<std::byte*>(_ioBuffer.data()) + group.startOffset;

      for(size_t i = 0; i < this->getNumberOfSamples(); ++i) {
        for(auto& channel : group.channels) {
          RawType rawValue = converter.toRaw(*channel.cookedIterator);

          // store the raw value safely to the buffer, independent of alignment etc.
          std::memcpy(rawIterator, &rawValue, sizeof(RawType));

          // increment iterators
          ++channel.cookedIterator;
          rawIterator += channel.offsetToNext;
        }
      }
    }
  }

  /********************************************************************************************************************/

  INSTANTIATE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);
} // namespace ChimeraTK
