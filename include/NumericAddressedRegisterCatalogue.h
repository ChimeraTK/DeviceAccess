// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterCatalogue.h"
#include "BackendRegisterInfoBase.h"

#include <cstdint>
#include <string>

namespace ChimeraTK {

  /********************************************************************************************************************/

  class NumericAddressedRegisterInfo : public BackendRegisterInfoBase {
   public:
    /**
     * Enum describing the access mode of the register:
     * \li read-only
     * \li write-only
     * \li read-write
     * \li interrupt (implies read-only)
     */
    enum class Access { READ_ONLY = 0, WRITE_ONLY = 1, READ_WRITE = 2, INTERRUPT = 3 };

    /**
     *  Enum descibing the data interpretation:
     *  \li Fixed point (includes integer = 0 fractional bits)
     *  \li IEEE754 floating point
     *  \li ASCII ascii characters
     *  \li VOID no data content, just trigger events (push type) FIXME: Currently implicit by 0 bits width
     *
     *  Note: The values need to be in "ascending" order of the information the type can hold. In 2D registers with
     *  different types in the channels, the type with the biggest value here will "win".
     */
    enum class Type { VOID = 0, FIXED_POINT = 1, IEEE754 = 2, ASCII = 3 };

    /**
     *  Per-channel information. For scalar and 1D registers, exactly one ChannelInfo is present. For 2D register, one
     *  ChannelInfo per channel is present.
     */
    struct ChannelInfo {
      uint32_t bitOffset;      /**< Offset in bits w.r.t. begining of the register. Often "big", i.e. byteOffset*8 */
      Type dataType;           /**< Data type (fixpoint, floating point) */
      uint32_t width;          /**< Number of significant bits in the register */
      int32_t nFractionalBits; /**< Number of fractional bits */
      bool signedFlag;         /**< Signed/Unsigned flag */
      DataType rawType;
      bool operator==(const ChannelInfo& rhs) const;
      bool operator!=(const ChannelInfo& rhs) const;
      [[nodiscard]] DataType getRawType() const;
    };

    struct DoubleBufferInfo {
      uint64_t address;                        /**< Secondary buffer address */
      RegisterPath enableRegisterPath;         /**< double buffer enable */
      RegisterPath inactiveBufferRegisterPath; /**< inactive register (BUF0/BUF1) */
      uint32_t index;                          /**< Index in enable register */
    };

    /**
     * Constructor to set all data members for scalar/1D registers. They all have default values, so this also acts as
     * default constructor.
     */
    explicit NumericAddressedRegisterInfo(RegisterPath const& pathName_ = {}, uint32_t nElements_ = 0,
        uint64_t address_ = 0, uint32_t nBytes_ = 0, uint64_t bar_ = 0, uint32_t width_ = 32,
        int32_t nFractionalBits_ = 0, bool signedFlag_ = true, Access dataAccess_ = Access::READ_WRITE,
        Type dataType_ = Type::FIXED_POINT, std::vector<size_t> interruptId_ = {},
        std::optional<DoubleBufferInfo> doubleBuffer_ = std::nullopt);

    /**
     * Constructor to set all data members for 2D registers.
     */
    NumericAddressedRegisterInfo(RegisterPath const& pathName_, uint64_t bar_, uint64_t address_, uint32_t nElements_,
        uint32_t elementPitchBits_, std::vector<ChannelInfo> channelInfo_, Access dataAccess_,
        std::vector<size_t> interruptId_, std::optional<DoubleBufferInfo> doubleBuffer_ = std::nullopt);

    NumericAddressedRegisterInfo(const NumericAddressedRegisterInfo&) = default;

    NumericAddressedRegisterInfo& operator=(const NumericAddressedRegisterInfo& other) = default;

    bool operator==(const ChimeraTK::NumericAddressedRegisterInfo& rhs) const;

    bool operator!=(const ChimeraTK::NumericAddressedRegisterInfo& rhs) const;

    [[nodiscard]] RegisterPath getRegisterName() const override { return pathName; }

    [[nodiscard]] unsigned int getNumberOfElements() const override { return nElements; }

