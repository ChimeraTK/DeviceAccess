// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Device.h"
#include "FixedPointConverter.h"
#include "LNMAccessorPlugin.h"
#include "LNMMathPlugin.h"
#include "LogicalNameMappingBackend.h"
#include "NDRegisterAccessor.h"

#include <ChimeraTK/cppext/finally.hpp>

#include <algorithm>

namespace ChimeraTK {

  /********************************************************************************************************************/

  template<typename UserType>
  class LNMBackendVariableAccessor : public NDRegisterAccessor<UserType> {
   public:
    LNMBackendVariableAccessor(const boost::shared_ptr<DeviceBackend>& dev, const RegisterPath& registerPathName,
        size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags);

    ~LNMBackendVariableAccessor() override;

    void doReadTransferSynchronously() override;

    void doPreWrite(TransferType type, VersionNumber) override;

    void doPostWrite(TransferType type, VersionNumber versionNumber) override;

    bool doWriteTransfer(ChimeraTK::VersionNumber) override;

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>&) const override;

    [[nodiscard]] bool isReadOnly() const override;

    [[nodiscard]] bool isReadable() const override;

    [[nodiscard]] bool isWriteable() const override;

    void doPreRead(TransferType) override;

    void doPostRead(TransferType, bool hasNewData) override;

    void interrupt() override;

   protected:
    /// register and module name
    RegisterPath _registerPathName;

    /// backend device
    boost::shared_ptr<LogicalNameMappingBackend> _dev;

    /// register information. We have a shared pointer to the original RegisterInfo inside the map, since we need to
    /// modify the value in it (in case of a writeable variable register)
    LNMBackendRegisterInfo _info;
    // RegisterInfo _info;
    /// Word offset when reading
    size_t _wordOffsetInRegister;

    /// Intermediate buffer used when receiving value from queue, as writing to application buffer must only happen
    /// in doPostRead(). Only used when wait_for_new_data is set.
    typename LNMVariable::ValueTable<UserType>::QueuedValue _queueValue;

    /// Version number of the last transfer
    VersionNumber currentVersion{nullptr};

    /// access mode flags
    AccessModeFlags _flags;

    /// in case a MathPlugin formulas are using this variable, references to the FormulaHelpers
    std::list<boost::shared_ptr<LNMBackend::MathPluginFormulaHelper>> _formulaHelpers;

    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<TransferElement> /*newElement*/) override {} // LCOV_EXCL_LINE
  };

  DECLARE_TEMPLATE_FOR_CHIMERATK_USER_TYPES(LNMBackendVariableAccessor);

  /********************************************************************************************************************/

} // namespace ChimeraTK
