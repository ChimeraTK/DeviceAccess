// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "BackendRegisterCatalogue.h"
#include "BackendRegisterInfoBase.h"
#include "ForwardDeclarations.h"
#include "RegisterInfo.h"
#include "TransferElement.h"

#include <boost/shared_ptr.hpp>

#include <mutex>

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

    [[nodiscard]] RegisterPath getRegisterName() const override { return name; }

    [[nodiscard]] unsigned int getNumberOfElements() const override { return length; }

    [[nodiscard]] unsigned int getNumberOfChannels() const override { return nChannels; }

    [[nodiscard]] const DataDescriptor& getDataDescriptor() const override { return _dataDescriptor; }

    [[nodiscard]] bool isReadable() const override { return readable; }

    [[nodiscard]] bool isWriteable() const override { return writeable; }

    [[nodiscard]] AccessModeFlags getSupportedAccessModes() const override { return supportedFlags; }

    /** Name of the registrer */
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

    //    /** Hold values of CONSTANT or VARIABLE types in a type-dependent table. Only the entry matching the valueType
    //     *  is actually valid.
    //     *  Note: We cannot directly put the std::vector in a TemplateUserTypeMap, since it has more than one template
    //     *  arguments (with defaults). */
    //    template<typename T>
    //    struct ValueTable {
    //      std::vector<T> latestValue;
    //      DataValidity latestValidity;
    //      VersionNumber latestVersion;
    //      struct QueuedValue {
    //        std::vector<T> value;
    //        DataValidity validity;
    //        VersionNumber version;
    //      };
    //      std::map<TransferElementID, cppext::future_queue<QueuedValue>> subscriptions;
    //    };
    //    TemplateUserTypeMap<ValueTable> valueTable;

    //    /** Mutex one needs to hold while accessing valueTable. */
    //    std::mutex valueTable_mutex;

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

    [[nodiscard]] std::unique_ptr<BackendRegisterInfoBase> clone() const override {
      return std::make_unique<LNMBackendRegisterInfo>(*this);
    }
  };
  /********************************************************************************************************************/

  /*class LNMRegisterCatalogue : public BackendRegisterCatalogue<LNMBackendRegisterInfo> {
    public:
      LNMRegisterCatalogue() = default;
      LNMRegisterCatalogue& operator=(const LNMRegisterCatalogue& other) = default;
      LNMRegisterCatalogue(const LNMRegisterCatalogue&) = default;
  };*/

  /********************************************************************************************************************/
} /* namespace ChimeraTK */
