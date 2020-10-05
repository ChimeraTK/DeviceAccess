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
    DoubleBufferPlugin(boost::shared_ptr<LNMBackendRegisterInfo> info, std::map<std::string, std::string> parameters);

    void updateRegisterInfo() override;
    template<typename UserType, typename TargetType>
    boost::shared_ptr<NDRegisterAccessor<UserType>> decorateAccessor(
        boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<TargetType>>& target) const;

   private:
    std::map<std::string, std::string> _parameters;
  };

  template<typename UserType>
  class DoubleBufferAccessor : public NDRegisterAccessorDecorator<UserType, double> {
   public:
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::buffer_2D;
    using ChimeraTK::NDRegisterAccessorDecorator<UserType, double>::_target;

    DoubleBufferAccessor(boost::shared_ptr<LogicalNameMappingBackend>& backend,
        boost::shared_ptr<NDRegisterAccessor<double>>& target, const std::map<std::string, std::string>& parameters);

    void doPreRead(TransferType type) override;

    void doReadTransferSynchronously() override;

    void doPostRead(TransferType type, bool hasNewData) override;

    void doPreWrite(TransferType, VersionNumber) override {
      throw ChimeraTK::logic_error("LogicalNameMappingBackend DoubleBufferPlugin: Writing is not allowed.");
    }

    void doPostWrite(TransferType, VersionNumber) override {
      // do not throw here again
    }

   private:
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>> _bufferRegister;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>> _controlRegister;
    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>> _statusRegister;
  };
}} // namespace ChimeraTK::LNMBackend
