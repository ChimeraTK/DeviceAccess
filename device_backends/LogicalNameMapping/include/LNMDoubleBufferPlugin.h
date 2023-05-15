// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMappingBackend.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <string>

namespace ChimeraTK::LNMBackend {
  class DoubleBufferPlugin : public AccessorPlugin<DoubleBufferPlugin> {
    template<typename UserType>
    friend class DoubleBufferAccessorDecorator;

   public:
    DoubleBufferPlugin(
        const LNMBackendRegisterInfo& info, size_t pluginIndex, std::map<std::string, std::string> parameters);

    void doRegisterInfoUpdate() override;
    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams) const;

   private:
    struct ReaderCount {
      uint32_t value = 0;
      std::mutex mutex;
    };
    std::map<std::string, std::string> _parameters;
    std::string _targetDeviceName;
    // number of currently active reader threads
    boost::shared_ptr<ReaderCount> _readerCount;
  };

  template<typename UserType>
  class DoubleBufferAccessorDecorator : public NDRegisterAccessorDecorator<UserType> {
   public:
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::_target;

    DoubleBufferAccessorDecorator(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<UserType>>& target, const DoubleBufferPlugin& plugin,
        const UndecoratedParams& accessorParams);

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    [[nodiscard]] bool isWriteable() const override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend DoubleBufferPlugin: Writing is not allowed.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }

    // below functions are needed for TransferGroup to work
    std::vector<boost::shared_ptr<TransferElement>> getHardwareAccessingElements() override;

    std::list<boost::shared_ptr<TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement> /* newElement */) override {
      // do nothing, we do not support merging of DoubleBufferAccessorDecorators
    }
    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<TransferElement const>& other) const override;

   private:
    // we know that plugin exists at least as long as any register (of the catalogue) refers to it,
    // so no shared_ptr required here
    const DoubleBufferPlugin& _plugin;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _secondBufferReg;

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};
    // FIXME - remove testUSleep feature
    uint32_t _testUSleep{0};
  };

} // namespace ChimeraTK::LNMBackend
