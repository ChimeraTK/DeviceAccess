// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "NDRegisterAccessor.h"
#include "NumericAddressedRegisterCatalogue.h"
#include "RawConverter.h"
#include "SupportedUserTypes.h"

#include <cstring>
#include <functional>
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
      Consumer(size_t offset, size_t pitch, size_t count, std::vector<UserType>& stagingBuffer, DataValidity& validity,
          bool& gateClosed, bool& hasGate, RegisterPath gatePath, uint64_t gateValue);

      size_t byteOffset{}; // offset of first sample within the raw buffer
      size_t stride{};     // elementPitchBits / 8 (equal to the element data width for a non-strided consumer)
      size_t nSamples{};
      std::vector<UserType>& staging;
      DataValidity& validity; // the consumer's data validity, written by the coordinator
      bool& gateClosed;       // the consumer's gate result, written by the coordinator
      bool& hasGate;          // whether this consumer carries a gate
      // The gating register accessor itself lives in the element's gate registry, keyed by this path; the coordinator
      // resolves it through the sink on each gate evaluation. gateValue is the expected value that opens the gate.
      RegisterPath gatePath;
      uint64_t gateValue{};
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

      /** The owning coordinator, so a group can evaluate its consumers' gates through the coordinator's sink. */
      ReadCoordinator* owner{nullptr};

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
     * Per-consumer state owned by the registration: the staging buffer the coordinator fills plus the two pieces of
     * per-consumer outcome the coordinator reports back through the registration - the data validity and the gate
     * result. Heap-allocated behind the registration (see Registration) so the registered Consumer's references stay
     * valid when the registration handle is moved, e.g. on a transfer-group merge.
     */
    template<typename UserType>
    struct ConsumerState {
      std::vector<UserType> staging;
      DataValidity validity{DataValidity::ok};
      bool gateClosed{false};
      bool hasGate{false};
      RegisterPath gatePath;
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
       * The full register info (including the register name and the path) is kept so RawConverter error messages stay
       * informative and so the consumer's gate accessor can be resolved from the element's gate registry. isRaw marks a
       * raw-mode consumer, which forms a separate raw-copy group. hasGate together with the register info's gatedBy and
       * gateValue attaches a gate to the consumer: the coordinator opens or closes it by comparing the gating
       * register's value with gateValue.
       */
      Registration(ReadCoordinator* owner, const NumericAddressedRegisterInfo& info, bool isRaw, size_t byteOffset,
          size_t stride, size_t nSamples, bool hasGate = false, uint64_t gateValue = 0);

      ~Registration();

      Registration(const Registration&) = delete;
      Registration& operator=(const Registration&) = delete;

      Registration(Registration&& other) noexcept;
      Registration& operator=(Registration&& other) noexcept;

      /** Remove the consumer from the registry. Idempotent. */
      void reset();

      /** Get the staging buffer the coordinator fills. */
      [[nodiscard]] std::vector<UserType>& staging();

      /** The consumer's data validity, as last written by the coordinator. */
      [[nodiscard]] DataValidity validity() const;

      /** The consumer's gate result, as last written by the coordinator. True while the gate is closed. */
      [[nodiscard]] bool gateClosed() const;

      /** Whether this consumer carries a gate. */
      [[nodiscard]] bool hasGate() const;

      /** The gating register accessor of this consumer, or null when it has no gate. Short for the element's
       * registry lookup by the consumer's gating register path. */
      [[nodiscard]] boost::shared_ptr<NDRegisterAccessor<uint64_t>> gateAccessor() const;

     private:
      friend class ReadCoordinator;

      ReadCoordinator* _owner{nullptr};
      size_t _groupId{}; // the consumer's group, see _groups
      std::list<Consumer<UserType>>::iterator _it;

      // Heap-allocated so its address (which the registered Consumer references) stays stable when the registration is
      // moved; the moveable handle owns it, the buffer's data pointer changes on each swap, never its address.
      std::unique_ptr<ConsumerState<UserType>> _state;

      uint64_t _gateValue{0};
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

    /**
     * Set the sink through which the coordinator asks the owning low-level element for the gate accessor of a gating
     * register. The element owns the gate accessors (keyed by gating register path, so one gating register is read once
     * per transfer however many consumers it gates); the registration hands it the gating register path and the
     * element builds the accessor itself for a path it does not yet hold, handing back the accessor it holds. The
     * accessor is looked up lazily on each gate evaluation, so a re-registration on a transfer-group merge (which moves
     * a consumer to a different element) resolves against the new owner.
     */
    void setGateAccessorSink(std::function<boost::shared_ptr<NDRegisterAccessor<uint64_t>>(const RegisterPath&)> sink);

   private:
    template<typename>
    friend class Registration;

    /**
     * The gate result of one consumer: true while the gating register's freshly read value differs from the consumer's
     * expected value. Preconditions (the accessor exists, is readable and its register is scalar) are checked when the
     * gated accessor is created, so no check is repeated here.
     */
    template<typename UserType>
    [[nodiscard]] bool evaluateGateClosed(const Consumer<UserType>& consumer) const;

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
    // sink asking the owning element for the gate accessor of a gating register, see setGateAccessorSink()
    std::function<boost::shared_ptr<NDRegisterAccessor<uint64_t>>(const RegisterPath&)> _gateAccessorSink;
  };

  /********************************************************************************************************************/

  template<typename UserType>
  inline ReadCoordinator::Consumer<UserType>::Consumer(size_t offset, size_t pitch, size_t count,
      std::vector<UserType>& stagingBuffer, DataValidity& consumerValidity, bool& consumerGateClosed,
      bool& consumerHasGate, RegisterPath consumerGatePath, uint64_t consumerGateValue)
  : byteOffset(offset), stride(pitch), nSamples(count), staging(stagingBuffer), validity(consumerValidity),
    gateClosed(consumerGateClosed), hasGate(consumerHasGate), gatePath(std::move(consumerGatePath)),
    gateValue(consumerGateValue) {}

  /********************************************************************************************************************/

  template<typename UserType>
  inline bool ReadCoordinator::evaluateGateClosed(const Consumer<UserType>& consumer) const {
    // Resolve the consumer's gating register accessor from the owning element on each evaluation, so the lookup stays
    // correct after a transfer-group merge moved the consumer to another element. The accessor's value has just been
    // filled by the element's gate group, which is read in the transfer phase before this consumer's group is
    // converted. A single read of the gating register per evaluation.
    auto gateAccessor = _gateAccessorSink(consumer.gatePath);
    return gateAccessor->accessData(0) != consumer.gateValue;
  }

  /********************************************************************************************************************/

  template<typename UserType>
  inline bool ReadCoordinator::Group<UserType>::consumersEmpty() const {
    return consumers.empty();
  }

  /********************************************************************************************************************/

  template<typename UserType>
  inline void ReadCoordinator::Group<UserType>::rawCopy(const std::vector<uint8_t>& rawBuffer) {
    for(const auto& consumer : consumers) {
      // Evaluate and record the gate while the group is processed, exactly as the converting path does.
      if(consumer.hasGate) {
        consumer.gateClosed = owner->evaluateGateClosed(consumer);
        if(consumer.gateClosed) {
          consumer.validity = DataValidity::faulty;
          continue;
        }
      }
      consumer.gateClosed = false;
      consumer.validity = DataValidity::ok;
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
