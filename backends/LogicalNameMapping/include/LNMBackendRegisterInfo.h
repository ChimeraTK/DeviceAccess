// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterInfoBase.h"
#include <boost/shared_ptr.hpp>


namespace ChimeraTK {

  namespace LNMBackend {
    class AccessorPluginBase;
  } // namespace LNMBackend

  /** RegisterInfo structure for the LogicalNameMappingBackend */
  class LNMBackendRegisterInfo : public BackendRegisterInfoBase {
   public:
    /** Potential target types */
    enum TargetType { INVALID, REGISTER, CHANNEL, BIT, CONSTANT, VARIABLE };

    /** constructor: initialise values */
    LNMBackendRegisterInfo() = default;
    LNMBackendRegisterInfo(const LNMBackendRegisterInfo&) = default;
    LNMBackendRegisterInfo& operator=(const LNMBackendRegisterInfo& other) = default;

    [[nodiscard]] bool operator==(const LNMBackendRegisterInfo&) const = default;

    [[nodiscard]] RegisterPath getRegisterName() const override { return name; }

    [[nodiscard]] unsigned int getNumberOfElements() const override { return length; }

    [[nodiscard]] unsigned int getNumberOfChannels() const override { return nChannels; }

    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

    [[nodiscard]] bool isReadable() const override { return readable; }

    [[nodiscard]] bool isWriteable() const override { return writeable; }

    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const override { return supportedFlags; }

    [[nodiscard]] std::set<std::string> getTags() const override { return tags; }

    /** Name of the register */
    RegisterPath name;

    /** Type of the target */
    TargetType targetType{TargetType::INVALID};

    /** The target device alias */
    std::string deviceName;

    /** The target register name */
    std::string registerName;

    /** The first index in the range */
    unsigned int firstIndex{};

    /** The length of the range (i.e. number of indices) */
    unsigned int length{};

    /** The channel of the target 2D register (if TargetType::CHANNEL) */
    unsigned int channel{};

    /** The bit of the target register (if TargetType::BIT) */
    unsigned int bit{};

    /** The number of channels of the logical register */
    unsigned int nChannels{};

    /** Data type of CONSTANT or VARIABLE type. */
    DataType valueType;

    /** Flag if the register is readable. Might be derived from the target
     * register */
    bool readable{};

    /** Flag if the register is writeable. Might be derived from the target
     * register */
    bool writeable{};

    /** Supported AccessMode flags. Might be derived from the target register */
    AccessModeFlags supportedFlags;

    /** List of accessor plugins enabled for this register */
    std::vector<boost::shared_ptr<LNMBackend::AccessorPluginBase>> plugins;

    DataDescriptor _dataDescriptor;

    std::set<std::string> tags;

    /** Custom engineering unit overwritten by plugins (e.g. SetDescriptionPlugin). Empty if not set. */
    std::string engineeringUnit;

    /** Custom description overwritten by plugins (e.g. SetDescriptionPlugin). Empty if not set. */
    std::string description;

    /** Return the engineering unit of the register. */
    [[nodiscard]] std::string getUnit() const override { return engineeringUnit; }

    /** Return the description of the register. */
    [[nodiscard]] std::string getDescription() const override { return description; }

    [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<LNMBackendRegisterInfo>(*this);
    }
  };

  /********************************************************************************************************************/
} /* namespace ChimeraTK */
