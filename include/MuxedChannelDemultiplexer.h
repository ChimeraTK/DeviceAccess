// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedRegisterCatalogue.h"
#include "RawConverter.h"
#include "SupportedUserTypes.h"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <typeindex>
#include <vector>

namespace ChimeraTK::detail {

  /********************************************************************************************************************/

  /**
   * Demultiplexer for strided channel slices of a muxed 2D register.
   *
   * Iterates the multiplexed raw buffer once per group of consumers with identical conversion parameters and UserType,
   * instead of once per consumer. With no consumers registered it is inert and adds no overhead.
   *
   * Carries no per-sample conversion dispatch: each non-raw group owns one RawConverter::ConverterLoopHelper which
   * dispatches once per group per read into the demultiplexer's templated doPostReadImpl (an inlined typed loop). Raw
   * groups (copied without conversion) are handled by a strided copy.
   *
   * Copying or moving leaves it empty; consumers must re-register.
   */
  class MuxedChannelDemultiplexer {
   public:
    /**
     * Identifies consumers which can share one raw pass. Matches the 2D accessor's converter information plus the
     * UserType; raw slices are grouped by their UserType and flagged isRaw, so they never mix with cooking consumers.
     */
    struct GroupKey {
      NumericAddressedRegisterInfo::Type dataType;
      uint32_t width;
      int32_t nFractionalBits;
      bool signedFlag;
      DataType rawType;
      std::type_index userType;
      bool isRaw;

      auto operator<=>(const GroupKey&) const = default;
    };

    /**
     * One strided channel consumer. byteOffset is the channel's first sample within the raw buffer (already
     * accounting for the sample offset), stride the distance between samples (elementPitchBits / 8). The staging
     * buffer pointer is resolved lazily, since a wrapping decorator may swap the buffer on every read.
     */
    struct Consumer {
      size_t byteOffset{};
      size_t stride{};
      size_t nSamples{};
      std::function<uint8_t*()> cookedBuffer;
    };

    /**
     * One group of consumers sharing a single raw pass. Each consumer's loop runs up to its own sample count, so the
     * group stores no shared iteration bound. Non-raw groups own a ConverterLoopHelper; raw groups are copied without
     * conversion.
     */
    struct Group {
      std::list<Consumer> consumers;
      bool isRaw{};
      size_t cookedElementSize{};
      std::unique_ptr<RawConverter::ConverterLoopHelper> converterLoopHelper;
    };

    /**
     * Handle removing its consumer from the registry on destruction or reset, so a consumer can be re-registered,
     * e.g. when its element is replaced during transfer-group assembly. Movable, not copyable.
     *
     * The handle owns the staging buffer into which the demultiplexer writes the channel's cooked data. The owning
     * accessor swaps it into its application buffer in its own post-read step, so the transfer-element rules hold:
     * the application buffer is changed only in doPostRead, data and meta data are updated together, and an
     * exception leaves the buffers unchanged.
     */
    template<typename UserType>
    class Registration {
     public:
      Registration() = default;

      /** Register the given consumer with the owner. The implementation is not performance critical and therefore
       *  placed in the .cc file; the template is explicitly instantiated for all supported user types there. */
      Registration(MuxedChannelDemultiplexer* owner, GroupKey key, Consumer consumer);
      ~Registration();

      Registration(const Registration&) = delete;
      Registration& operator=(const Registration&) = delete;

      Registration(Registration&& other) noexcept;
      Registration& operator=(Registration&& other) noexcept;

      /** Remove the consumer from the registry. Idempotent. */
      void reset();

      [[nodiscard]] bool active() const;

      /** The staging buffer the demultiplexer fills. Its data pointer changes after each swap into the application
       *  buffer, so the consumer resolves it lazily on every read. */
      [[nodiscard]] std::vector<UserType>& staging();

     private:
      friend class MuxedChannelDemultiplexer;

      MuxedChannelDemultiplexer* _owner{nullptr};
      std::map<GroupKey, Group>::iterator _groupIt;
      std::list<Consumer>::iterator _it;
      std::vector<UserType> _staging;
    };

    MuxedChannelDemultiplexer() = default;

    /** Copying or moving leaves the demultiplexer empty (see class comment). */
    MuxedChannelDemultiplexer(const MuxedChannelDemultiplexer&);
    MuxedChannelDemultiplexer(MuxedChannelDemultiplexer&&) noexcept;
    MuxedChannelDemultiplexer& operator=(const MuxedChannelDemultiplexer&);
    MuxedChannelDemultiplexer& operator=(MuxedChannelDemultiplexer&&) noexcept;

    ~MuxedChannelDemultiplexer() = default;

    /** Remove all consumers. Existing Registration handles become inert. */
    void clear();

    [[nodiscard]] bool empty() const;

    /** Remember that fresh raw data is available for demultiplexing. */
    void rememberNewData();

    /** True if fresh raw data has been remembered and not yet demultiplexed. */
    [[nodiscard]] bool hasNewData() const;

    /** Demultiplex the given fresh raw buffer into all registered consumers, grouped, and clear the new-data flag. */
    void run(const uint8_t* rawBuffer);

    /**
     * Callback for RawConverter::ConverterLoopHelper, see its documentation. groupId is the index of the group within
     * _groups. Performs the inlined typed conversion loop for the group's consumers, resolving each staging buffer once.
     */
    template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId);

    /**
     * Callback for RawConverter::ConverterLoopHelper used by the read-only demultiplexer. The demultiplexer never
     * writes, so this is a no-op; it exists only to satisfy the ConverterLoopHelper::doPreWrite interface.
     */
    template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPreWriteImpl(RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId);

   private:
    template<typename>
    friend class Registration;

    std::map<GroupKey, Group> _groups;
    bool _hasNewData{false};
    const uint8_t* _rawBuffer{nullptr};
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  inline void MuxedChannelDemultiplexer::rememberNewData() {
    _hasNewData = true;
  }

  /********************************************************************************************************************/

  inline bool MuxedChannelDemultiplexer::hasNewData() const {
    return _hasNewData;
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MuxedChannelDemultiplexer::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
