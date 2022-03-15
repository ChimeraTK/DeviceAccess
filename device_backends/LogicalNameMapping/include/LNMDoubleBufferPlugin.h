#pragma once

#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"
#include "LNMBackendRegisterInfo.h"
#include "LogicalNameMappingBackend.h"
#include "TransferElement.h"
#include "LNMAccessorPlugin.h"
#include "LNMBackendRegisterInfo.h"

#include <string>

namespace ChimeraTK { namespace LNMBackend {
  class DoubleBufferPlugin : public AccessorPlugin<DoubleBufferPlugin> {
   public:
    DoubleBufferPlugin(LNMBackendRegisterInfo info, std::map<std::string, std::string> parameters);

    void updateRegisterInfo(BackendRegisterCatalogue<LNMBackendRegisterInfo>&) override;
    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

   private:
    std::map<std::string, std::string> _parameters;
    std::string _targetDeviceName;
  };

  template<typename UserType>
  class DoubleBufferAccessor : public NDRegisterAccessorDecorator<UserType> {
   public:
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType>::_target;

    DoubleBufferAccessor(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<UserType>>& target, const std::map<std::string, std::string>& parameters,
        std::string _targetDeviceName);

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
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<UserType>> _secondBufferReg;

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _enableDoubleBufferReg;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint32_t>> _currentBufferNumberReg;
    uint32_t _currentBuffer{0};
  };
}} // namespace ChimeraTK::LNMBackend
