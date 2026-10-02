// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Exception.h"
#include "NDRegisterAccessor.h"
#include "NumericAddressedBackend.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "RawConverter.h"

#include <boost/shared_ptr.hpp>

namespace ChimeraTK {

  constexpr auto MULTIPLEXED_SEQUENCE_PREFIX = "AREA_MULTIPLEXED_SEQUENCE_";
  constexpr auto SEQUENCE_PREFIX = "SEQUENCE_";

  constexpr auto MEM_MULTIPLEXED_PREFIX = "MEM_MULTIPLEXED_";

  /********************************************************************************************************************/
  /**
   * Implementation of the NDRegisterAccessor for NumericAddressedBackends for multiplexd 2D registers
   */
  template<class UserType>
  class NumericAddressedBackendMuxedRegisterAccessor : public NDRegisterAccessor<UserType> {
   public:
    NumericAddressedBackendMuxedRegisterAccessor(const RegisterPath& registerPathName, size_t numberOfElements,
        size_t elementsOffset, const boost::shared_ptr<DeviceBackend>& _backend);

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    template<typename UserType2, typename RawType, RawConverter::SignificantBitsCase sc,
        RawConverter::FractionalCase fc, bool isSigned>
    void doPostReadImpl(RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelGroupId);

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

    void doPreWrite(TransferType type, VersionNumber versionNumber) override;

    template<typename UserType2, typename RawType, RawConverter::SignificantBitsCase sc,
        RawConverter::FractionalCase fc, bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<UserType2, RawType, sc, fc, isSigned> converter, size_t channelGroupId);

    void doPreRead(TransferType) override {
      if(!_ioDevice->isOpen()) throw ChimeraTK::logic_error("Device not opened.");
    }

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override {
      auto rhsCasted = boost::dynamic_pointer_cast<const NumericAddressedBackendMuxedRegisterAccessor<UserType>>(other);
      if(rhsCasted.get() == this) {
        return false;
      }
      if(!rhsCasted) return false;
      if(_ioDevice != rhsCasted->_ioDevice) return false;
      if(_registerInfo != rhsCasted->_registerInfo) return false;
      // No need to compare converters, since they are derived from registerInfo and UserType.
      return true;
    }

    [[nodiscard]] bool isReadOnly() const override { return isReadable() && !isWriteable(); }

    [[nodiscard]] bool isReadable() const override { return _registerInfo.isReadable(); }

    [[nodiscard]] bool isWriteable() const override { return _registerInfo.isWriteable(); }

   protected:
    // Channels will be grouped such that all channels in each group can use the same RawConverter. This allows us to
    // implement a more cache efficient demultiplexing strategy.
    struct ChannelGroup {
      struct Channel {
        // Index of the channel
        size_t index;

        // Number of bytes to advance the raw iterator/pointer after a sample for the channel has been processed, so it
        // subsequently points to the next sample of the next channel to process within this ChannelGroup. For the last
        // channel in the group, it will contain the offset to move to the next sample for the first channel in the
        // group.
        size_t offsetToNext;

        // This iterator is needed only during doPostReadImpl/doPreWriteImpl, but to avoid frequent memory
        // (de)allocation, we provide storage for it here.
        std::vector<UserType>::iterator cookedIterator;
      };
      std::vector<Channel> channels;

      std::unique_ptr<RawConverter::ConverterLoopHelper> converterLoopHelper;

      // offset from beginning of the register to the first sample in the group.
      size_t startOffset{};
    };
    std::vector<ChannelGroup> _channelGroups;

    /** The device from (/to) which to perform the DMA transfer */
    boost::shared_ptr<NumericAddressedBackend> _ioDevice;

    std::vector<int32_t> _ioBuffer;

    NumericAddressedRegisterInfo _registerInfo;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE

    using NDRegisterAccessor<UserType>::buffer_2D;
    using TransferElement::_exceptionBackend;
  };

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(NumericAddressedBackendMuxedRegisterAccessor);

  /********************************************************************************************************************/

} // namespace ChimeraTK
