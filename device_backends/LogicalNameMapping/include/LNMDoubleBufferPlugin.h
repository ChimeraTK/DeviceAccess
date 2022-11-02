// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMappingBackend.h"
#include "NDRegisterAccessorDecorator.h"
#include "TransferElement.h"

#include <string>

namespace ChimeraTK { namespace LNMBackend {
  class DoubleBufferPlugin : public AccessorPlugin<DoubleBufferPlugin> {
    template<typename UserType>
    friend class DoubleBufferAccessorDecorator;

   public:
    DoubleBufferPlugin(LNMBackendRegisterInfo info, std::map<std::string, std::string> parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;
    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target, const UndecoratedParams& accessorParams) const;

   private:
    std::map<std::string, std::string> _parameters;
    std::string _targetDeviceName;
    // number of currently active reader threads
    boost::shared_ptr<std::atomic<uint32_t>> _readerCount;
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

    bool isWriteable() const override { return false; }

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend DoubleBufferPlugin: Writing is not allowed.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }

   private:
    // we know that plugin exists at least as long as any register (of the catalogue) refers to it,
    // so no shared_ptr required here
    const DoubleBufferPlugin& _plugin;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _secondBufferReg;

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};
  };

}} // namespace ChimeraTK::LNMBackend
