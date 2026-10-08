// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NumericAddressedRegisterCatalogue.h"
#include "RawConverter.h"
#include "SupportedUserTypes.h"

#include <cstring>
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
   * instead of once per consumer. With no consumers registered it is inert and adds almost no overhead.
   */
  class MuxedChannelDemultiplexer {
   public:
    /**
     * Identifies consumers which can share one raw pass. Matches the 2D accessor's converter information plus the
     * UserType; raw slices are grouped by their UserType and flagged isRaw, so they never mix with cooking consumers.
     */
    struct GroupKey {
      NumericAddressedRegisterInfo::Type dataType{};
      uint32_t width{};
      int32_t nFractionalBits{};
      bool signedFlag{};
      DataType rawType;
      std::type_index userType{typeid(std::nullptr_t)};
      bool isRaw{};

      auto operator<=>(const GroupKey&) const = default;
    };

    /**
     * One strided channel consumer.
     */
    template<typename UserType>
    struct Consumer {
      Consumer(size_t offset, size_t pitch, size_t count, std::vector<UserType>& stagingBuffer);

      size_t byteOffset{}; // offset of first sample within raw buffer
      size_t stride{};     // elementPitchBits / 8
      size_t nSamples{};
      std::vector<UserType>& staging;
    };

    /**
     * Type-erased base of a group of consumers sharing a single raw pass. Holds the shared isRaw state and the
     * ownership of the group's ConverterLoopHelper; the typed Group<UserType> derived class holds the per-user-type
     * consumer list (all consumers of one group share the GroupKey user type).
     */
    struct GroupBase {
      /** True when the group has lost all its consumers (it stays registered until teardown, see _groups). */
      [[nodiscard]] virtual bool consumersEmpty() const = 0;

      /** Strided raw copy of all consumers' samples into their staging buffers. Called for raw groups only. */
      virtual void rawCopy(const std::vector<uint8_t>& rawBuffer) = 0;

      virtual ~GroupBase() = default;

      bool isRaw{};
      std::unique_ptr<RawConverter::ConverterLoopHelper> converterLoopHelper;
    };

    /**
     * One group of consumers sharing a single raw pass. Non-raw groups own a ConverterLoopHelper; raw groups are
     * copied without conversion.
     */
    template<typename UserType>
    struct Group : GroupBase {
      [[nodiscard]] bool consumersEmpty() const override;

      /** Strided typed memcpy of each consumer's samples (up to its own sample count) into its staging buffer. */
      void rawCopy(const std::vector<uint8_t>& rawBuffer) override;

      std::list<Consumer<UserType>> consumers;
    };

    /**
     * Per-consumer registration handle: it ties one strided channel consumer to its group in the demultiplexer's
     * registry and owns the staging buffer into which the group's conversion loop writes the channel. Destroying or
     * resetting the handle deregisters the consumer. It is movable (so a consumer can be re-registered when a
     * transfer-group merge replaces its element) but not copyable.
     */
    template<typename UserType>
    class Registration {
     public:
      Registration() = default;

      /**
       * Register the given channel consumer with the owner.
       *
       * The full register info (including the register name) and channel index are kept so RawConverter error messages
       * stay informative; isRaw marks raw-mode slices, which form a separate raw-copy group.
       */
      Registration(MuxedChannelDemultiplexer* owner, const NumericAddressedRegisterInfo& info, bool isRaw,
          size_t byteOffset, size_t stride, size_t nSamples);

      ~Registration();

      Registration(const Registration&) = delete;
      Registration& operator=(const Registration&) = delete;

      Registration(Registration&& other) noexcept;
      Registration& operator=(Registration&& other) noexcept;

      /** Remove the consumer from the registry. Idempotent. */
      void reset();

      /** Get the staging buffer the demultiplexer fills. */
      [[nodiscard]] std::vector<UserType>& staging();

     private:
      friend class MuxedChannelDemultiplexer;

      MuxedChannelDemultiplexer* _owner{nullptr};
      size_t _groupId{}; // the consumer's group, see _groups
      std::list<Consumer<UserType>>::iterator _it;

      // Heap-allocated so its address (which the registered Consumer references) stays stable when the registration is
      // moved; the moveable handle owns it, the buffer's data pointer changes on each swap, never its address.
      std::unique_ptr<std::vector<UserType>> _staging;
    };

    MuxedChannelDemultiplexer() = default;

    MuxedChannelDemultiplexer(const MuxedChannelDemultiplexer&) = delete;
    MuxedChannelDemultiplexer(MuxedChannelDemultiplexer&&) noexcept = delete;
    MuxedChannelDemultiplexer& operator=(const MuxedChannelDemultiplexer&) = delete;
    MuxedChannelDemultiplexer& operator=(MuxedChannelDemultiplexer&&) noexcept = delete;

    ~MuxedChannelDemultiplexer() = default;

    /** True when no consumers are registered. */
    [[nodiscard]] bool empty() const;

    /** Remember that fresh raw data is available, so demultiplexing is pending for this read. */
    void demultiplexingPending();

    /** True if the demultiplexing for the current read has not run yet; it runs once per operation on the first call. */
    [[nodiscard]] bool pendingDemultiplexing() const;

    /** Demultiplex the given fresh raw buffer into all registered consumers, grouped, and clear the pending flag. */
    void run(const std::vector<uint8_t>& rawBuffer);

    /**
     * Callback for RawConverter::ConverterLoopHelper, see its documentation. groupId identifies the group within
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

    // _groups stores the type-erased groups keyed by their stable, monotonically increasing groupId; _groupIndex maps
    // each group key to that groupId. The groupId is a non-positional identity handed to the group's
    // ConverterLoopHelper, so it can never go stale. A group is removed only at teardown (it is never erased while the
    // element lives, it just loses its consumers), so the registry only ever grows or vanishes wholesale with the
    // element and needs no freed-slot tracking.
    std::map<size_t, std::unique_ptr<GroupBase>> _groups;
    std::map<GroupKey, size_t> _groupIndex;
    size_t _nextGroupId{};
    bool _pendingDemultiplexing{false};
    // the current read's raw buffer, valid only between run() and the last group's demultiplexing; nullable between runs
    const std::vector<uint8_t>* _rawBuffer{nullptr};
  };

  /********************************************************************************************************************/

  template<typename UserType>
  inline MuxedChannelDemultiplexer::Consumer<UserType>::Consumer(
      size_t offset, size_t pitch, size_t count, std::vector<UserType>& stagingBuffer)
  : byteOffset(offset), stride(pitch), nSamples(count), staging(stagingBuffer) {}

  /********************************************************************************************************************/

  template<typename UserType>
  inline bool MuxedChannelDemultiplexer::Group<UserType>::consumersEmpty() const {
    return consumers.empty();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  inline void MuxedChannelDemultiplexer::Group<UserType>::rawCopy(const std::vector<uint8_t>& rawBuffer) {
    for(const auto& consumer : consumers) {
      if(consumer.nSamples == 0) {
        continue;
      }
      auto* cooked = consumer.staging.data();
      const auto* raw = rawBuffer.data() + consumer.byteOffset;
      // A raw group holds raw-mode slices, whose user type is the register's raw data type: an integer of the matching
      // size and signedness (signed or unsigned). Only trivially copyable user types can be bit-copied from raw
      // memory; the if constexpr is the compile-time gate, so the strided memcpy below is not even compiled for any
      // other user type (e.g. std::string), which would be undefined behaviour. Such a use cannot be formed through the
      // library, so no runtime fallback exists.
      if constexpr(std::is_trivially_copyable_v<UserType>) {
        for(size_t i = 0; i < consumer.nSamples; ++i) {
          std::memcpy(cooked + i, raw + i * consumer.stride, sizeof(UserType));
        }
      }
    }
  }

  /********************************************************************************************************************/
  /********************************************************************************************************************/

  inline void MuxedChannelDemultiplexer::demultiplexingPending() {
    _pendingDemultiplexing = true;
  }

  /********************************************************************************************************************/

  inline bool MuxedChannelDemultiplexer::pendingDemultiplexing() const {
    return _pendingDemultiplexing;
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(MuxedChannelDemultiplexer::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
