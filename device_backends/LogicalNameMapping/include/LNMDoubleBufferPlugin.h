#pragma once

#include "LNMAccessorPlugin.h"
#include "NDRegisterAccessorDecorator.h"
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
  class DoubleBufferAccessor : public ChimeraTK::NDRegisterAccessorDecorator<UserType, double> {};
}} // namespace ChimeraTK::LNMBackend