    [[nodiscard]] unsigned int getNumberOfChannels() const override { return channels.size(); }

    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return dataDescriptor; }

    [[nodiscard]] bool isReadable() const override {
      return (registerAccess == Access::READ_ONLY) || (registerAccess == Access::READ_WRITE) ||
          (registerAccess == Access::INTERRUPT);
    }

    [[nodiscard]] bool isWriteable() const override {
      return (registerAccess == Access::WRITE_ONLY) || (registerAccess == Access::READ_WRITE);
    }

    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const override {
      AccessModeFlags flags;

      if(registerAccess == Access::INTERRUPT) {
        flags.add(AccessMode::wait_for_new_data);
      }

      if(channels.size() == 1 && channels.front().dataType != Type::VOID && channels.front().dataType != Type::ASCII) {
        flags.add(AccessMode::raw);
      }

      return flags;
    }

    RegisterPath pathName;

    uint32_t nElements;        /**< Number of elements in register */
    uint32_t elementPitchBits; /**< Distance in bits (!) between two elements (of the same channel) */

    uint64_t bar;     /**< Upper part of the address (name originally from PCIe, meaning now generalised) */
    uint64_t address; /**< Lower part of the address relative to BAR, in bytes */

    Access registerAccess; /**< Data access direction: Read, write, read and write or interrupt */
    std::vector<size_t> interruptId;
    std::optional<DoubleBufferInfo> doubleBuffer;
    /** Define per-channel information (bit interpretation etc.), 1D/scalars have exactly one entry. */
    std::vector<ChannelInfo> channels;

    DataDescriptor dataDescriptor;

    bool hidden{false};

    [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::unique_ptr<BackendRegisterInfoBase>(new NumericAddressedRegisterInfo(*this));
    }

    [[nodiscard]] std::vector<size_t> getQualifiedAsyncId() const override;

    void computeDataDescriptor();

    [[nodiscard]] bool isHidden() const override { return hidden; }
  };

  /********************************************************************************************************************/

  class NumericAddressedRegisterCatalogue : public BackendRegisterCatalogue<NumericAddressedRegisterInfo> {
   public:
    [[nodiscard]] NumericAddressedRegisterInfo getBackendRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] bool hasRegister(const RegisterPath& registerPathName) const override;

    [[nodiscard]] const std::set<std::vector<size_t>>& getListOfInterrupts() const;

    void addRegister(const NumericAddressedRegisterInfo& registerInfo);

    [[nodiscard]] std::unique_ptr<BackendRegisterCatalogueBase> clone() const override;

    [[nodiscard]] std::shared_ptr<async::DataConsistencyRealm> getDataConsistencyRealm(
        const std::vector<size_t>& qualifiedAsyncDomainId) const override;

    [[nodiscard]] RegisterPath getDataConsistencyKeyRegisterPath(
        const std::vector<size_t>& qualifiedAsyncDomainId) const override;

    void addDataConsistencyRealm(const RegisterPath& registerPath, const std::string& realmName);

   protected:
    void fillFromThis(NumericAddressedRegisterCatalogue* target) const;

    /**
     *  set of interrupt IDs. Each interrupt ID is a vector of (hierarchical) interrupt numbers.
     *  (Use a vector because it's the easiest container, and set because it ensures that each entry is there only once).
     */
    std::set<std::vector<size_t>> _listOfInterrupts;

    /** A canonical interrupt path consists of an
     *  exclamation mark, followed by a numeric interrupt and a colon separated
     *  list of hierarchical sub interrupts. For each interrupt with sub levels there
     *  is always a canonical interrupt for all higher levels.
     *
     *  Example:
     *  For the canonical interrupt `!3:5:9` there are is an interrupt `!3:5` and the
     *  primary interrupt `!3`.
     */
    std::map<RegisterPath, std::vector<size_t>> _canonicalInterrupts;

    /**
     * Map of data consistency key register paths to realm names
     */
    std::map<RegisterPath, std::string> _dataConsistencyRealms;
  };

  /********************************************************************************************************************/

} // namespace ChimeraTK
