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
   * Read coordinator of a NumericAddressed low-level transfer element.
   *
   * Serves every read consumer of the element: scalar registers, non-muxed 1D arrays and strided channel slices of a
   * muxed 2D register alike. It iterates the (potentially multiplexed) raw buffer once per group of consumers with
   * identical conversion parameters and UserType. The write path is not served by the coordinator.
   */
  class ReadCoordinator {
   public:
    /**
     * Identifies consumers which can share one raw pass.
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
     * Read consumer: represents high-level accessor receiving data
     */
    template<typename UserType>
    struct Consumer {
      Consumer(size_t offset, size_t pitch, size_t count, std::vector<UserType>& stagingBuffer);

      size_t byteOffset{}; // offset of first sample within the raw buffer
      size_t stride{};     // elementPitchBits / 8 (equal to the element data width for a non-strided consumer)
      size_t nSamples{};
      std::vector<UserType>& staging;
    };

    /**
     * Type-erased base of a group of consumers sharing a single raw pass. Holds the shared raw-mode state and the
     * ownership of the group's ConverterLoopHelper; the typed Group<UserType> derived class holds the consumer list
     * (all consumers of one group share the GroupKey UserType).
     */
    struct GroupBase {
      /** True when the group has lost all its consumers (it stays registered until teardown, see _groups). */
      [[nodiscard]] virtual bool consumersEmpty() const = 0;

      /** Raw copy of all consumers' samples into their staging buffers. Called for raw-mode groups only. */
      virtual void rawCopy(const std::vector<uint8_t>& rawBuffer) = 0;

      virtual ~GroupBase() = default;

      bool isRaw{};
      std::unique_ptr<RawConverter::ConverterLoopHelper> converterLoopHelper;
    };

    /**
     * One group of consumers sharing a single raw pass. Converting groups own a ConverterLoopHelper; raw-mode groups
     * are copied without conversion.
     */
    template<typename UserType>
    struct Group : GroupBase {
      [[nodiscard]] bool consumersEmpty() const override;

      /** Typed memcpy of each consumer's samples (up to its own sample count) into its staging buffer. */
      void rawCopy(const std::vector<uint8_t>& rawBuffer) override;

      std::list<Consumer<UserType>> consumers;
    };

    /**
     * Per-consumer registration handle (owned by the high-level accessor): it ties one read consumer to its group in
     * the coordinator's registry and owns the staging buffer into which the group's conversion loop writes the
     * consumer's samples. Destroying or resetting the handle deregisters the consumer. It is movable (so a consumer can
     * be re-registered when a transfer-group merge replaces its element) but not copyable.
     */
    template<typename UserType>
    class Registration {
     public:
      Registration() = default;

      /**
       * Register the given read consumer with the owner.
       *
       * The full register info (including the register name) is kept so RawConverter error messages stay informative;
       * isRaw marks a raw-mode consumer, which forms a separate raw-copy group.
       */
      Registration(ReadCoordinator* owner, const NumericAddressedRegisterInfo& info, bool isRaw, size_t byteOffset,
          size_t stride, size_t nSamples);

      ~Registration();

      Registration(const Registration&) = delete;
      Registration& operator=(const Registration&) = delete;

      Registration(Registration&& other) noexcept;
      Registration& operator=(Registration&& other) noexcept;

      /** Remove the consumer from the registry. Idempotent. */
      void reset();

      /** Get the staging buffer the coordinator fills. */
      [[nodiscard]] std::vector<UserType>& staging();

     private:
      friend class ReadCoordinator;

      ReadCoordinator* _owner{nullptr};
      size_t _groupId{}; // the consumer's group, see _groups
      std::list<Consumer<UserType>>::iterator _it;

      // Heap-allocated so its address (which the registered Consumer references) stays stable when the registration is
      // moved; the moveable handle owns it, the buffer's data pointer changes on each swap, never its address.
      std::unique_ptr<std::vector<UserType>> _staging;
    };

    ReadCoordinator() = default;

    ReadCoordinator(const ReadCoordinator&) = delete;
    ReadCoordinator(ReadCoordinator&&) noexcept = delete;
    ReadCoordinator& operator=(const ReadCoordinator&) = delete;
    ReadCoordinator& operator=(ReadCoordinator&&) noexcept = delete;

    ~ReadCoordinator() = default;

    /** True when no consumers are registered. */
    [[nodiscard]] bool empty() const;

    /**
     * Demultiplex the given fresh raw buffer into all registered consumers, grouped. Called once per read from the
     * element's doPostRead, which the base class enters at most once per read.
     */
    void run(const std::vector<uint8_t>& rawBuffer);

    /**
     * Callback for RawConverter::ConverterLoopHelper, see its documentation. groupId identifies the group within
     * _groups. Performs the inlined typed conversion loop for the group's consumers, resolving each staging buffer
     * once.
     */
    template<class UserType, typename RawType, RawConverter::SignificantBitsCase sc, RawConverter::FractionalCase fc,
        bool isSigned>
    void doPostReadImpl(RawConverter::Converter<UserType, RawType, sc, fc, isSigned> converter, size_t groupId);

    /**
     * Callback for RawConverter::ConverterLoopHelper used by the read-only coordinator. The coordinator never writes,
     * so this is a no-op; it exists only to satisfy the ConverterLoopHelper::doPreWrite interface.
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
    // the current read's raw buffer, set only while run() iterates the groups; null between reads
    const std::vector<uint8_t>* _rawBuffer{nullptr};
  };

  /********************************************************************************************************************/

  template<typename UserType>
  inline ReadCoordinator::Consumer<UserType>::Consumer(
      size_t offset, size_t pitch, size_t count, std::vector<UserType>& stagingBuffer)
  : byteOffset(offset), stride(pitch), nSamples(count), staging(stagingBuffer) {}

  /********************************************************************************************************************/

  template<typename UserType>
  inline bool ReadCoordinator::Group<UserType>::consumersEmpty() const {
    return consumers.empty();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  inline void ReadCoordinator::Group<UserType>::rawCopy(const std::vector<uint8_t>& rawBuffer) {
    for(const auto& consumer : consumers) {
      if(consumer.nSamples == 0) {
        continue;
      }
      auto* cooked = consumer.staging.data();
      const auto* raw = rawBuffer.data() + consumer.byteOffset;
      // A raw-mode group holds consumers whose UserType is the register's raw data type: an integer of the matching
      // size and signedness (signed or unsigned). The nested if constexpr is a compile-time gate: the memcpy below is
      // instantiated only for such types and never for any other UserType (which would be undefined behaviour). The
      // outer integral guard comes first because std::make_unsigned_t is defined only for integral and enum types, so
      // the unsigned counterpart may be formed only behind it; the remaining non-integral UserTypes are excluded by it.
      // isRawType covers unsigned raw types only, while a raw-mode UserType may be signed, hence the unsigned
      // counterpart is checked. Such a use cannot be formed through the library, so no runtime fallback exists.
      if constexpr(std::is_integral_v<UserType>) {
        if constexpr(ChimeraTK::isRawType<std::make_unsigned_t<UserType>>) {
          for(size_t i = 0; i < consumer.nSamples; ++i) {
            std::memcpy(cooked + i, raw + i * consumer.stride, sizeof(UserType));
          }
        }
      }
    }
  }

  /********************************************************************************************************************/

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(ReadCoordinator::Registration);

  /********************************************************************************************************************/

} // namespace ChimeraTK::detail
